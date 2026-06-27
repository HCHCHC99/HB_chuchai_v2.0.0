#include "App_Motor_Project.h"
#include "hc32_ll.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "rtt_log.h"
#include "dev_rturn.h"          //         
#include "App_Params.h"         //      g_AppParam   ? Modbus  ?       
// ==========   ???  ?  ??    ? ==========
SystemSim_t g_sim = {
    .sim_pwr_pos = 0,
    .sim_pwr_neg = 0,
    .sim_hall_up = 0,
    .sim_hall_down = 0,
    .sim_io_fwd = 0,
    .sim_io_rev = 0,
    .sim_io_speed = 85.0f,
    .sim_adc_val = 1500,
    .sim_motor_speed = 0,
    .sim_motor_dir = 0
};

SystemStatus_t g_status = {
    .motor_status = 0,
    .power_status = 0,
    .hall_status = 0,
    .io_status = 0,
    .current_duty = 0.0f
};

//    ?   ?        ?       Γ±      
RTurn_Device_t* g_rturn_dev = NULL;


// ========== ?2  ?      ?   ? y ==========
static PWM_Device_t g_pwm_dev;
static MotorDevice_t g_motor_dev;
// Keil Watch      ? ?     ?   ?    ?  ?  
MotorDevice_t* volatile g_pMotorDevWatch = NULL;
static Power_Device_t* g_pwr_pos_dev = NULL;
static Power_Device_t* g_pwr_neg_dev = NULL;
static IO_Device_t* g_io_fwd_dev = NULL;
static IO_Device_t* g_io_rev_dev = NULL;
static Hall_Device_t* g_hall_up_dev = NULL;
static Hall_Device_t* g_hall_down_dev = NULL;
static Voltage_Device_t* g_voltage_bus_dev = NULL;
static Sensor_Device_t* g_sensor_current_dev = NULL;
static MotorHall_Device_t* g_motor_hall_dev = NULL;

// ========== ?  2?o    y    ?   ==========
static void RegisterAllDevices(void);
static void SetupEventBusSubscriptions(void);
static void UpdateStatusIndicators(void);
static void ProcessDeviceUpdates(void);
static void SetDeviceUpdateIntervals(void);

// ==========       ?    2  o    y ==========

static void RegisterAllDevices(void) {
    // // 1.     2  PWM      ?
    // PWM_Config_t pwmConfig = {
    //     .tmr = NULL,  // D    a?  ?Y    ?    2?t????
    //     .periphClk = 0,
    //     .channel = 0,
    //     .port = 0,
    //     .pin = 0,
    //     .pinFunc = 0,
    //     .countMode = 0,
    //     .countDir = 0,
    //     .defaultFreqHz = 20000,
    //     .defaultDutyPercent = 0,
    //     .activePolarity = PWM_ACTIVE_HIGH
    // };
    
    // memcpy(&g_pwm_dev.config, &pwmConfig, sizeof(PWM_Config_t));
    
    // DeviceOps_t pwm_ops = {
    //     .init = PWM_Device_Init,
    //     .deinit = PWM_Device_Deinit,
    //     .read = PWM_Device_Read,
    //     .write = PWM_Device_Write,
    //     .control = PWM_Device_Control,
    //     .update = PWM_Device_Update
    // };
    // DeviceManager_Register(ID_PWM_MOTOR, "PWM_Motor", DEVICE_TYPE_PWM, &g_pwm_dev, pwm_ops);

    // 2.     2  ?y  ?? ?    ?
    Power_Config_t pwrPosCfg = {
        .port = GPIO_PORT_C,
        .pin = GPIO_PIN_13,
        .active_level = 1,
        .power_id = 0,
        .debounce_ms = 20,
        .window_size = 5,
        .sample_interval = 5
    };
    g_pwr_pos_dev = Power_Device_Create(&pwrPosCfg);
    DeviceManager_Register(ID_PWR_POS, "PowerPositive", DEVICE_TYPE_POWER, 
                           g_pwr_pos_dev, g_power_ops);

    // // 3.     2  ?o  ?? ?    ? (?  PB2    )
    // Power_Config_t pwrNegCfg = {
    //     .port = GPIO_PORT_C,
    //     .pin = GPIO_PIN_14,
    //     .active_level = 1,
    //     .power_id = 1,
    //     .debounce_ms = 20,
    //     .window_size = 5,
    //     .sample_interval = 5
    // };
    // g_pwr_neg_dev = Power_Device_Create(&pwrNegCfg);
    // DeviceManager_Register(ID_PWR_NEG, "PowerNegative", DEVICE_TYPE_POWER, 
    //                        g_pwr_neg_dev, g_power_ops);



    // ?   ?    υτ?  
    
    // Hall_Config_t upCfg = {
    //     .port = GPIO_PORT_B,
    //     .pin = GPIO_PIN_02,
    //     .active_level = 0,           //  ?   ???  DD  ? ?   ς    - ?   ?
    //     .bind_dir = DIR_FWD,
    //     .is_soft_limit = 0,
    //     .debounce_ms = 20,
    //     .window_size = 3,
    //     .sample_interval = 2,
    //     .hall_id = 0
    // };
    // g_hall_up_dev = Hall_Device_Create(&upCfg);
    // DeviceManager_Register(ID_HALL_UP, "LimitUp", DEVICE_TYPE_HALL, 
    //                     g_hall_up_dev, g_hall_ops);

    // // 5.     2  ???T?????? (PC14)
    // Hall_Config_t downCfg = {
    //     .port = GPIO_PORT_B,
    //     .pin = GPIO_PIN_10,
    //     .active_level = 0,           //  ?   ???  DD  ? ?   ς    - ?   ?
    //     .bind_dir = DIR_REV,
    //     .is_soft_limit = 0,
    //     .debounce_ms = 20,
    //     .window_size = 3,
    //     .sample_interval = 2,
    //     .hall_id = 1
    // };
    // g_hall_down_dev = Hall_Device_Create(&downCfg);
    // DeviceManager_Register(ID_HALL_DOWN, "LimitDown", DEVICE_TYPE_HALL, 
    //                     g_hall_down_dev, g_hall_ops);
    

    // // 6.     2  ?y  aIO      ?
    // IO_Config_t ioFwdCfg = {
    //     .port = GPIO_PORT_B,
    //     .pin = GPIO_PIN_13,
    //     .active_level = 1,
    //     .debounce_ms = 20,
    //     .window_size = 3,
    //     .sample_interval = 2,
    //     .io_id = 0
    // };
    // g_io_fwd_dev = IO_Device_Create(&ioFwdCfg);
    // DeviceManager_Register(ID_IO_FWD, "IO_Forward", DEVICE_TYPE_IO, 
    //                        g_io_fwd_dev, g_io_ops);

    // // 7.     2       aIO      ?
    // IO_Config_t ioRevCfg = {
    //     .port = GPIO_PORT_B,
    //     .pin = GPIO_PIN_12,
    //     .active_level = 1,
    //     .debounce_ms = 20,
    //     .window_size = 3,
    //     .sample_interval = 2,
    //     .io_id = 1
    // };
    // g_io_rev_dev = IO_Device_Create(&ioRevCfg);
    // DeviceManager_Register(ID_IO_REV, "IO_Reverse", DEVICE_TYPE_IO, 
    //                        g_io_rev_dev, g_io_ops);

    // 8.     2    ??  ?  2?      ?
    DeviceOps_t motor_ops = {
        .init = Motor_Init,
        .deinit = Motor_Deinit,
        .read = Motor_Read,
        .write = Motor_Write,
        .control = Motor_Control,
        .update = Motor_Update
    };
    DeviceManager_Register(ID_MOTOR, "MotorArbitrator", DEVICE_TYPE_MOTOR, 
                           &g_motor_dev, motor_ops);
    // Keil Watch    ?   ?  ?  ?     ?   ?  
    g_pMotorDevWatch = &g_motor_dev;

    // ?         υτ
    motor_hall_config_t motorHallCfg = {
        .hall_a_port = GPIO_PORT_A,
        .hall_a_pin = GPIO_PIN_10,
        .hall_b_port = GPIO_PORT_A,
        .hall_b_pin = GPIO_PIN_09,
        .eirq_ch_a = EXTINT_CH10,
        .eirq_ch_b = EXTINT_CH09,
        .irqn_a = INT010_IRQn,
        .irqn_b = INT009_IRQn,
        .irq_src_a = INT_SRC_PORT_EIRQ10,
        .irq_src_b = INT_SRC_PORT_EIRQ9,
        .irq_priority = 2,
        .pole_pairs = (uint8_t)g_AppParam.motor_hall_pole_pairs,  // ΄ΣFlash¶ΑΘ΅
        .hall_count = 2,
        .custom_pulses_per_rev = 0
    };

    MotorHall_DeviceConfig_t motorHallDevCfg = {
        .motor_id = 0,
        .update_interval_ms = 1
    };

        g_motor_hall_dev = MotorHall_Device_Create(&motorHallCfg, &motorHallDevCfg);

    MAIN_D("Before Register: motor_hall_dev=0x%p\r\n", g_motor_hall_dev);
    MAIN_D("Before Register: motor_hall_dev->config.hall_a_pin=0x%04X\r\n", g_motor_hall_dev->config.hall_a_pin);

    DeviceManager_Register(ID_MOTOR_HALL, "MotorHall", DEVICE_TYPE_HALL, 
                        g_motor_hall_dev, g_motor_hall_ops);

    // ?         ?
    DeviceNode_t* check_node = DeviceManager_Get(ID_MOTOR_HALL);
    if (check_node && check_node->private_data) {
        MotorHall_Device_t* check_dev = (MotorHall_Device_t*)check_node->private_data;
        MAIN_D("After Register: check_dev=0x%p\r\n", check_dev);
        MAIN_D("After Register: check_dev->config.hall_a_pin=0x%04X\r\n", check_dev->config.hall_a_pin);
    }                    

    // ============================================================
    // ADC  υτ?  
    // ? ?ADC  ? ?    ?     dev_adc.c    ADC_AdpLayerInit()  ?    
    //   ?   ADC  υτ  ?  ?   ?      Adc_Create/Adc_Init/Adc_Start
    // ============================================================

    // ?        ADC υτ (PA5, CH5) -  §Ψ ??
    ADC_Config_t adcCurrentCfg = {
        .u8AdcId = 0,                       //    ADC_AdpLayerInit  ?     
        .u8Channel = 5,                     // ADC_CH5 = PA5
        .u8Port = GPIO_PORT_A,
        .u16Pin = GPIO_PIN_05,
        .enAcqMode = ADC_ACQ_MODE_INTERRUPT, //  §Ψ ??
        .u16DmaBufferSize = 0,              //  §Ψ ??    ?DMA      
        .u8DmaChannel = 0                   //  §Ψ ??    ?DMA?  
    };

    ADC_Device_t* adc_current_dev = ADC_Device_Create(&adcCurrentCfg);
    DeviceManager_Register(ID_ADC_CURRENT, "ADC_Current", DEVICE_TYPE_ADC, 
                        adc_current_dev, g_adc_ops);

    // ?   ?   ADC υτ
    //   HB_chuchai ??   ? PA04, CH4
    //      ?‰Ν   HandB ? ΅κ   PA06, CH6
    //   ?  ? ?? ?? PA04 + CH4
    ADC_Config_t adcVoltageCfg = {
        .u8AdcId = 1,                       //    ADC_AdpLayerInit  ?     
        .u8Channel = PIN_ADC_VOLTAGE_CH,                     // ADC_CH6 = PA06 (   ? ); ? HB_chuchai: CH4 = PA04
        .u8Port = PIN_ADC_VOLTAGE_PORT,
        .u16Pin = PIN_ADC_VOLTAGE_PIN,              // PA06 (   ? ); ? HB_chuchai: GPIO_PIN_04
        .enAcqMode = ADC_ACQ_MODE_INTERRUPT, //  §Ψ ??
        .u16DmaBufferSize = 0,              //  §Ψ ??    ?DMA      
        .u8DmaChannel = 0                   //  §Ψ ??    ?DMA?  
    };

    ADC_Device_t* adc_voltage_dev = ADC_Device_Create(&adcVoltageCfg);
    DeviceManager_Register(ID_ADC_VOLTAGE, "ADC_Voltage", DEVICE_TYPE_ADC, 
                        adc_voltage_dev, g_adc_ops);

    // ?   ??   υτ      ADC_VOLTAGE    ?  ? ? ?  
    //  Σ  ?   g_AppParam   ?  ?   Flash    ? 
    {
        //    Flash   ?  ?   ?   ¦Λ?    0.1V -> mV
        uint32_t u32OvervoltageThresholdMv = (uint32_t)g_AppParam.voltage_upper_limit * 100UL;
        uint32_t u32UndervoltageThresholdMv = (uint32_t)g_AppParam.voltage_lower_limit * 100UL;
        uint32_t u32OvervoltageHysteresisMv = (uint32_t)g_AppParam.voltage_upper_hysteresis * 100UL;
        uint32_t u32UndervoltageHysteresisMv = (uint32_t)g_AppParam.voltage_lower_hysteresis * 100UL;
        uint8_t u8OvervoltageTriggerCount = g_AppParam.overvoltage_trigger_count;
        uint8_t u8UndervoltageTriggerCount = g_AppParam.undervoltage_trigger_count;
        
        //     Flash  §Φ ?? 0     ? ?  ?    ? υτ            
        if (u32OvervoltageThresholdMv == 0) {
            u32OvervoltageThresholdMv = PARAM_DEFAULT_VOLTAGE_UPPER_LIMIT * 100UL;
            PARAMS_DBG("[VOLTAGE] Overvoltage threshold is 0, using default: %lu mV", u32OvervoltageThresholdMv);
        }
        if (u32UndervoltageThresholdMv == 0) {
            u32UndervoltageThresholdMv = PARAM_DEFAULT_VOLTAGE_LOWER_LIMIT * 100UL;
            PARAMS_DBG("[VOLTAGE] Undervoltage threshold is 0, using default: %lu mV", u32UndervoltageThresholdMv);
        }
        if (u32OvervoltageHysteresisMv == 0) {
            u32OvervoltageHysteresisMv = PARAM_DEFAULT_VOLTAGE_UPPER_HYSTERESIS * 100UL;
        }
        if (u32UndervoltageHysteresisMv == 0) {
            u32UndervoltageHysteresisMv = PARAM_DEFAULT_VOLTAGE_LOWER_HYSTERESIS * 100UL;
        }
        if (u8OvervoltageTriggerCount == 0) {
            u8OvervoltageTriggerCount = PARAM_DEFAULT_OVERVOLTAGE_TRIGGER_CNT;
        }
        if (u8UndervoltageTriggerCount == 0) {
            u8UndervoltageTriggerCount = PARAM_DEFAULT_UNDERVOLTAGE_TRIGGER_CNT;
        }
        
        MAIN_D("[APP] Voltage config from Flash: upper=%lu mV (0.1V=%d), lower=%lu mV (0.1V=%d), "
            "upper_hys=%lu mV, lower_hys=%lu mV, ov_cnt=%d, uv_cnt=%d",
            u32OvervoltageThresholdMv, g_AppParam.voltage_upper_limit,
            u32UndervoltageThresholdMv, g_AppParam.voltage_lower_limit,
            u32OvervoltageHysteresisMv, u32UndervoltageHysteresisMv,
            u8OvervoltageTriggerCount, u8UndervoltageTriggerCount);
        
        Voltage_Config_t voltageBusCfg = {
            .u8AdcDevId = ID_ADC_VOLTAGE,
            
            //   ?       
            .u32OvervoltageThresholdMv = u32OvervoltageThresholdMv,
            .u32OvervoltageHysteresisMv = u32OvervoltageHysteresisMv,
            .u8OvervoltageTriggerCount = u8OvervoltageTriggerCount,
            
            // ??       
            .u32UndervoltageThresholdMv = u32UndervoltageThresholdMv,
            .u32UndervoltageHysteresisMv = u32UndervoltageHysteresisMv,
            .u8UndervoltageTriggerCount = u8UndervoltageTriggerCount,
        };
        g_voltage_bus_dev = Voltage_Device_Create(&voltageBusCfg);
        DeviceManager_Register(ID_VOLTAGE_BUS, "VoltageBus", DEVICE_TYPE_SENSOR,
                            g_voltage_bus_dev, g_voltage_ops);
    }

    // ?            υτ
    //    Modbus  ?     ?      ? ? ? ? ?      ?  ?    
    {
        int32_t s32ThresholdMa = (int32_t)g_AppParam.current_upper_limit;
        uint32_t u32TriggerMs = (uint32_t)g_AppParam.current_detect_ms;
        uint32_t u32ReleaseMs = (uint32_t)g_AppParam.current_release_ms;
        int32_t s32HysteresisMa = (int32_t)g_AppParam.current_hysteresis_ma;
        uint8_t u8TriggerCount = g_AppParam.overcurrent_trigger_count;
        
        //     ?   ?? 0  ¦Δ   ?      ? ?  ?
        // ? ?0     §Ή?     ?  ?             2 ?  ? 0
        //     Flash ¦Δ  ?    ? 0xFF    g_AppParam.current_upper_limit      0xFFFFFFFF  int32_t    -1  
        //          §Ψ  < 0   ?¦Δ  ?    = 0   ? ?       ? 0
        if (s32ThresholdMa < 0) {
            s32ThresholdMa = PARAM_DEFAULT_CURRENT_UPPER_LIMIT;
            PARAMS_DBG("[CURRENT] Threshold is invalid (%ld), using default: %ld mA", 
                       (long)s32ThresholdMa, (long)PARAM_DEFAULT_CURRENT_UPPER_LIMIT);
        }
        if (u32TriggerMs == 0) {
            u32TriggerMs = PARAM_DEFAULT_CURRENT_DETECT_MS;
        }
        if (u32ReleaseMs == 0) {
            u32ReleaseMs = PARAM_DEFAULT_CURRENT_RELEASE_MS;
        }
        if (s32HysteresisMa == 0) {
            s32HysteresisMa = PARAM_DEFAULT_CURRENT_HYSTERESIS_MA;
        }
        if (u8TriggerCount == 0) {
            u8TriggerCount = PARAM_DEFAULT_OVERCURRENT_TRIGGER_CNT;
        }
        
        MAIN_D("[APP] Sensor config from Flash: threshold=%ldmA, trigger_window=%ldms, release_window=%ldms, hysteresis=%ldmA, trigger_cnt=%d",
            (long)s32ThresholdMa, (long)u32TriggerMs, (long)u32ReleaseMs, (long)s32HysteresisMa, (int)u8TriggerCount);
        
        Sensor_Config_t sensorCurrentCfg = {
            .u8AdcDevId = ID_ADC_CURRENT,
            
            .s32OvercurrentThresholdMa = s32ThresholdMa,
            .s32OvercurrentHysteresisMa = s32HysteresisMa,
            
            .u8OvercurrentMode = OVERCURRENT_MODE_TIME_WINDOW,  //     ? ? ?  ?  ??
            
            //     ??   ? ?  ?? 2 ? ? 
            .u16TriggerWindowSize = 0,
            .u16ReleaseWindowSize = 0,
            
            // ?  ??    
            .u32TriggerWindowMs = u32TriggerMs,
            .u32ReleaseWindowMs = u32ReleaseMs,
        };
        g_sensor_current_dev = Sensor_Device_Create(&sensorCurrentCfg);
        DeviceManager_Register(ID_SENSOR_CURRENT, "SensorCurrent", DEVICE_TYPE_SENSOR,
                            g_sensor_current_dev, g_sensor_ops);
    }

    // ?  ?  ?       υτ
    RTurn_Config_t rturnCfg = {
        .u8MotorHallDevId = ID_MOTOR_HALL,                      //  ??       υτ
        .u8MotorArbiterDevId = ID_MOTOR,                        //        ??   ?             
        .u8SensorDevId = ID_SENSOR_CURRENT,                     //  ??          υτ
        .fReductionRatio = (float)g_AppParam.rturn_reduction_ratio / 10.0f,  //   Flash  ?    ? 
        .fMaxAngle = RTURN_MAX_ANGLE,                           //    ? 
        .fMinAngle = RTURN_MIN_ANGLE,                           //   §³ ? 
        .u8ReverseOutput = RTURN_REVERSE_OUTPUT,                //           ?
        .u8DeviceId = ID_RTURN,
        .u16UpdateIntervalMs = RTURN_UPDATE_INTERVAL_MS         //    ?  
    };

    // RTurn_Device_t* rturn_dev = RTurn_Device_Create(&rturnCfg);
    g_rturn_dev = RTurn_Device_Create(&rturnCfg);
    DeviceManager_Register(ID_RTURN, "RTurn", DEVICE_TYPE_SENSOR, 
                        g_rturn_dev, g_rturn_ops);


                        
}

// ========== EventBus ???????? ==========
static void SetupEventBusSubscriptions(void) {
    //     υτ   ?    ??  ?     ? 0       ?     ?      
    EventBus_Subscribe(TOPIC_POWER, Motor_OnPowerEvent, 0);
    EventBus_Subscribe(TOPIC_LIMIT_HARD, Motor_OnHardLimit, 0);
    EventBus_Subscribe(TOPIC_LIMIT_SOFT, Motor_OnHardLimit, 0);
    EventBus_Subscribe(TOPIC_MANUAL_IO, Motor_OnManualIO, 0);
    EventBus_Subscribe(TOPIC_MOTOR_SPEED_FEEDBACK, Motor_OnSpeedFeedback, 0);    
    //        υτ   ?    ? 
    EventBus_Subscribe(TOPIC_ALARM, Motor_OnOvercurrent, 0);
    EventBus_Subscribe(TOPIC_VOLTAGE_ALARM, Motor_OnVoltageAlarm, 0);
    // ?  ?         ?    ? 
    EventBus_Subscribe(TOPIC_CURRENT_ALARM, RTurn_OnCurrentAlarm, 1);	
    EventBus_Subscribe(TOPIC_CURRENT_ALARM, Motor_OnCurrentAlarm, 1);

    //     ?       ?  ?        ¦Λ ? 
    EventBus_Subscribe(TOPIC_RTURN_LIMIT, Motor_OnRTurnLimit, 0);
    // RS485  ?    ? ?   Modbus REG_CTRL_CMD       
    EventBus_Subscribe(TOPIC_MANUAL_RS485, Motor_OnManualIO, 0);
}

// ==========       ??  D??  ?  ???? ==========
static void SetDeviceUpdateIntervals(void) {
    DeviceManager_SetUpdateInterval(ID_PWR_POS, 1);
    DeviceManager_SetUpdateInterval(ID_PWR_NEG, 1);
    // ????      ? -     ???a0  ???  ????  D? (  ?  )
    // DeviceManager_SetUpdateInterval(ID_HALL_UP, 1);    //   ??T??
    // DeviceManager_SetUpdateInterval(ID_HALL_DOWN, 1);  // ???T??
    // DeviceManager_SetUpdateInterval(ID_IO_FWD, 1000);
    // DeviceManager_SetUpdateInterval(ID_IO_REV, 1000);
    // DeviceManager_SetUpdateInterval(ID_PWM_MOTOR, 10000);
    DeviceManager_SetUpdateInterval(ID_MOTOR, 1);
    DeviceManager_SetUpdateInterval(ID_MOTOR_HALL, 1);  // ?10ms    
    //    SetDeviceUpdateIntervals           
    DeviceManager_SetUpdateInterval(ID_ADC_CURRENT, 1);   // ADC        
    DeviceManager_SetUpdateInterval(ID_ADC_VOLTAGE, 1);   // ADC  ?    
    
    //   ?? ??          υτ - ?10ms  ADC  ?      ?  
    DeviceManager_SetUpdateInterval(ID_VOLTAGE_BUS, 10);   // ?10ms    ?  ? ? ?
    DeviceManager_SetUpdateInterval(ID_SENSOR_CURRENT, 1); // ?10ms    ? ¦Ε   ?
    DeviceManager_SetUpdateInterval(ID_RTURN, 1);  // ?10ms    ? ¦Ν? 
}

// ========== ?  ?a? ? |     ==========
#if ENABLE_SIMULATION_MODE

void Sim_ProcessInput(void) {
    static uint32_t last_update = 0;
    uint32_t now = tickTimer_GetCount();
    
    if (now - last_update < 100) return;
    last_update = now;
}

void Sim_PublishEvents(void) {
    uint32_t now = tickTimer_GetCount();
    (void)now;  //     ¦Δ? t   
    
    //   ? ? 
    static uint8_t last_pwr_pos = 0;
    static uint8_t last_pwr_neg = 0;
    
    if (g_sim.sim_pwr_pos != last_pwr_pos) {
        MotorPowerEvent_t pwrEvent = {.power_id = 0, .is_on = (g_sim.sim_pwr_pos != 0)};
        EventBus_Publish(TOPIC_POWER, &pwrEvent);
        MAIN_D("[SIM] POWER_POS changed: %d -> %d\r\n", last_pwr_pos, g_sim.sim_pwr_pos);
        last_pwr_pos = g_sim.sim_pwr_pos;
    }
    
    if (g_sim.sim_pwr_neg != last_pwr_neg) {
        MotorPowerEvent_t pwrEvent = {.power_id = 1, .is_on = (g_sim.sim_pwr_neg != 0)};
        EventBus_Publish(TOPIC_POWER, &pwrEvent);
        MAIN_D("[SIM] POWER_NEG changed: %d -> %d\r\n", last_pwr_neg, g_sim.sim_pwr_neg);
        last_pwr_neg = g_sim.sim_pwr_neg;
    }
    
    //   ¦Λ ? 
    static uint8_t last_hall_up = 0;
    static uint8_t last_hall_down = 0;
    
    if (g_sim.sim_hall_up != last_hall_up) {
        MotorLimitEvent_t limitEvent = {.dir = DIR_FWD, .is_active = (g_sim.sim_hall_up != 0)};
        EventBus_Publish(TOPIC_LIMIT_HARD, &limitEvent);
        MAIN_D("[SIM] HALL_UP changed: %d -> %d\r\n", last_hall_up, g_sim.sim_hall_up);
        last_hall_up = g_sim.sim_hall_up;
    }
    
    if (g_sim.sim_hall_down != last_hall_down) {
        MotorLimitEvent_t limitEvent = {.dir = DIR_REV, .is_active = (g_sim.sim_hall_down != 0)};
        EventBus_Publish(TOPIC_LIMIT_HARD, &limitEvent);
        MAIN_D("[SIM] HALL_DOWN changed: %d -> %d\r\n", last_hall_down, g_sim.sim_hall_down);
        last_hall_down = g_sim.sim_hall_down;
    }
    
    // IO ?  - ?   §Ò£?  ?
    static uint8_t last_io_fwd = 0;
    static uint8_t last_io_rev = 0;
    
    //   ?IO £   
    if (g_sim.sim_io_fwd != last_io_fwd) {
        MAIN_D("[SIM] IO_FWD changed: %d -> %d\r\n", last_io_fwd, g_sim.sim_io_fwd);
        
        MotorManualIOEvent_t ioEvent;
        memset(&ioEvent, 0, sizeof(MotorManualIOEvent_t));  //   ?       ? 
        
        if (g_sim.sim_io_fwd) {
            ioEvent.dir = DIR_FWD;
            ioEvent.type = CMD_TYPE_RUN_FWD;
            ioEvent.speed = g_sim.sim_io_speed;
        } else {
            ioEvent.dir = DIR_FWD;
            ioEvent.type = CMD_TYPE_STOP;
            ioEvent.speed = 0.0f;
        }
        
        //   ?       ?     
        int32_t speed_int = (int32_t)(ioEvent.speed * 10);
        MAIN_D("[SIM] Publishing IO_FWD event: dir=%d, type=%d, speed=%ld.%ld%%\r\n",
               ioEvent.dir, ioEvent.type, (long)(speed_int / 10), (long)(speed_int % 10));
        
        EventBus_Publish(TOPIC_MANUAL_IO, &ioEvent);
        last_io_fwd = g_sim.sim_io_fwd;
    }
    
    //   ?IO £   
    if (g_sim.sim_io_rev != last_io_rev) {
        MAIN_D("[SIM] IO_REV changed: %d -> %d\r\n", last_io_rev, g_sim.sim_io_rev);
        
        MotorManualIOEvent_t ioEvent;
        memset(&ioEvent, 0, sizeof(MotorManualIOEvent_t));  //   ?       ? 
        
        if (g_sim.sim_io_rev) {
            ioEvent.dir = DIR_REV;
            ioEvent.type = CMD_TYPE_RUN_REV;
            ioEvent.speed = g_sim.sim_io_speed;
        } else {
            ioEvent.dir = DIR_REV;
            ioEvent.type = CMD_TYPE_STOP;
            ioEvent.speed = 0.0f;
        }
        
        //   ?       ?     
        int32_t speed_int = (int32_t)(ioEvent.speed * 10);
        MAIN_D("[SIM] Publishing IO_REV event: dir=%d, type=%d, speed=%ld.%ld%%\r\n",
               ioEvent.dir, ioEvent.type, (long)(speed_int / 10), (long)(speed_int % 10));
        
        EventBus_Publish(TOPIC_MANUAL_IO, &ioEvent);
        last_io_rev = g_sim.sim_io_rev;
    }
}

#endif

// ==========    ???  D?o    y ==========

static void UpdateStatusIndicators(void) {
    static uint32_t last_update = 0;
    uint32_t now = tickTimer_GetCount();
    
    if (now - last_update < 50) return;
    last_update = now;
    
    // ??  ?  ??   ?΅§ ?D??  
    const MotorDebugInfo_t* dbg = Motor_GetDebugInfo(&g_motor_dev);
    if (dbg) {
        if (dbg->state == MS_IDLE) {
            g_status.motor_status = MOTOR_STOPPED;
        } else if (dbg->active_dir == DIR_FWD) {
            g_status.motor_status = MOTOR_FORWARD;
        } else if (dbg->active_dir == DIR_REV) {
            g_status.motor_status = MOTOR_REVERSE;
        }
        g_status.current_duty = dbg->current_duty;
    }
    
    // ?  D?  ??    ??
    if (g_sim.sim_pwr_pos && g_sim.sim_pwr_neg) {
        g_status.power_status = POWER_BOTH_ON;
    } else if (g_sim.sim_pwr_pos) {
        g_status.power_status = POWER_POS_ON;
    } else if (g_sim.sim_pwr_neg) {
        g_status.power_status = POWER_NEG_ON;
    } else {
        g_status.power_status = POWER_BOTH_OFF;
    }
    
    // ?  D?????   ??
    if (g_sim.sim_hall_up && g_sim.sim_hall_down) {
        g_status.hall_status = HALL_BOTH_LIMIT;
    } else if (g_sim.sim_hall_up) {
        g_status.hall_status = HALL_UP_LIMIT;
    } else if (g_sim.sim_hall_down) {
        g_status.hall_status = HALL_DOWN_LIMIT;
    } else {
        g_status.hall_status = HALL_NO_LIMIT;
    }
    
    g_status.io_status = (g_sim.sim_io_fwd ? 1 : (g_sim.sim_io_rev ? 2 : 0));
}

static void ProcessDeviceUpdates(void) {
    DeviceManager_UpdateAll();
}

// ========== ? ? 33?  ??   ==========
void ESystem_Init(void) {
    // 1.  ? ?   EventBus  ?  ?  ? ¦² 
    EventBus_Init();
    
    // 2.   ?   DeviceManager       ?   ?   EventBus  
    DeviceManagerConfig_t config = {
        .operation_timeout_ms = 500,
        .enable_mutex = 1,
        .auto_subscribe = 0
    };
    DeviceManager_Init(&config);
    
    // 3. ?       υτ
    RegisterAllDevices();
    
    // 4.        ? ?    ?    ? ? υτ  ?  ??  ?   
    SetupEventBusSubscriptions();
    

    
    // 6.  ??¦Γ ?   υτ?     ? ?      ?      ? ?    ?  
    //  ? ?      ?   
    MAIN_D("[APP] Initializing motor arbiter first...\r\n");
    Device_Init(ID_MOTOR);
    
    //  ? ?       υτ        ?  
    MAIN_D("[APP] Initializing remaining devices...\r\n");
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        if (i == ID_MOTOR) continue;  //  ? ?  
        Device_Init(i);
    }
    
    // 6.      υτ    ?  
    SetDeviceUpdateIntervals();
    
    // 7.          υτ    
    DeviceManager_EnableAllUpdate();
    
    memset(&g_status, 0, sizeof(SystemStatus_t));
}

// ========== ? ? 3?  ?-?   ==========
// ==========          ?          ¦Λ   ==========
/**
 * @brief    g_AppParam       ?   ?      υτ?  
 *        Modbus §Υ       ¨²   ?           ¦Λ
 */
void App_ReloadConfig(void)
{
    MAIN_D("[RELOAD] Reloading config from g_AppParam...\r\n");

    /* ---   ??   υτ     1 ?/??  ? --- */
    if (g_voltage_bus_dev != NULL)
    {
        g_voltage_bus_dev->stcConfig.u32OvervoltageThresholdMv =
            (uint32_t)g_AppParam.voltage_upper_limit * 100UL;
        g_voltage_bus_dev->stcConfig.u32UndervoltageThresholdMv =
            (uint32_t)g_AppParam.voltage_lower_limit * 100UL;
        g_voltage_bus_dev->stcConfig.u32OvervoltageHysteresisMv =
            (uint32_t)g_AppParam.voltage_upper_hysteresis * 100UL;
        g_voltage_bus_dev->stcConfig.u32UndervoltageHysteresisMv =
            (uint32_t)g_AppParam.voltage_lower_hysteresis * 100UL;
        g_voltage_bus_dev->stcConfig.u8OvervoltageTriggerCount =
            g_AppParam.overvoltage_trigger_count;
        g_voltage_bus_dev->stcConfig.u8UndervoltageTriggerCount =
            g_AppParam.undervoltage_trigger_count;

        MAIN_D("[RELOAD] Voltage: over=%lu mV, under=%lu mV\r\n",
               g_voltage_bus_dev->stcConfig.u32OvervoltageThresholdMv,
               g_voltage_bus_dev->stcConfig.u32UndervoltageThresholdMv);
    }

    /* ---            υτ     1     ? --- */
    if (g_sensor_current_dev != NULL)
    {
        g_sensor_current_dev->stcConfig.s32OvercurrentThresholdMa =
            (int32_t)g_AppParam.current_upper_limit;
        g_sensor_current_dev->stcConfig.u32TriggerWindowMs =
            (uint32_t)g_AppParam.current_detect_ms;
        g_sensor_current_dev->stcConfig.u32ReleaseWindowMs =
            (uint32_t)g_AppParam.current_release_ms;
        g_sensor_current_dev->stcConfig.s32OvercurrentHysteresisMa =
            (int32_t)g_AppParam.current_hysteresis_ma;

        MAIN_D("[RELOAD] Current: threshold=%ld mA, trigger=%lu ms, release=%lu ms\r\n",
               (long)g_sensor_current_dev->stcConfig.s32OvercurrentThresholdMa,
               (unsigned long)g_sensor_current_dev->stcConfig.u32TriggerWindowMs,
               (unsigned long)g_sensor_current_dev->stcConfig.u32ReleaseWindowMs);
    }

    /* --- ?  / ? ?  ?? ?  g_AppParam   ?           --- */
    /* ---  υτ  ? (node_id)   §Υ   Flash   ΅δ  Param_Init ?  §Ή --- */


    /* ---    ? :     RTurn υτ --- */
    if (g_rturn_dev != NULL)
    {
        g_rturn_dev->stcConfig.fReductionRatio =
            (float)g_AppParam.rturn_reduction_ratio / 10.0f;
        MAIN_D("[RELOAD] Reduction ratio: %.1f\r\n",
               (double)g_rturn_dev->stcConfig.fReductionRatio);
    }

    /* ---          :     MotorHall υτ --- */
    if (g_motor_hall_dev != NULL && g_motor_hall_dev->handle != NULL)
    {
        motor_hall_set_pole_pairs(g_motor_hall_dev->handle,
                                  (uint8_t)g_AppParam.motor_hall_pole_pairs);
        g_motor_hall_dev->config.pole_pairs =
            (uint8_t)g_AppParam.motor_hall_pole_pairs;
        MAIN_D("[RELOAD] Pole pairs: %d\r\n",
               (int)g_AppParam.motor_hall_pole_pairs);
    }

    MAIN_D("[RELOAD] Config reload complete.\r\n");
}


void ESystem_MainLoop(void) {
    static uint32_t last_loop_time = 0;
    uint32_t now = tickTimer_GetCount();
    
    if (now - last_loop_time < 1) return;
    last_loop_time = now;
    
#if ENABLE_SIMULATION_MODE
    Sim_ProcessInput();
    Sim_PublishEvents();
#endif
    
    ProcessDeviceUpdates();
    UpdateStatusIndicators();
}
