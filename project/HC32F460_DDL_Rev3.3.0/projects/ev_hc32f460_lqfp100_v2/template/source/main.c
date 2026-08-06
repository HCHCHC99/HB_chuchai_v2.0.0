#include "main.h"
#include "Hardware.h"
#include "rtt_log.h"
#include "timer6_timebase.h"
#include "Motor_hall.h"
#include "TickTimer.h"
#include "device_manager.h"
#include "App_Motor_Project.h"
#include "param_manager.h"
#include "Gpio_io.h"
#include "rs485.h"
#include "App_Modbus.h"
#include "App_Params.h"
#include "App_FaultHandler.h"
#include "rtt_manager.h"   // 包含 INTERVAL_DECLARE / INTERVAL_PRINT 宏
#include "Pwm.h"
#include "Template_Pwm.h"
#include "hc32_ll_utility.h"
#include "EventRecorder.h"

/*=============================================================================
 * 全局变量定义
 *============================================================================*/

/* ISR CPU 观测变量（Logic Analyzer / Performance Analyzer 用）：主循环每转一圈 +1 */
volatile uint32_t g_loop_heartbeat = 0U;

/*=============================================================================
 * 全局PWM实例（供dev_motor使用）
 *============================================================================*/
pwm_t g_motor_pwm_ch1;  // PB6
pwm_t g_motor_pwm_ch2;  // PB7
pwm_t g_motor_pwm_ch3;  // PB8
pwm_t g_motor_pwm_ch4;  // PB9

/*=============================================================================
 * 电机PWM初始化（4通道，20kHz，低有效）
 *============================================================================*/
static void Motor_Pwm_Init(void)
{
    // 解锁GPIO寄存器（允许修改GPIO配置）
    LL_PERIPH_WE(LL_PERIPH_GPIO);

    // CH1: PB6 - 低有效
    g_motor_pwm_ch1 = PWM_Init(CM_TMRA_4, FCG2_PERIPH_TMRA_4, TMRA_CH1,
                                GPIO_PORT_B, GPIO_PIN_06, GPIO_FUNC_4,
                                TMRA_MD_SAWTOOTH, TMRA_DIR_UP,
                                6000, 0, PWM_ACTIVE_LOW);

    // CH2: PB7 - 低有效
    g_motor_pwm_ch2 = PWM_Init(CM_TMRA_4, FCG2_PERIPH_TMRA_4, TMRA_CH2,
                                GPIO_PORT_B, GPIO_PIN_07, GPIO_FUNC_4,
                                TMRA_MD_SAWTOOTH, TMRA_DIR_UP,
                                6000, 0, PWM_ACTIVE_LOW);

    // CH3: PB8 - 低有效
    g_motor_pwm_ch3 = PWM_Init(CM_TMRA_4, FCG2_PERIPH_TMRA_4, TMRA_CH3,
                                GPIO_PORT_B, GPIO_PIN_08, GPIO_FUNC_4,
                                TMRA_MD_SAWTOOTH, TMRA_DIR_UP,
                                6000, 0, PWM_ACTIVE_LOW);

    // CH4: PB9 - 低有效
    g_motor_pwm_ch4 = PWM_Init(CM_TMRA_4, FCG2_PERIPH_TMRA_4, TMRA_CH4,
                                GPIO_PORT_B, GPIO_PIN_09, GPIO_FUNC_4,
                                TMRA_MD_SAWTOOTH, TMRA_DIR_UP,
                                6000, 0, PWM_ACTIVE_LOW);

    // 锁定GPIO寄存器（配置完成后锁定）
    LL_PERIPH_WP(LL_PERIPH_GPIO);

    // 解锁FCG寄存器（允许使能定时器时钟）
    LL_PERIPH_WE(LL_PERIPH_FCG);

    // 启动4路PWM定时器
    PWM_Start(&g_motor_pwm_ch1);
    PWM_Start(&g_motor_pwm_ch2);
    PWM_Start(&g_motor_pwm_ch3);
    PWM_Start(&g_motor_pwm_ch4);

    // 使能输出
    PWM_OutputCmd(&g_motor_pwm_ch1, PWM_OUTPUT_ENABLE);
    PWM_OutputCmd(&g_motor_pwm_ch2, PWM_OUTPUT_ENABLE);
    PWM_OutputCmd(&g_motor_pwm_ch3, PWM_OUTPUT_ENABLE);
    PWM_OutputCmd(&g_motor_pwm_ch4, PWM_OUTPUT_ENABLE);

    // 锁定FCG寄存器
    LL_PERIPH_WP(LL_PERIPH_FCG);

    MAIN_D("Motor PWM initialized: 4 channels, 20kHz, low active\r\n");
}

int main(void)
{
    /* Event Recorder 初始化（DAP 变体，J-Link 调试器可读取事件） */
    EventRecorderInitialize(EventRecordAll, 1U);

    Hardware_Init();

    /* RS485 初始化 - 配置 USART 和 GPIO，等待中断处理 */
    RS485_Init();

    /* Modbus 初始化 - 配置协议栈，注册读写回调函数 */
    Modbus_Init();

    ESystem_Init();

    /* 初始化电机PWM（4通道，在电机设备初始化之前） */
    // Motor_Pwm_Init();

    /* 故障处理器初始化 - 注册电压/电流/温度等故障检测回调 */
    FaultHandler_Init();

    /*=========================================================================
     * 电机模式控制变量 - 在 Keil Watch 窗口中可动态修改
     * 0: 停止, 1: 正转, 2: 反转
     *=========================================================================*/
    // volatile uint8_t motor_mode = 0;

    // MotorDevice_t* motor = NULL;       // TODO: 从设备管理器中获取电机设备指针

    EventBus_Enable();

    /* 测试计数器 */
    uint32_t test_counter = 0;

    while (1)
    {

        g_loop_heartbeat++;   /* ISR CPU 观测：主循环心跳，每转一圈 +1 */
        {
            static uint32_t s_mainEvtCnt = 0U;
            if ((++s_mainEvtCnt & 0x3FFu) == 0U) {   /* 每 1024 圈记一次事件，避免刷爆缓冲区 */
                EventRecord2(EventID(EventLevelAPI, 2U, 1U), g_loop_heartbeat, 0U);
            }
        }

         ESystem_MainLoop();

#if MOTOR_CONTROL_MODE == 1
        // 更新4通道PWM状态
        PWM_Update(&g_motor_pwm_ch1);
        PWM_Update(&g_motor_pwm_ch2);
        PWM_Update(&g_motor_pwm_ch3);
        PWM_Update(&g_motor_pwm_ch4);
#endif

        RS485_Poll();
        Modbus_Poll();

        // // 根据电机模式变量控制电机运行状态
        // if (motor_mode == 0) {
        //     Motor_OnArbitrationStop(motor);
        // } else if (motor_mode == 1) {
        //     Motor_OnArbitrationFwd(motor, 0.0f);
        // } else if (motor_mode == 2) {
        //     Motor_OnArbitrationRev(motor, 0.0f);
        // }

    }
}
