#include "dev_sensor.h"
#include "TickTimer.h"
#include "rtt_log.h"
#include <string.h>
#include <stdlib.h>
#include "timer6_timebase.h"
#include "EventBus.h"
// dev_sensor经过校准和灵敏度修正后的最终电流值(mA)
volatile int32_t g_dbg_sensor_final_ma = 0;
// ========== 过流 ISR 检测（方案二）状态与缓存 ==========
static Sensor_Device_t* s_pSensorOcDev = NULL;          // ISR 回调上下文（Init 时赋值）
static volatile uint8_t  s_u8IsrOcState = 0;            // 0=正常 1=计时中 2=已触发
static volatile uint64_t s_u64IsrOcStartUs = 0;         // 窗口起始 μs
static volatile uint32_t s_u32IsrOcWindowUs = 0;        // 锁存窗口(μs)
static volatile int32_t  s_s32IsrOcThresholdMa = 0;     // 锁存阈值(mA)
static volatile uint8_t  s_u8OcFilterType = SENSOR_OC_FILTER_NONE;   // 预留滤波类型
static volatile uint8_t  g_u8SensorOcPending = 0;       // ISR→主循环 待处理标志
static volatile Current_AlarmEvent_t g_sensor_oc_cache = {0, 0, 0};

/* ISR 过流调试变量（Keil Watch 可观察） */
volatile uint8_t  g_dbg_isr_oc_state = 0;
volatile uint64_t g_dbg_isr_oc_start_us = 0;
volatile uint32_t g_dbg_isr_oc_elapsed_us = 0;
volatile uint32_t g_dbg_isr_oc_window_us = 0;
volatile int32_t  g_dbg_isr_oc_cur_ma = 0;
volatile uint64_t g_u64OcTriggerUs = 0;       // 过流触发时刻（Timer6 μs，调试用）
static volatile uint16_t s_u16IsrOcBelowCount = 0;  // 连续低于阈值采样计数（回落容忍）
static volatile uint8_t  s_u8OcAttemptDone = 0;     // 1=本轮过流尝试已结束（触发/清除）
volatile uint16_t g_dbg_isr_oc_reset_cnt = 0;       // 本轮过流被回落清零重启的次数（调试）
volatile uint8_t  g_u8OcStopMeasureActive = 0; // 1=正在测量过流急停耗时（调试用）
/* 预计算：原始 ADC 阈值（配置/校准/灵敏度变化时由 Sensor_OcRefreshThresholdRaw 刷新） */
static volatile int32_t s_s32OcZeroRaw = 0;                 // 零点对应的原始 ADC 码
static volatile int32_t s_s32OcThresholdRaw = 0;            // 过流阈值对应的 |raw-零点| 半幅
static volatile int32_t s_s32IsrOcThresholdRawLatch = 0;    // 窗口起始锁存的原始阈值
static volatile int32_t s_s32OcSensitivity = SENSOR_SENSITIVITY_INT; // 灵敏度（mA 换算）
static volatile int32_t s_s32OcScale = 100;                 // 灵敏度缩放（默认100）
// ========== 模拟模式全局变量 ==========
#ifdef SENSOR_SIMULATION_MODE
    // 模拟的是传感器原始输出电压（mV），不是ADC输入电压
    static volatile uint16_t s_u16SimSensorRawMv = 1650;
    volatile uint16_t* const g_pu16DbgSimSensorRawMv = &s_u16SimSensorRawMv;
    
    // 根据传感器原始电压计算ADC输入电压（考虑分压电路）
    static uint16_t Sensor_Sim_GetAdcVoltageMv(uint16_t u16SensorRawMv) {
#if SENSOR_VOLTAGE_DIVIDER_ENABLE
        // V_adc = V_sensor * R2 / (R1 + R2)
        return (uint16_t)((uint32_t)u16SensorRawMv * SENSOR_DIVIDER_R2 / (SENSOR_DIVIDER_R1 + SENSOR_DIVIDER_R2));
#else
        return u16SensorRawMv;
#endif
    }
    
    // 根据期望电流值设置模拟值（更直观的接口）
    static void Sensor_Sim_SetCurrent(int32_t s32CurrentMa) {
        // 反向计算：Current -> 传感器原始电压
        // V_sensor = ZeroPoint + Current * Sensitivity
        int32_t s32SensorRawMv = SENSOR_RAW_ZERO_MV + 
                                  (s32CurrentMa * SENSOR_RAW_SENSITIVITY_MV_PER_A) / 1000;
        
        if (s32SensorRawMv < 0) s32SensorRawMv = 0;
        if (s32SensorRawMv > 3300) s32SensorRawMv = 3300;
        
        s_u16SimSensorRawMv = (uint16_t)s32SensorRawMv;
    }
    
    #define SIM_ADC_RAW_VALUE(voltage)  ((uint16_t)((uint32_t)(voltage) * 4095 / 3300))
#endif

// ========== 慢速打印 ==========
#ifdef DEBUG_SENSOR_SLOW
    static uint32_t s_u32LastSlowPrintTime = 0;
    #define SLOW_PRINT_INTERVAL_MS   4000
#endif

// ========== 真实模式调试 ==========
#ifdef DEV_SENSOR_REAL
    static NonBlockingDelay_t s_stcRealDbgTimer;
    static uint8_t s_u8RealDbgTimerInit = 0;
    #define SENSOR_REAL_PRINT_INTERVAL_MS   (3000)
    
    static uint8_t Sensor_RealDbg_IsTime(void) {
        if (!s_u8RealDbgTimerInit) {
            nbDelay_Init(&s_stcRealDbgTimer, SENSOR_REAL_PRINT_INTERVAL_MS);
            nbDelay_Start(&s_stcRealDbgTimer);
            s_u8RealDbgTimerInit = 1;
            return 1;
        }
        if (nbDelay_IsComplete(&s_stcRealDbgTimer)) {
            nbDelay_Start(&s_stcRealDbgTimer);
            return 1;
        }
        return 0;
    }
#endif

// ========== 窗口调试缓冲区 ==========
#ifdef DEBUG_SENSOR_WINDOW_BUFFER
    volatile uint16_t g_u16DbgSensorBuffer[SENSOR_WINDOW_BUFFER_SIZE] = {0};
    volatile uint16_t g_u16DbgSensorBufIndex = 0;
    volatile uint16_t g_u16DbgSensorBufCount = 0;
    volatile uint8_t  g_u8DbgSensorBufOverflow = 0;
    
    volatile int32_t  g_dbg_sensor_cur_ma = 0;
    volatile int16_t  g_dbg_sensor_cur_ax100 = 0;
    volatile uint16_t g_dbg_sensor_adc_mv = 0;
    volatile uint8_t  g_dbg_sensor_alarm = 0;
    volatile uint8_t  g_dbg_sensor_mode = 0;
    volatile uint16_t g_dbg_sensor_trigger_cnt = 0;
    volatile uint16_t g_dbg_sensor_trigger_win = 0;
    volatile uint16_t g_dbg_sensor_release_win = 0;
    volatile uint32_t g_dbg_sensor_elapsed_ms = 0;
    volatile uint32_t g_dbg_sensor_trigger_ms = 0;
    volatile uint32_t g_dbg_sensor_release_ms = 0;
    volatile uint8_t  g_dbg_sensor_timer_run = 0;
    
    static void Sensor_Debug_AddToWindow(uint16_t u16VoltageMv) {
        g_u16DbgSensorBuffer[g_u16DbgSensorBufIndex] = u16VoltageMv;
        g_u16DbgSensorBufIndex++;
        if (g_u16DbgSensorBufIndex >= SENSOR_WINDOW_BUFFER_SIZE) {
            g_u16DbgSensorBufIndex = 0;
            g_u8DbgSensorBufOverflow = 1;
        }
        if (g_u16DbgSensorBufCount < SENSOR_WINDOW_BUFFER_SIZE) {
            g_u16DbgSensorBufCount++;
        }
    }
#endif

// ========== 校准静态函数 ==========
static void Sensor_CalibrateZeroInternal(Sensor_Device_t* pstcDev, uint16_t u16AdcVoltageMv) {
    if (!pstcDev) return;
    
    int32_t s32ZeroTheory = SENSOR_VOUT_ZERO_MA_INT;
    int32_t s32ZeroMeas = (int32_t)u16AdcVoltageMv;
    int32_t s32ZeroOffset = s32ZeroMeas - s32ZeroTheory;
    
    pstcDev->stcCalibration.s32ZeroOffsetMv = s32ZeroOffset;
    pstcDev->stcCalibration.s32CalibrationValid = 0x5A5A5A5A;
    
    SENSOR_DEBUG("CALIBRATION: theory=%d mV, meas=%d mV, offset=%d mV\r\n",
                 (int)s32ZeroTheory, (int)s32ZeroMeas, (int)s32ZeroOffset);
    Sensor_OcRefreshThresholdRaw(pstcDev);
}

// ========== 电流计算静态函数 ==========
static int32_t Sensor_CalcCurrentInternal(Sensor_Device_t* pstcDev, uint16_t u16AdcVoltageMv) {
    if (!pstcDev) return 0;
    
    int32_t s32ZeroTheory = SENSOR_VOUT_ZERO_MA_INT;
    int32_t s32Sensitivity = SENSOR_SENSITIVITY_INT;
    int32_t s32ZeroOffset = pstcDev->stcCalibration.s32ZeroOffsetMv;
    
    if (pstcDev->stcCalibration.s32CalibrationValid != 0x5A5A5A5A) {
        s32ZeroOffset = 0;
    }
    
    int32_t s32Diff = (int32_t)u16AdcVoltageMv - s32ZeroTheory - s32ZeroOffset;
    
    int64_t s64Temp = (int64_t)s32Diff * 1000;
    int32_t s32CurrentMa = (int32_t)(s64Temp / s32Sensitivity);
    
    if (pstcDev->stcCalibration.s16SensitivityScale != 0 && 
        pstcDev->stcCalibration.s16SensitivityScale != 100) {
        s32CurrentMa = (s32CurrentMa * pstcDev->stcCalibration.s16SensitivityScale) / 100;
    }
    
    static uint8_t s_u8CalcPrinted = 0;
    if (!s_u8CalcPrinted) {
        s_u8CalcPrinted = 1;
        SENSOR_EMA_DBG("Calc: V=%d, ZeroTheory=%d, Sensitivity=%d, ZeroOffset=%d, Valid=0x%08X\r\n",
                       u16AdcVoltageMv,
                       (int)s32ZeroTheory,
                       (int)s32Sensitivity,
                       (int)s32ZeroOffset,
                       (unsigned int)pstcDev->stcCalibration.s32CalibrationValid);
        SENSOR_EMA_DBG("Result: Diff=%d, Current=%d mA\r\n", (int)s32Diff, (int)s32CurrentMa);
    }
    
    return s32CurrentMa;
}

// ========== 从ADC读取数据 ==========
static DeviceResult_t Sensor_ReadFromAdc(Sensor_Device_t* pstcDev) {
    if (!pstcDev) return RESULT_PARAM_ERR;
    
#ifdef SENSOR_SIMULATION_MODE
    uint16_t u16AdcVoltageMv = Sensor_Sim_GetAdcVoltageMv(s_u16SimSensorRawMv);
    pstcDev->u16AdcVoltageMv = u16AdcVoltageMv;
    pstcDev->u16AdcRawValue = SIM_ADC_RAW_VALUE(u16AdcVoltageMv);
    return RESULT_OK;
#else
    ADC_ReadResponse_t stcAdcResp;
    DeviceResult_t res = Device_Read(pstcDev->stcConfig.u8AdcDevId, &stcAdcResp, sizeof(ADC_ReadResponse_t));
    
#ifdef DEV_SENSOR_REAL
    if (Sensor_RealDbg_IsTime()) {
        SENSOR_REAL_DEBUG("ADC_Read: id=%d res=%d raw=%d mV=%d\r\n", 
                          pstcDev->stcConfig.u8AdcDevId, res, 
                          stcAdcResp.u16RawValue, stcAdcResp.u16VoltageMv);
    }
#endif
    
    if (res == RESULT_OK) {
        pstcDev->u16AdcRawValue = stcAdcResp.u16RawValue;
        pstcDev->u16AdcVoltageMv = stcAdcResp.u16VoltageMv;
    }
    
    return res;
#endif
}

// ========== 计算电流 ==========
static void Sensor_CalcCurrent(Sensor_Device_t* pstcDev) {
    if (!pstcDev) return;
    
    pstcDev->s32CurrentMa = Sensor_CalcCurrentInternal(pstcDev, pstcDev->u16AdcVoltageMv);
    pstcDev->s16CurrentAx100 = (int16_t)(pstcDev->s32CurrentMa / 10);
    
    // ============================================================
    // ★★★ JScope调试变量更新 - 取绝对值 ★★★
    // ============================================================
    g_dbg_sensor_final_ma = (pstcDev->s32CurrentMa >= 0) ? pstcDev->s32CurrentMa : -pstcDev->s32CurrentMa;
}

#if !SENSOR_OC_ISR_DETECT_ENABLE
// ========== 过流检测 - 点数模式 ==========
static void Sensor_CheckOvercurrent_SampleCount(Sensor_Device_t* pstcDev, 
                                                   uint8_t u8IsOvercurrent, 
                                                   uint8_t u8IsNormal,
                                                   int32_t s32CurrentMa,
                                                   int32_t s32TriggerThreshold,
                                                   int32_t s32AbsThreshold,
                                                   int32_t s32AbsHysteresis) {
    Sensor_AlarmState_t* pstcAlarm = &pstcDev->stcAlarmState;
    Sensor_Config_t* pstcCfg = &pstcDev->stcConfig;
    
    if (pstcAlarm->u8OvercurrentAlarm == 0) {
        if (u8IsOvercurrent) {
            pstcAlarm->u16ConsecutiveCount++;
            if (pstcAlarm->u16ConsecutiveCount >= pstcCfg->u16TriggerWindowSize) {
                pstcAlarm->u8OvercurrentAlarm = 1;
                pstcAlarm->u16ConsecutiveCount = 0;
                
                Current_AlarmEvent_t stcEvent;
                stcEvent.s32CurrentMa = s32CurrentMa;
                stcEvent.s32ThresholdMa = pstcCfg->s32OvercurrentThresholdMa;
                stcEvent.u8IsActive = 1;
                
                GPIO_RESET(GPIO_LED_PORT, GPIO_LED_PIN);
                EventBus_Publish(TOPIC_CURRENT_ALARM, &stcEvent);
            }
        } else {
            pstcAlarm->u16ConsecutiveCount = 0;
        }
    } else {
        if (u8IsNormal) {
            pstcAlarm->u16ConsecutiveCount++;
            if (pstcAlarm->u16ConsecutiveCount >= pstcCfg->u16ReleaseWindowSize) {
                pstcAlarm->u8OvercurrentAlarm = 0;
                pstcAlarm->u16ConsecutiveCount = 0;
                
#if OVERCURRENT_CLEAR_MODE == OVERCURRENT_CLEAR_AUTO
                Current_AlarmEvent_t stcEvent;
                stcEvent.s32CurrentMa = s32CurrentMa;
                stcEvent.s32ThresholdMa = pstcCfg->s32OvercurrentThresholdMa;
                stcEvent.u8IsActive = 0;
                EventBus_Publish(TOPIC_CURRENT_ALARM, &stcEvent);
#endif
            }
        } else if (u8IsOvercurrent) {
            pstcAlarm->u16ConsecutiveCount = 0;
        } else {
            pstcAlarm->u16ConsecutiveCount = 0;
        }
    }
}

// ========== 过流检测 - 时间模式 ==========
static void Sensor_CheckOvercurrent_TimeWindow(Sensor_Device_t* pstcDev, 
                                                 uint8_t u8IsOvercurrent, 
                                                 uint8_t u8IsNormal,
                                                 int32_t s32CurrentMa,
                                                 int32_t s32TriggerThreshold,
                                                 int32_t s32AbsThreshold,
                                                 int32_t s32AbsHysteresis) {
    (void)s32AbsThreshold;
    (void)s32AbsHysteresis;
    
    Sensor_AlarmState_t* pstcAlarm = &pstcDev->stcAlarmState;
    Sensor_Config_t* pstcCfg = &pstcDev->stcConfig;
    
    if (pstcAlarm->u8OvercurrentAlarm == 0) {
        if (u8IsOvercurrent) {
            if (!pstcAlarm->u8TimerRunning) {
                nbDelay_Init(&pstcAlarm->stcTriggerTimer, pstcCfg->u32TriggerWindowMs);
                nbDelay_Start(&pstcAlarm->stcTriggerTimer);
                pstcAlarm->u8TimerRunning = 1;
                
                SENSOR_TIMER_DBG("TIMER START: delayMs=%d, startTick=%d\r\n",
                                 (int)pstcCfg->u32TriggerWindowMs,
                                 (int)pstcAlarm->stcTriggerTimer.startTick);
            } else {
                if (nbDelay_IsComplete(&pstcAlarm->stcTriggerTimer)) {
                    uint64_t u64Elapsed = tickTimer_GetCount() - pstcAlarm->stcTriggerTimer.startTick;
                    
                    SENSOR_TIMER_DBG("OVERCURRENT TRIGGERED: elapsed=%d ms >= %d ms\r\n",
                                     (int)u64Elapsed, (int)pstcCfg->u32TriggerWindowMs);
                    
                    pstcAlarm->u8OvercurrentAlarm = 1;
                    pstcAlarm->u8TimerRunning = 0;
                    
                    Current_AlarmEvent_t stcEvent;
                    stcEvent.s32CurrentMa = s32CurrentMa;
                    stcEvent.s32ThresholdMa = pstcCfg->s32OvercurrentThresholdMa;
                    stcEvent.u8IsActive = 1;
                    EventBus_Publish(TOPIC_CURRENT_ALARM, &stcEvent);
                }
            }
        } else {
            if (pstcAlarm->u8TimerRunning) {
                SENSOR_TIMER_DBG("TIMER STOP: current normal, elapsed=%d ms\r\n",
                                 (int)(tickTimer_GetCount() - pstcAlarm->stcTriggerTimer.startTick));
                nbDelay_Stop(&pstcAlarm->stcTriggerTimer);
                pstcAlarm->u8TimerRunning = 0;
            }
        }
    } else {
        if (u8IsNormal) {
            if (!pstcAlarm->u8TimerRunning) {
                nbDelay_Init(&pstcAlarm->stcReleaseTimer, pstcCfg->u32ReleaseWindowMs);
                nbDelay_Start(&pstcAlarm->stcReleaseTimer);
                pstcAlarm->u8TimerRunning = 1;
                SENSOR_TIMER_DBG("RELEASE START: delayMs=%d\r\n", (int)pstcCfg->u32ReleaseWindowMs);
            } else {
                if (nbDelay_IsComplete(&pstcAlarm->stcReleaseTimer)) {
                    SENSOR_TIMER_DBG("OVERCURRENT CLEARED\r\n");
                    pstcAlarm->u8OvercurrentAlarm = 0;
                    pstcAlarm->u8TimerRunning = 0;
                    
#if OVERCURRENT_CLEAR_MODE == OVERCURRENT_CLEAR_AUTO
                    Current_AlarmEvent_t stcEvent;
                    stcEvent.s32CurrentMa = s32CurrentMa;
                    stcEvent.s32ThresholdMa = pstcCfg->s32OvercurrentThresholdMa;
                    stcEvent.u8IsActive = 0;
                    EventBus_Publish(TOPIC_CURRENT_ALARM, &stcEvent);
#endif
                }
            }
        } else if (u8IsOvercurrent) {
            if (pstcAlarm->u8TimerRunning) {
                nbDelay_Stop(&pstcAlarm->stcReleaseTimer);
                pstcAlarm->u8TimerRunning = 0;
                SENSOR_TIMER_DBG("RELEASE RESET: still overcurrent\r\n");
            }
        } else {
            if (pstcAlarm->u8TimerRunning) {
                nbDelay_Stop(&pstcAlarm->stcTriggerTimer);
                nbDelay_Stop(&pstcAlarm->stcReleaseTimer);
                pstcAlarm->u8TimerRunning = 0;
                SENSOR_TIMER_DBG("ALL STOP: unknown state\r\n");
            }
        }
    }
}

// ========== 过流检测统一入口 ==========
static void Sensor_CheckOvercurrent(Sensor_Device_t* pstcDev) {
    if (!pstcDev) return;

    extern volatile uint8_t g_u8MotorForwardBlankActive;
    extern volatile uint8_t g_u8MotorReverseBlankActive;
    if (g_u8MotorForwardBlankActive || g_u8MotorReverseBlankActive) {
        return;
    }

    int32_t s32CurrentMa = pstcDev->s32CurrentMa;
    Sensor_Config_t* pstcCfg = &pstcDev->stcConfig;
    
    int32_t s32AbsCurrent = (s32CurrentMa >= 0) ? s32CurrentMa : -s32CurrentMa;
    int32_t s32AbsThreshold = (pstcCfg->s32OvercurrentThresholdMa >= 0) ? 
                              pstcCfg->s32OvercurrentThresholdMa : -pstcCfg->s32OvercurrentThresholdMa;
    int32_t s32AbsHysteresis = (pstcCfg->s32OvercurrentHysteresisMa >= 0) ?
                               pstcCfg->s32OvercurrentHysteresisMa : -pstcCfg->s32OvercurrentHysteresisMa;
    
    int32_t s32TriggerThreshold = s32AbsThreshold + s32AbsHysteresis;
    int32_t s32ReleaseThreshold = s32AbsThreshold - s32AbsHysteresis;
    if (s32ReleaseThreshold < 0) s32ReleaseThreshold = 0;
    
    uint8_t u8IsOvercurrent = (s32AbsCurrent >= s32TriggerThreshold) ? 1 : 0;
    uint8_t u8IsNormal = (s32AbsCurrent <= s32ReleaseThreshold) ? 1 : 0;
    
    if (pstcCfg->u8OvercurrentMode == OVERCURRENT_MODE_SAMPLE_COUNT) {
        Sensor_CheckOvercurrent_SampleCount(pstcDev, u8IsOvercurrent, u8IsNormal,
                                             s32CurrentMa, s32TriggerThreshold,
                                             s32AbsThreshold, s32AbsHysteresis);
    } else {
        Sensor_CheckOvercurrent_TimeWindow(pstcDev, u8IsOvercurrent, u8IsNormal,
                                           s32CurrentMa, s32TriggerThreshold,
                                           s32AbsThreshold, s32AbsHysteresis);
    }
}
#endif

// ========== 过流 ISR 检测（方案二） ==========

/* 预留：过流判定输入滤波。当前默认直通原始值（SENSOR_OC_FILTER_NONE）。
 * SENSOR_OC_FILTER_MA4 预留为 4 点滑动平均(≈0.8ms @200μs)，纯压噪、不改“连续 40ms”窗口语义。
 * TODO(预留)：实现 MA4 时在此处理，并保持 ISR 内纯整数运算。 */
/* 预计算过流判定的原始 ADC 阈值（配置/校准/灵敏度变化时调用，主循环上下文） */
void Sensor_OcRefreshThresholdRaw(Sensor_Device_t* pstcDev)
{
    if (pstcDev == NULL) return;

    int32_t s32ZeroTheory = SENSOR_VOUT_ZERO_MA_INT;
    int32_t s32ZeroOffset = pstcDev->stcCalibration.s32ZeroOffsetMv;
    if (pstcDev->stcCalibration.s32CalibrationValid != 0x5A5A5A5A) {
        s32ZeroOffset = 0;   /* 未校准：用理论零点 */
    }
    int32_t s32Sens = SENSOR_SENSITIVITY_INT;
    int32_t s32Scale = pstcDev->stcCalibration.s16SensitivityScale;
    if (s32Scale == 0) s32Scale = 100;

    /* 零点原始码：raw = mV * 4095 / 3300 */
    s_s32OcZeroRaw = (int32_t)(((uint32_t)(s32ZeroTheory + s32ZeroOffset) * 4095UL) / 3300UL);

    /* 阈值(mA) -> 原始码半幅：
     *   base_mA  = threshold * 100 / scale （考虑灵敏度缩放）
     *   mv_diff  = base_mA * sens / 1000
     *   raw_diff = mv_diff * 4095 / 3300 */
    int32_t s32ThresholdMa = pstcDev->stcConfig.s32OvercurrentThresholdMa;
    if (s32ThresholdMa < 0) s32ThresholdMa = -s32ThresholdMa;
    int32_t s32BaseMa = (s32ThresholdMa * 100) / s32Scale;
    int32_t s32MvDiff  = (s32BaseMa * s32Sens) / 1000;
    s_s32OcThresholdRaw = (int32_t)(((uint32_t)s32MvDiff * 4095UL) / 3300UL);

    s_s32OcSensitivity = s32Sens;
    s_s32OcScale = s32Scale;
}
static uint16_t Sensor_OcFilterSample(Sensor_Device_t* pstcDev, uint16_t u16Raw)
{
    (void)pstcDev;
    if (s_u8OcFilterType == SENSOR_OC_FILTER_MA4) {
        return u16Raw;   /* 预留：暂未启用，保持直通 */
    }
    return u16Raw;
}

/* ADC 数据回调（ISR 上下文）：逐采样点过流判定，时间戳为 Timer6 μs */
static void Sensor_OcIsrCallback(uint16_t u16AdcValue, uint8_t u8Channel, void* pCtx)
{
    Sensor_Device_t* pstcDev = (Sensor_Device_t*)pCtx;
    if (pstcDev == NULL) return;
    (void)u8Channel;

    /* 0) 时间戳：先更新累加器再取 μs（量化 ≤ 一个采样周期） */
    Timer6_Timebase_UpdateTimestamp();
    uint64_t u64NowUs = Timer6_Timebase_GetTimestamp();

    /* 1) 消隐期间完全不判定，清空窗口状态 */
    extern volatile uint8_t g_u8MotorForwardBlankActive;
    extern volatile uint8_t g_u8MotorReverseBlankActive;
    if (g_u8MotorForwardBlankActive || g_u8MotorReverseBlankActive) {
        s_u8IsrOcState = 0;
        s_u16IsrOcBelowCount = 0;
        g_dbg_isr_oc_elapsed_us = 0;
        return;
    }

    /* 2) 输入滤波（预留接口，当前直通） */
    uint16_t u16Filtered = Sensor_OcFilterSample(pstcDev, u16AdcValue);

    /* 3) 预计算原始码阈值比较（int32，零除法）+ 电流 mA（调试/事件用，硬件除法） */
    int32_t s32Diff = (int32_t)u16Filtered - s_s32OcZeroRaw;
    int32_t s32AbsDiff = (s32Diff >= 0) ? s32Diff : -s32Diff;

    uint32_t u32Mv = ((uint32_t)s32AbsDiff * 3300UL) / 4095UL;
    int32_t s32CurrentMa = (int32_t)((u32Mv * 1000U) / (uint32_t)s_s32OcSensitivity);
    if (s_s32OcScale != 100) {
        s32CurrentMa = (s32CurrentMa * s_s32OcScale) / 100;
    }
    g_dbg_isr_oc_cur_ma = (s32CurrentMa >= 0) ? s32CurrentMa : -s32CurrentMa;

    /* 4) 阈值比较 + 过流状态机：严格连续 + 时间回落容忍（free-run）
     *    状态0 用预计算原始阈值判起始并锁存；状态1/2 用锁存值（本过程不受 485 改参影响） */
    int32_t s32CompareThreshold = (s_u8IsrOcState == 0) ?
                                  s_s32OcThresholdRaw :
                                  s_s32IsrOcThresholdRawLatch;

    if (s32AbsDiff >= s32CompareThreshold) {
        /* 超阈值：回落计数清零 */
        s_u16IsrOcBelowCount = 0;

        if (s_u8IsrOcState == 0) {
            /* 进入计时：锁存窗口/阈值；新一轮过流尝试则清 reset_cnt */
            s_u8IsrOcState = 1;
            s_u64IsrOcStartUs = u64NowUs;
            s_u32IsrOcWindowUs = (uint32_t)pstcDev->stcConfig.u32TriggerWindowMs * 1000UL;
            s_s32IsrOcThresholdMa = pstcDev->stcConfig.s32OvercurrentThresholdMa;
            s_s32IsrOcThresholdRawLatch = s_s32OcThresholdRaw;
            g_dbg_isr_oc_start_us = u64NowUs;
            g_dbg_isr_oc_window_us = s_u32IsrOcWindowUs;
            if (s_u8OcAttemptDone) {
                s_u8OcAttemptDone = 0;
                g_dbg_isr_oc_reset_cnt = 0;   /* 新一轮过流尝试：清零重启计数 */
            }
        } else if (s_u8IsrOcState == 1) {
            /* 计时中：检查窗口是否满足（free-run：容忍回落期间墙钟继续走） */
            uint64_t u64Elapsed = u64NowUs - s_u64IsrOcStartUs;
            g_dbg_isr_oc_elapsed_us = (uint32_t)u64Elapsed;
            if (u64Elapsed >= s_u32IsrOcWindowUs) {
                s_u8IsrOcState = 2;
                s_u8OcAttemptDone = 1;      /* 本轮尝试已触发 */
                /* 写缓存（仅当 pending==0，避免覆盖主循环未处理的事件） */
                if (g_u8SensorOcPending == 0) {
                    g_sensor_oc_cache.s32CurrentMa = s32CurrentMa;
                    g_sensor_oc_cache.s32ThresholdMa = s_s32IsrOcThresholdMa;
                    g_sensor_oc_cache.u8IsActive = 1;
                    g_u64OcTriggerUs = u64NowUs;        /* T0：过流触发时刻 */
                    g_u8OcStopMeasureActive = 1;        /* 标记开始测量急停耗时 */
                    g_u8SensorOcPending = 1;
                }
            }
        }
        /* 状态2：保持，等待主循环处理或电流回落后自动重新武装 */
    } else {
        /* 低于阈值：回落容忍，连续低于 DROP_SAMPLES 才作废窗口 */
        s_u16IsrOcBelowCount++;
        if (s_u16IsrOcBelowCount >= SENSOR_OC_DROP_TOLERANCE_SAMPLES) {
            uint8_t u8WasTiming = (s_u8IsrOcState == 1);
            s_u8IsrOcState = 0;
            s_u16IsrOcBelowCount = 0;
            g_dbg_isr_oc_elapsed_us = 0;
            if (u8WasTiming) {
                g_dbg_isr_oc_reset_cnt++;   /* 计时中被回落清零重启（诊断） */
            }
        }
    }

    g_dbg_isr_oc_state = s_u8IsrOcState;
}

/* 主循环调用（ESystem_MainLoop 顶部）：发布 ISR 缓存的过流事件，走现有链路 */
void Sensor_Device_ProcessPendingEvent(void)
{
    if (s_pSensorOcDev == NULL) return;
    if (!EventBus_IsEnabled()) return;   /* 未使能前保持 pending，防止事件被吞 */
    if (!g_u8SensorOcPending) return;

    Current_AlarmEvent_t stcCopy;
    stcCopy = g_sensor_oc_cache;         /* 先拷贝 */
    g_u8SensorOcPending = 0;             /* 后清标志 */

    EventBus_Publish(TOPIC_CURRENT_ALARM, &stcCopy);
}

/* 预留滤波接口：当前仅 NONE 生效；MA4 记录类型但暂未实现（保持直通） */
void Sensor_Device_SetOcFilter(uint8_t u8FilterType)
{
    if (u8FilterType == SENSOR_OC_FILTER_NONE) {
        s_u8OcFilterType = u8FilterType;
        SENSOR_DEBUG("OC filter set to NONE (raw passthrough)\r\n");
    } else if (u8FilterType == SENSOR_OC_FILTER_MA4) {
        s_u8OcFilterType = u8FilterType;
        SENSOR_DEBUG("OC filter set to MA4 (reserved, still passthrough)\r\n");
    } else {
        SENSOR_DEBUG("OC filter type invalid: %d\r\n", (int)u8FilterType);
    }
}

// ========== 标准设备操作 ==========
DeviceResult_t Sensor_Device_Init(void* handle) {
    Sensor_Device_t* pstcDev = (Sensor_Device_t*)handle;
    if (!pstcDev) return RESULT_PARAM_ERR;
    
    memset(&pstcDev->stcCalibration, 0, sizeof(Sensor_Calibration_t));
    pstcDev->stcCalibration.s16SensitivityScale = 100;
    pstcDev->u8Calibrated = 0;
    pstcDev->u32InitTime = tickTimer_GetCount();
    
    SENSOR_DEBUG("Init: ADC Dev ID=%d\r\n", pstcDev->stcConfig.u8AdcDevId);
    
#if SENSOR_VOLTAGE_DIVIDER_ENABLE
    SENSOR_DEBUG("Mode: Voltage Divider ENABLED (R1=%d Ohm, R2=%d Ohm)\r\n", 
                 (int)SENSOR_DIVIDER_R1, (int)SENSOR_DIVIDER_R2);
    SENSOR_DEBUG("V_zero_theory=%d mV, Sensitivity=%d mV/A\r\n",
                 (int)SENSOR_VOUT_ZERO_MA_INT, (int)SENSOR_SENSITIVITY_INT);
#else
    SENSOR_DEBUG("Mode: Voltage Divider DISABLED\r\n");
#endif
    
    pstcDev->u16AdcRawValue = 0;
    pstcDev->u16AdcVoltageMv = 0;
    pstcDev->s32CurrentMa = 0;
    pstcDev->s16CurrentAx100 = 0;
    
    pstcDev->stcAlarmState.u8OvercurrentAlarm = 0;
    pstcDev->stcAlarmState.u16ConsecutiveCount = 0;
    pstcDev->stcAlarmState.u8TimerRunning = 0;
    nbDelay_Init(&pstcDev->stcAlarmState.stcTriggerTimer, 0);
    nbDelay_Init(&pstcDev->stcAlarmState.stcReleaseTimer, 0);
    
    Sensor_ReadFromAdc(pstcDev);
    Sensor_CalcCurrent(pstcDev);
    
    pstcDev->u8Initialized = 1;
    pstcDev->u32LastUpdateTime = tickTimer_GetCount();
    /* 注册 ADC 数据回调（方案二：ISR 过流判定） */
    {
        DeviceNode_t* pstcAdcNode = DeviceManager_Get(pstcDev->stcConfig.u8AdcDevId);
        if (pstcAdcNode && pstcAdcNode->private_data) {
            ADC_Device_t* pstcAdcDev = (ADC_Device_t*)pstcAdcNode->private_data;
            ADC_Device_SetDataCallback(pstcAdcDev, Sensor_OcIsrCallback, (void*)pstcDev);
            s_pSensorOcDev = pstcDev;
            SENSOR_DEBUG("OC ISR callback registered (ADC dev id=%d)\r\n", pstcDev->stcConfig.u8AdcDevId);
        } else {
            SENSOR_DEBUG("Warning: ADC device not found, OC ISR callback NOT registered!\r\n");
        }
    }
    
    Sensor_OcRefreshThresholdRaw(pstcDev);
    SENSOR_DEBUG("Init success: ADC mV=%d, Current=%d mA (waiting for calibration)\r\n",
                 pstcDev->u16AdcVoltageMv, (int)pstcDev->s32CurrentMa);
    
    return RESULT_OK;
}

DeviceResult_t Sensor_Device_Update(void* handle) {
    Sensor_Device_t* pstcDev = (Sensor_Device_t*)handle;
    if (!pstcDev || !pstcDev->u8Initialized) return RESULT_ERROR;
    
    if (!pstcDev->u8Calibrated) {
        
        if (!pstcDev->stcCalibState.u8CalibDelayDone) {
            SENSOR_DEBUG("Waiting %d ms for power surge to settle...\r\n",
                         (int)SENSOR_CALIB_DELAY_MS);
            tickTimer_DelayMs(SENSOR_CALIB_DELAY_MS);
            pstcDev->stcCalibState.u8CalibDelayDone = 1;
            pstcDev->stcCalibState.u32CalibStartTime = tickTimer_GetCount();
            SENSOR_DEBUG("Power surge wait done, starting ADC stabilization...\r\n");
            
            Sensor_ReadFromAdc(pstcDev);
            pstcDev->u32LastUpdateTime = tickTimer_GetCount();
            return RESULT_OK;
        }
        
        uint32_t u32Now = tickTimer_GetCount();
        uint32_t u32Elapsed = u32Now - pstcDev->stcCalibState.u32CalibStartTime;
        
        Sensor_ReadFromAdc(pstcDev);
        uint16_t u16Voltage = pstcDev->u16AdcVoltageMv;
        
        SENSOR_DEBUG("Calibration sampling: elapsed=%d ms, V=%d mV\r\n",
                     (int)u32Elapsed, u16Voltage);
        
        if (u32Elapsed >= SENSOR_CALIB_STABLE_MS) {
            
            if (u16Voltage > 100 && u16Voltage < 3300) {
                Sensor_CalibrateZeroInternal(pstcDev, u16Voltage);
                pstcDev->u8Calibrated = 1;
                SENSOR_DEBUG("Calibration DONE: V=%d mV, Offset=%d mV\r\n",
                             u16Voltage, (int)pstcDev->stcCalibration.s32ZeroOffsetMv);
            } else {
                SENSOR_DEBUG("Calibration FAILED: voltage abnormal (%d mV), using theoretical zero\r\n",
                             u16Voltage);
                pstcDev->stcCalibration.s32ZeroOffsetMv = 0;
                pstcDev->stcCalibration.s32CalibrationValid = 0x5A5A5A5A;
                Sensor_OcRefreshThresholdRaw(pstcDev);
                pstcDev->u8Calibrated = 1;
            }
            
            Sensor_CalcCurrent(pstcDev);
            pstcDev->u32LastUpdateTime = tickTimer_GetCount();
            return RESULT_OK;
        }
        
        pstcDev->u32LastUpdateTime = tickTimer_GetCount();
        return RESULT_OK;
    }
    
    DeviceResult_t res = Sensor_ReadFromAdc(pstcDev);
    if (res != RESULT_OK) {
        return res;
    }
    
    Sensor_CalcCurrent(pstcDev);
    
    static uint32_t s_u32LastEmaPrint = 0;
    uint32_t u32Now = tickTimer_GetCount();
    if (u32Now - s_u32LastEmaPrint >= 100) {
        s_u32LastEmaPrint = u32Now;
        SENSOR_EMA_DBG("ADC_mV=%d, Current=%d mA, alarm=%d, timerRunning=%d\r\n",
                       pstcDev->u16AdcVoltageMv,
                       (int)pstcDev->s32CurrentMa,
                       (s_u8IsrOcState == 2) ? 1 : 0,
                       pstcDev->stcAlarmState.u8TimerRunning);
    }
    
#if !SENSOR_OC_ISR_DETECT_ENABLE
    Sensor_CheckOvercurrent(pstcDev);
#endif
    
#ifdef DEBUG_SENSOR_WINDOW_BUFFER
    Sensor_Debug_AddToWindow(pstcDev->u16AdcVoltageMv);
    g_dbg_sensor_cur_ma = pstcDev->s32CurrentMa;
    g_dbg_sensor_cur_ax100 = pstcDev->s16CurrentAx100;
    g_dbg_sensor_adc_mv = pstcDev->u16AdcVoltageMv;
    g_dbg_sensor_alarm = (uint8_t)((s_u8IsrOcState == 2) ? 1 : 0);
#endif
    
#ifdef DEBUG_SENSOR_SLOW
    if (u32Now - s_u32LastSlowPrintTime >= SLOW_PRINT_INTERVAL_MS) {
        s_u32LastSlowPrintTime = u32Now;
        int32_t s32AbsCurrentSlow = (pstcDev->s32CurrentMa >= 0) ? pstcDev->s32CurrentMa : -pstcDev->s32CurrentMa;
        int16_t s16AbsAx100 = (pstcDev->s16CurrentAx100 >= 0) ? pstcDev->s16CurrentAx100 : -pstcDev->s16CurrentAx100;
        
        SENSOR_DEBUG_SLOW("Current=%s%d.%02dA (ADC Raw=%d, ADC mV=%d)%s\r\n",
            (pstcDev->s16CurrentAx100 < 0) ? "-" : "",
            (int)(s16AbsAx100 / 100),
            (int)(s16AbsAx100 % 100),
            pstcDev->u16AdcRawValue,
            pstcDev->u16AdcVoltageMv,
            (s_u8IsrOcState == 2) ? " [OVERCURRENT]" : "");
    }
#endif
    
    pstcDev->u32LastUpdateTime = tickTimer_GetCount();
    return RESULT_OK;
}

DeviceResult_t Sensor_Device_Deinit(void* handle) {
    Sensor_Device_t* pstcDev = (Sensor_Device_t*)handle;
    if (!pstcDev) return RESULT_PARAM_ERR;
    pstcDev->u8Initialized = 0;
    return RESULT_OK;
}

DeviceResult_t Sensor_Device_Read(void* handle, void* data, uint32_t size) {
    Sensor_Device_t* pstcDev = (Sensor_Device_t*)handle;
    if (!pstcDev || !data) return RESULT_PARAM_ERR;
    if (!pstcDev->u8Initialized) return RESULT_ERROR;
    
    if (size == sizeof(Sensor_ReadResponse_t)) {
        Sensor_ReadResponse_t* pstcResp = (Sensor_ReadResponse_t*)data;
        pstcResp->s32CurrentMa = pstcDev->s32CurrentMa;
        pstcResp->s16CurrentAx100 = pstcDev->s16CurrentAx100;
        pstcResp->u16AdcRawValue = pstcDev->u16AdcRawValue;
        pstcResp->u16AdcVoltageMv = pstcDev->u16AdcVoltageMv;
        return RESULT_OK;
    }
    return RESULT_PARAM_ERR;
}

DeviceResult_t Sensor_Device_Write(void* handle, const void* data, uint32_t size) {
    (void)handle;
    (void)data;
    (void)size;
    return RESULT_ERROR;
}

DeviceResult_t Sensor_Device_Control(void* handle, DeviceCommandData_t* pstcCmd) {
    Sensor_Device_t* pstcDev = (Sensor_Device_t*)handle;
    if (!pstcDev || !pstcCmd) return RESULT_PARAM_ERR;
    if (!pstcDev->u8Initialized) return RESULT_ERROR;
    
    switch (pstcCmd->cmd) {
        case CMD_SENSOR_GET_CURRENT_MA:
            if (pstcCmd->response && pstcCmd->response_size >= sizeof(int32_t)) {
                *(int32_t*)pstcCmd->response = pstcDev->s32CurrentMa;
                return RESULT_OK;
            }
            return RESULT_PARAM_ERR;
        case CMD_SENSOR_GET_CURRENT_AX100:
            if (pstcCmd->response && pstcCmd->response_size >= sizeof(int16_t)) {
                *(int16_t*)pstcCmd->response = pstcDev->s16CurrentAx100;
                return RESULT_OK;
            }
            return RESULT_PARAM_ERR;
        case CMD_SENSOR_GET_ALARM_STATUS:
            if (pstcCmd->response && pstcCmd->response_size >= sizeof(uint8_t)) {
                *(uint8_t*)pstcCmd->response = (uint8_t)(((s_u8IsrOcState == 2) || (g_u8SensorOcPending == 1)) ? 1 : 0);
                return RESULT_OK;
            }
            return RESULT_PARAM_ERR;
        case CMD_SENSOR_GET_CALIBRATION:
            if (pstcCmd->response && pstcCmd->response_size >= sizeof(Sensor_Calibration_t)) {
                *(Sensor_Calibration_t*)pstcCmd->response = pstcDev->stcCalibration;
                return RESULT_OK;
            }
            return RESULT_PARAM_ERR;
        default:
            return RESULT_ERROR;
    }
}

// ========== 电流传感器特定接口 ==========
int32_t Sensor_Device_GetCurrentMA(Sensor_Device_t* pstcDev) {
    if (!pstcDev || !pstcDev->u8Initialized) return 0;
    return pstcDev->s32CurrentMa;
}

int16_t Sensor_Device_GetCurrentAx100(Sensor_Device_t* pstcDev) {
    if (!pstcDev || !pstcDev->u8Initialized) return 0;
    return pstcDev->s16CurrentAx100;
}

Sensor_Device_t* Sensor_Device_Create(const Sensor_Config_t* pstcConfig) {
    if (!pstcConfig) return NULL;
    
    Sensor_Device_t* pstcDev = (Sensor_Device_t*)malloc(sizeof(Sensor_Device_t));
    if (!pstcDev) return NULL;
    
    memset(pstcDev, 0, sizeof(Sensor_Device_t));
    pstcDev->stcConfig = *pstcConfig;
    pstcDev->u8Initialized = 0;
    
    return pstcDev;
}

// ========== 校准接口实现 ==========
void Sensor_Device_CalibrateZero(Sensor_Device_t* pstcDev) {
    if (!pstcDev || !pstcDev->u8Initialized) return;
    
    Sensor_ReadFromAdc(pstcDev);
    Sensor_CalibrateZeroInternal(pstcDev, pstcDev->u16AdcVoltageMv);
    Sensor_CalcCurrent(pstcDev);
    pstcDev->u8Calibrated = 1;
    
    SENSOR_DEBUG("Manual calibration done: ZeroOffset=%d mV\r\n",
                 (int)pstcDev->stcCalibration.s32ZeroOffsetMv);
}

void Sensor_Device_SetSensitivityScale(Sensor_Device_t* pstcDev, int16_t s16ScalePercent) {
    if (!pstcDev) return;
    if (s16ScalePercent >= 50 && s16ScalePercent <= 200) {
        pstcDev->stcCalibration.s16SensitivityScale = s16ScalePercent;
        Sensor_OcRefreshThresholdRaw(pstcDev);
        if (pstcDev->u8Initialized) {
            Sensor_CalcCurrent(pstcDev);
        }
    }
}

void Sensor_Device_GetCalibration(Sensor_Device_t* pstcDev, Sensor_Calibration_t* pstcCal) {
    if (!pstcDev || !pstcCal) return;
    *pstcCal = pstcDev->stcCalibration;
}

// ========== 模拟模式接口 ==========
#ifdef SENSOR_SIMULATION_MODE
void Sensor_SetSimulationValue(uint16_t u16VoltageMv) {
    s_u16SimSensorRawMv = u16VoltageMv;
}

void Sensor_SetSimulationCurrent(int32_t s32CurrentMa) {
    Sensor_Sim_SetCurrent(s32CurrentMa);
}

uint16_t Sensor_GetSimulationSensorRawMv(void) {
    return s_u16SimSensorRawMv;
}
#endif

// ========== 过流告警手动清除接口 ==========
void Sensor_Device_ClearAlarm(Sensor_Device_t* pstcDev) {
    if (!pstcDev || !pstcDev->u8Initialized) return;

    /* 方案二：无条件复位 ISR 过流状态机（锁存 + 丢弃陈旧 pending），保证清除后能再次触发 */
    s_u8IsrOcState = 0;
    s_u16IsrOcBelowCount = 0;
    s_u8OcAttemptDone = 0;
    g_dbg_isr_oc_reset_cnt = 0;
    g_u8SensorOcPending = 0;
    g_dbg_isr_oc_state = 0;
    g_dbg_isr_oc_elapsed_us = 0;

    pstcDev->stcAlarmState.u8OvercurrentAlarm = 0;
    pstcDev->stcAlarmState.u16ConsecutiveCount = 0;
    pstcDev->stcAlarmState.u8TimerRunning = 0;
    nbDelay_Stop(&pstcDev->stcAlarmState.stcTriggerTimer);
    nbDelay_Stop(&pstcDev->stcAlarmState.stcReleaseTimer);

    Current_AlarmEvent_t stcEvent;
    stcEvent.s32CurrentMa = pstcDev->s32CurrentMa;
    stcEvent.s32ThresholdMa = pstcDev->stcConfig.s32OvercurrentThresholdMa;
    stcEvent.u8IsActive = 0;
    EventBus_Publish(TOPIC_CURRENT_ALARM, &stcEvent);
}

// ========== 全局操作函数表 ==========
const DeviceOps_t g_sensor_ops = {
    .init = Sensor_Device_Init,
    .deinit = Sensor_Device_Deinit,
    .read = Sensor_Device_Read,
    .write = Sensor_Device_Write,
    .control = Sensor_Device_Control,
    .update = Sensor_Device_Update
};
