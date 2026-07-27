#ifndef DEV_MOTOR_H_
#define DEV_MOTOR_H_

#include "device_manager.h"
#include "EventBus.h"
#include "dev_power.h"
#include "dev_hall.h"
#include "dev_voltage.h"
#include "dev_adc.h"
#include "dev_sensor.h"
#include "dev_rturn.h"
#include "dev_motor_hall.h"
#include <stdint.h>
#include <stdbool.h>
#include "rtt_manager.h"

// ========== 调试宏定义 ==========
// 所有调试输出统一使用 rtt_manager.h 中的宏，通过 DEV_MOTOR 控制开关

#ifdef DEV_MOTOR
    #define MOTOR_DEBUG(fmt, ...)    MAIN_D("[MOTOR_DEBUG] " fmt, ##__VA_ARGS__)
    #define MOTOR_OUT(fmt, ...)      MAIN_D("[MOTOR_OUT] " fmt, ##__VA_ARGS__)
#else
    #define MOTOR_DEBUG(fmt, ...)    ((void)0)
    #define MOTOR_OUT(fmt, ...)      ((void)0)
#endif


// ========== 硬件版本：电机控制模式 ==========
// 在 main.h 中通过 BOARD_VERSION 统一管理
#include "main.h"
#if BOARD_VERSION == 0
    // 原HB_chuchai板：GPIO PB8/PB9 直接控制正转/反转/停止
    #define MOTOR_CONTROL_MODE  0
#else
    // 整合板：4通道 PWM 占空比控制，缓起/缓停
    #define MOTOR_CONTROL_MODE  1
#endif

// ========== 电机设备命令定义 ==========
// 注意：CMD_BASE_MOTOR 在 device_manager.h 中没有预定义，在此定义
#define CMD_BASE_MOTOR              0x9000
#define CMD_MOTOR_STOP              (CMD_BASE_MOTOR + 0x01)
#define CMD_MOTOR_RUN_FWD           (CMD_BASE_MOTOR + 0x02)
#define CMD_MOTOR_RUN_REV           (CMD_BASE_MOTOR + 0x03)
#define CMD_MOTOR_SET_SPEED         (CMD_BASE_MOTOR + 0x04)
#define CMD_MOTOR_EMERGENCY_STOP    (CMD_BASE_MOTOR + 0x05)
// 添加到 dev_motor.h 中的命令定义扩展
#define CMD_MOTOR_GET_DESIRED_DIR   (CMD_BASE_MOTOR + 0x06)   // 获取当前期望方向（仲裁结果）


// ========== 电机设备配置宏 ==========
// 模式切换宏：0=单极性（单电源全桥），1=双极性（双电源全桥）
#ifndef MOTOR_MODE_BIPOLAR
#define MOTOR_MODE_BIPOLAR      0
#endif

// 优先级模式配置：1=IO优先级高, 0=CAN优先级高
#ifndef MOTOR_PRIORITY_IO_HIGH
#define MOTOR_PRIORITY_IO_HIGH  1
#endif

// 设备能力配置：位掩码
#ifndef MOTOR_MANUAL_CAPABILITY
#define MOTOR_MANUAL_CAPABILITY  (CAP_ALLOW | CAP_BLOCK)  // IO设备可以请求运动也可以禁止运动
#endif

#ifndef MOTOR_CAN_CAPABILITY
#define MOTOR_CAN_CAPABILITY     (CAP_ALLOW)  // CAN设备可以请求运动
#endif

// 设备能力位定义
#define CAP_BLOCK      (1 << 0)
#define CAP_ALLOW      (1 << 1)

// ========== 公共枚举定义 ==========
// 电机状态结构体（用于 Device_Read 一次性读取）
typedef enum {
    DIR_NONE = 0,
    DIR_FWD = 1,
    DIR_REV = 2
} MotorDir_t;

typedef enum {
    MODE_NONE = 0,
    MODE_AUTO = 1,
    MODE_REMOTE = 2,
    MODE_MANUAL = 3
} MotorMode_t;

typedef enum {
    MS_IDLE = 0,
    MS_RAMPING = 1,
    MS_RUNNING = 2,
    MS_LOCKED = 3
} MotorState_t;

typedef enum {
    CMD_TYPE_NONE_USE = 255,
    CMD_TYPE_STOP = 1,
    CMD_TYPE_RUN_FWD = 2,
    CMD_TYPE_RUN_REV = 3,
    CMD_TYPE_RAMP_FWD = 4,
    CMD_TYPE_RAMP_REV = 5,
    CMD_TYPE_BLOCK_FWD = 6,
    CMD_TYPE_BLOCK_REV = 7,
    CMD_TYPE_BLOCK_BOTH = 8,
} CmdType_t;

// ========== 设备ID枚举 ==========
typedef enum {
    DEV_ID_NONE             = 255,
    DEV_ID_POWER_POS        = 1,
    DEV_ID_POWER_NEG        = 2,
    DEV_ID_LIMIT_FWD        = 3,
    DEV_ID_LIMIT_REV        = 4,
    DEV_ID_CAN              = 5,
    DEV_ID_IO_FWD           = 6,    // 正转IO设备
    DEV_ID_IO_REV           = 7,    // 反转IO设备
    DEV_ID_EMERGENCY        = 8,
    DEV_ID_RTURN_FWD        = 9,    // 旋转限位-正转限位
    DEV_ID_RTURN_REV        = 10,   // 旋转限位-反转限位
    DEV_ID_OVERVOLTAGE_FWD  = 11,   // 过压-阻塞正转
    DEV_ID_OVERVOLTAGE_REV  = 12,   // 过压-阻塞反转
    DEV_ID_UNDERVOLTAGE_FWD = 13,   // 欠压-阻塞正转
    DEV_ID_UNDERVOLTAGE_REV = 14,   // 欠压-阻塞反转
    DEV_ID_OVERCUR_FWD      = 15,   // 正转过流(开窗) - block_fwd
    DEV_ID_OVERCUR_REV      = 16,   // 反转过流(关窗) - block_rev
    DEV_ID_MAX
} MotorDeviceId_t;

// 优先级枚举
typedef enum {
    PRIO_NONE = 255,
    PRIO_EMERGENCY = 0,
    PRIO_LIMIT = 2,
    PRIO_MANUAL = 3,
    PRIO_CAN = 4,
    PRIO_POWER = 5
} MotorPriority_t;

// ========== 命令结构体 ==========
typedef struct {
    MotorDeviceId_t device_id;
    MotorPriority_t priority;
    CmdType_t type;
    float param;
    uint32_t timestamp;
} MotorControlCommand_t;

typedef struct {
    MotorDir_t desired_dir;     // 当前仲裁器得出的期望方向
    MotorState_t state;         // 电机状态：IDLE/RAMPING/RUNNING等
    MotorDir_t active_dir;      // 当前活动方向
    float current_duty;         // 当前占空比
    uint8_t enable;             // 电机使能状态
} Motor_StateInfo_t;

#define MAX_COMMANDS_PER_DIRECTION 20

typedef struct {
    MotorControlCommand_t commands[MAX_COMMANDS_PER_DIRECTION];
    uint8_t count;
} MotorCommandList_t;

// ========== 调试信息结构体 ==========
typedef struct {
    struct {
        MotorDeviceId_t device_ids[MAX_COMMANDS_PER_DIRECTION];
        uint8_t count;
    } block_fwd;

    struct {
        MotorDeviceId_t device_ids[MAX_COMMANDS_PER_DIRECTION];
        uint8_t count;
    } block_rev;

    struct {
        MotorDeviceId_t device_ids[MAX_COMMANDS_PER_DIRECTION];
        uint8_t priorities[MAX_COMMANDS_PER_DIRECTION];
        uint8_t count;
    } allow_fwd;

    struct {
        MotorDeviceId_t device_ids[MAX_COMMANDS_PER_DIRECTION];
        uint8_t priorities[MAX_COMMANDS_PER_DIRECTION];
        uint8_t count;
    } allow_rev;

    MotorDeviceId_t active_device_id;
    MotorState_t state;
    MotorDir_t active_dir;
    float current_duty;
    bool conflict_fault;
} MotorDebugInfo_t;

// ========== 电机设备结构体 ==========
typedef struct {
    // 仲裁队列
    MotorCommandList_t block_fwd;
    MotorCommandList_t block_rev;
    MotorCommandList_t allow_fwd;
    MotorCommandList_t allow_rev;
    MotorState_t state;
    MotorDir_t active_dir;
    MotorDeviceId_t active_device_id;
    float current_duty;

    // 调试信息
    MotorDebugInfo_t debug_info;

    // 内部状态
    float last_sent_duty;
    uint32_t last_arbitration_time;

    // 设备属性
    uint8_t motor_id;           // 电机ID（用于多电机场景）
    uint8_t enable;             // 电机使能
} MotorDevice_t;

// ========== 事件数据结构（用于EventBus） ==========
typedef struct {
    MotorDir_t dir;
    bool is_active;
} MotorLimitEvent_t;

typedef struct {
    MotorDeviceId_t power_id;
    bool is_on;
} MotorPowerEvent_t;

typedef struct {
    MotorDir_t dir;
    CmdType_t type;
    float speed;
} MotorManualIOEvent_t;

typedef struct {
    MotorDir_t dir;
    CmdType_t type;
    float speed;
} MotorCANEvent_t;

typedef struct {
    uint8_t adc_id;             // ADC设备ID（哪个ADC检测到的）
    uint16_t current_ma;        // 当前电流(mA)
    uint16_t threshold_ma;      // 电流阈值(mA)
    uint32_t duration_ms;       // 持续时间(ms)
} MotorOvercurrentEvent_t;

// ========== 仲裁回调函数（弱定义，用户可重写） ==========
/**
 * @brief 仲裁决策为停止时调用的回调
 * @param motor 电机设备指针
 */
void Motor_OnArbitrationStop(MotorDevice_t* motor);

/**
 * @brief 仲裁决策为正转时调用的回调
 * @param motor 电机设备指针
 * @param duty 当前占空比
 */
void Motor_OnArbitrationFwd(MotorDevice_t* motor, float duty);

/**
 * @brief 仲裁决策为反转时调用的回调
 * @param motor 电机设备指针
 * @param duty 当前占空比
 */
void Motor_OnArbitrationRev(MotorDevice_t* motor, float duty);

// ========== 电机设备接口（符合DeviceManager规范） ==========
// 标准设备操作
DeviceResult_t Motor_Init(void* handle);
DeviceResult_t Motor_Deinit(void* handle);
DeviceResult_t Motor_Read(void* handle, void* data, uint32_t size);
DeviceResult_t Motor_Write(void* handle, const void* data, uint32_t size);
DeviceResult_t Motor_Control(void* handle, DeviceCommandData_t* cmd);
DeviceResult_t Motor_Update(void* handle);  // 定时轮询

// 特定操作接口
void Motor_SetSpeed(MotorDevice_t* motor, float duty);
void Motor_Start(MotorDevice_t* motor, MotorDir_t dir);
void Motor_Stop(MotorDevice_t* motor);
void Motor_EmergencyStop(MotorDevice_t* motor);

// 清空允许队列接口（按方向清空 allow 指令，不影响 block）
void Motor_ClearAllowFwd(MotorDevice_t* motor);
void Motor_ClearAllowRev(MotorDevice_t* motor);

// 调试接口
const MotorDebugInfo_t* Motor_GetDebugInfo(MotorDevice_t* motor);

// ========== EventBus回调函数声明 ==========
void Motor_OnPowerEvent(void* payload);
void Motor_OnHardLimit(void* payload);
void Motor_OnManualIO(void* payload);
void Motor_OnCANEvent(void* payload);
void Motor_OnSpeedFeedback(void* payload);  // 速度反馈（预留）
void Motor_OnOvercurrent(void* payload);
void Motor_OnVoltageAlarm(void* payload);
void Motor_OnCurrentAlarm(void* payload);  // 已禁用
void Motor_OnOvercurrentFwd(void* payload);  // 正转(开窗)过流 → block_fwd
void Motor_OnOvercurrentRev(void* payload);  // 反转(关窗)过流 → block_rev
void Motor_OnRTurnLimit(void* payload);  // 旋转限位
// 获取当前仲裁器期望方向（即当前正在执行的方向）
MotorDir_t Motor_GetDesiredDirection(MotorDevice_t* motor);

// ========== 电压报警手动清除接口 ==========
/**
 * @brief 清除电压报警在仲裁器中设置的 block 指令
 * @param motor 电机设备指针
 * @param u8AlarmType 报警类型：VOLTAGE_ALARM_OVERVOLTAGE 或 VOLTAGE_ALARM_UNDERVOLTAGE
 * @note 仅在 VOLTAGE_CLEAR_MODE == VOLTAGE_CLEAR_MANUAL 时使用
 *       在 App_FaultHandler 收到 TOPIC_FAULT_CLEAR 事件时调用
 */
void Motor_ClearVoltageBlock(MotorDevice_t* motor, uint8_t u8AlarmType);

// ========== 电流报警手动清除接口 ==========
/**
 * @brief 清除电流报警在仲裁器中设置的 block 指令
 * @param motor 电机设备指针
 * @note 在 App_FaultHandler 收到 TOPIC_FAULT_CLEAR 事件时调用
 *       清除过流故障后，需要解除仲裁器中的双向阻塞
 */
void Motor_ClearOvercurrentBlock(MotorDevice_t* motor);

// ========== Keil Watch 调试全局变量 ==========
// 在 Watch 窗口中查看 g_pMotorDevWatch 可观察仲裁器内部状态
extern MotorDevice_t* volatile g_pMotorDevWatch;

#endif /* DEV_MOTOR_H_ */
