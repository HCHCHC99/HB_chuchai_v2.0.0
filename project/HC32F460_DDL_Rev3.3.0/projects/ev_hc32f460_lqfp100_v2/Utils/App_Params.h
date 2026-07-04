#ifndef __APP_PARAMS_H__
#define __APP_PARAMS_H__

#include <stdint.h>
#include <stdbool.h>
#include "rtt_manager.h"

/*=============================================================================
 * 调试宏定义
 * 开关在 rtt_manager.h 中统一管理：APP_PARAMS_DBG
 *============================================================================*/
#ifdef APP_PARAMS_DBG
    #define PARAMS_DBG(fmt, ...)    MAIN_D("[PARAMS] " fmt, ##__VA_ARGS__)
#else
    #define PARAMS_DBG(fmt, ...)    ((void)0)
#endif

/*=============================================================================
 * 客户协议寄存器地址定义
 * 保持寄存器 (Holding Register, 功能码 03/06/10)
 *============================================================================*/

/* --- 配置参数（断电保存，Flash 存储） --- */
#define REG_NODE_ID                 (0x2710U)   /* 设备地址 1~247 (uint16_t) */
#define REG_TARGET_SPEED            (0x2711U)   /* 转速 r/min (int16_t) */
#define REG_TARGET_ANGLE            (0x2712U)   /* 角度 0.1度 (int16_t) */
#define REG_VOLTAGE_UPPER_LIMIT     (0x2714U)   /* 电压上限 0.1V (uint16_t) */
#define REG_VOLTAGE_LOWER_LIMIT     (0x2715U)   /* 电压下限 0.1V (uint16_t) */
#define REG_CURRENT_UPPER_LIMIT     (0x2716U)   /* 电流上限 1mA (uint16_t) */
#define REG_CLOSE_LIMIT_ANGLE       (0x271CU)   /* 关闭极限角度 0.1度 (int16_t) */
#define REG_OPEN_LIMIT_ANGLE        (0x271DU)   /* 打开极限角度 0.1度 (int16_t) */
#define REG_CURRENT_DETECT_MS       (0x271EU)   /* 电流检测时间 1ms (uint16_t) */


/* --- 电机方向配置寄存器 (不在 Flash 保存范围，仅 RAM 生效) --- */
#define REG_MOTOR_HALL_DIR          (0x3710U)   /* 霍尔方向: 0=正常, 1=反转 (uint16_t) */
#define REG_MOTOR_DIR               (0x3711U)   /* 电机方向: 0=正常, 1=反转 (uint16_t) */
#define REG_RTURN_REDUCTION_RATIO    (0x3712U)   /* 减速比 (uint16_t) */
#define REG_MOTOR_HALL_POLE_PAIRS   (0x3713U)   /* 电机极对数 (uint16_t) */
#define REG_MOTOR_HALL_COUNT_LO    (0x3714U)   /* 霍尔脉冲累计低16位 (int32_t累加值) */
#define REG_MOTOR_HALL_COUNT_HI    (0x3715U)   /* 霍尔脉冲累计高16位 */

/* 注意：0x2713, 0x2717-0x271B, 0x271F 为保留地址，不暴露给 Modbus */

/* --- 控制命令（写触发，读回实际状态） --- */
#define REG_CTRL_CMD                (0x2720U)   /* 控制命令寄存器 (uint16_t) */

/* REG_CTRL_CMD (0x2720) 位定义：
 * 写操作：
 *   bit0 = 1: 启动（使能 RS485 控制），必须先发此指令
 *   bit1 = 1: 停止（关闭 RS485 控制）
 *   bit2 = 1: 急停（取消正转/反转，不关闭 RS485 控制）
 *   bit4 = 1: 正转（仅在已启动状态下有效，与 bit5 互斥）
 *   bit5 = 1: 反转（仅在已启动状态下有效，与 bit4 互斥）
 * 读操作：
 *   bit4 = 1: 当前正在正转
 *   bit5 = 1: 当前正在反转
 *   bit4=0 且 bit5=0: 当前停止
 */
#define CTRL_CMD_START              (0x0001U)   /* bit0: 启动指令 */
#define CTRL_CMD_STOP               (0x0002U)   /* bit1: 停止指令 */
#define CTRL_CMD_ESTOP              (0x0004U)   /* bit2: 急停指令 */
#define CTRL_CMD_FWD                (0x0010U)   /* bit4: 正转指令/状态 */
#define CTRL_CMD_REV                (0x0020U)   /* bit5: 反转指令/状态 */

/* --- 实时数据（只读，不存 Flash） --- */
#define REG_REAL_SPEED              (0x2730U)   /* 实时转速 r/min (int16_t) */
#define REG_REAL_ANGLE              (0x2731U)   /* 实时角度 0.1度 (int16_t) */
#define REG_REAL_VOLTAGE            (0x2732U)   /* 电压 0.1V (uint16_t) */
#define REG_REAL_CURRENT            (0x2733U)   /* 电流 1mA (uint16_t) */
#define REG_REAL_DIRECTION          (0x2737U)   /* 实时转向 (int16_t) */

/* --- 实时故障（只读，不存 Flash） --- */
#define REG_FAULT_STATUS            (0x2740U)   /* 故障状态 (uint16_t) */

/*=============================================================================
 * 故障状态位定义 (REG_FAULT_STATUS, 0x2740)
 * 1=故障, 0=正常
 *============================================================================*/
#define FAULT_BIT_OVERVOLTAGE       (0x0001U)   /* bit0: 过压 */
#define FAULT_BIT_OVERCURRENT       (0x0002U)   /* bit1: 过流 */
#define FAULT_BIT_OVERTEMP          (0x0004U)   /* bit2: 过温（保留，当前未使用）*/
#define FAULT_BIT_RESET             (0x0008U)   /* bit3: 复位 */
#define FAULT_BIT_OVERLOAD          (0x0010U)   /* bit4: 过载 */
#define FAULT_BIT_STALL             (0x0020U)   /* bit5: 堵转 */
#define FAULT_BIT_UNDERVOLTAGE      (0x0040U)   /* bit6: 欠压 */

/*=============================================================================
 * Flash 存储参数默认值宏定义
 * 对应 AppParamRecord_t 结构体字段，上电从 Flash 加载
 * 注意：电压滞回值单位 0.1V，电流滞回值单位 mA
 *============================================================================*/

/* --- Modbus 协议参数（保持寄存器，断电保存，暴露给 Modbus） --- */
#define PARAM_DEFAULT_NODE_ID                   (1U)        /* 设备地址 1~247 */
#define PARAM_DEFAULT_TARGET_SPEED              (0)         /* 转速 r/min */
#define PARAM_DEFAULT_TARGET_ANGLE              (0)         /* 角度 0.1度 */
#define PARAM_DEFAULT_VOLTAGE_UPPER_LIMIT       (270U)      /* 电压上限 0.1V (26.0V) */
#define PARAM_DEFAULT_VOLTAGE_LOWER_LIMIT       (210U)      /* 电压下限 0.1V (21.0V) */
#define PARAM_DEFAULT_CURRENT_UPPER_LIMIT       (2300)       /* 电流上限 1mA (5A) */
#define PARAM_DEFAULT_CURRENT_DETECT_MS         (20)        /* 电流检测时间 1ms */
#define PARAM_DEFAULT_MOTOR_HALL_DIR        (1)   /* 霍尔方向 0=正常, 1=反转 */
#define PARAM_DEFAULT_MOTOR_DIR             (0)   /* 电机方向 0=正常, 1=反转 */
#define PARAM_DEFAULT_RTURN_REDUCTION_RATIO   (11830)  /* 减速比 x0.1 */
#define PARAM_DEFAULT_MOTOR_HALL_POLE_PAIRS  (3)       /* 电机极对数 */
#define PARAM_DEFAULT_CLOSE_LIMIT_ANGLE         (-20)       /* 关闭极限角度 0.1度 */
#define PARAM_DEFAULT_OPEN_LIMIT_ANGLE          (880)       /* 打开极限角度 0.1度 */
#define PARAM_DEFAULT_BAUD_RATE                 (9600UL)    /* 默认波特率 */

/* --- 内部参数（不暴露给 Modbus，仅 Flash 存储，上电加载） --- */
#define PARAM_DEFAULT_VOLTAGE_UPPER_HYSTERESIS  (20U)       /* 过压滞回 0.1V (2.0V) */
#define PARAM_DEFAULT_VOLTAGE_LOWER_HYSTERESIS  (20U)       /* 欠压滞回 0.1V (2.0V) */
#define PARAM_DEFAULT_OVERVOLTAGE_TRIGGER_CNT   (1U)        /* 过压触发次数 */
#define PARAM_DEFAULT_UNDERVOLTAGE_TRIGGER_CNT  (1U)        /* 欠压触发次数 */
#define PARAM_DEFAULT_CURRENT_HYSTERESIS_MA     (0U)        /* 电流滞回 1mA (0A) - 改为0，无滞回 */
#define PARAM_DEFAULT_CURRENT_RELEASE_MS        (200U)      /* 电流解除时间 1ms (0.2s) */
#define PARAM_DEFAULT_OVERCURRENT_TRIGGER_CNT   (1U)        /* 过流触发次数（时间模式可选使用）*/

/*=============================================================================
 * 编译时配置宏定义（非 Flash 存储，仅宏定义开关）
 *============================================================================*/

/* --- 调试开关 --- */
#define APP_PARAMS_REALTIME_DBG                 /* 启用实时数据周期性打印（每5秒一次） */
// #define APP_PARAMS_REALTIME_SIMULATE          /* 启用实时数据模拟（每5秒各参数+1） */
#define APP_PARAMS_SIM_DBG                      /* 启用模拟实时数据调试打印 */

/* --- 实时数据来源选择 --- */
/* 注释掉此行则使用模拟实时数据（SimRealtimeData，Keil Debug 在线修改）
 * 取消注释则从实际的 dev_rturn 设备读取实时数据 */
#define APP_PARAMS_USE_DEV_RTURN

/*=============================================================================
 * 核心参数记录结构体 (客户 Modbus 协议定义)
 * 注意：结构体布局必须与 Flash 中存储的布局一致
 *       head_magic 和 tail_magic 用于 Flash 块识别
 *       checksum 用于数据完整性校验
 *============================================================================*/
#pragma pack(4)
typedef struct {
    /* 头部信息 */
    uint32_t head_magic;        /* 头部魔数 (0x55AA55AA) */
    uint32_t sequence_id;       /* 序列号，每次修改递增 */
    uint32_t erase_count;       /* Flash擦除次数记录 */

    /* 客户 Modbus 协议参数 (保持寄存器，断电保存，暴露给 Modbus) */
    uint16_t node_id;               /* 0x2710: 设备地址 1~247 */
    int16_t  target_speed;          /* 0x2711: 转速 (r/min) */
    int16_t  target_angle;          /* 0x2712: 角度 (0.1度) */
    uint16_t voltage_upper_limit;   /* 0x2714: 电压上限 (0.1V) */
    uint16_t voltage_lower_limit;   /* 0x2715: 电压下限 (0.1V) */
    uint16_t current_upper_limit;   /* 0x2716: 电流上限 (1mA) */
    uint16_t current_detect_ms;     /* 0x271E: 电流检测时间 (1ms) */
    int16_t  close_limit_angle;     /* 0x271C: 关闭极限角度 (0.1度) */
    int16_t  open_limit_angle;      /* 0x271D: 打开极限角度 (0.1度) */
    uint32_t baud_rate;             /* 波特率 (内部参数，不上Modbus) */

    /* 内部参数（不暴露给 Modbus，仅 Flash 存储，上电加载） */
    uint16_t voltage_upper_hysteresis;   /* 过压滞回 (0.1V) */
    uint16_t voltage_lower_hysteresis;   /* 欠压滞回 (0.1V) */
    uint8_t  overvoltage_trigger_count;  /* 过压触发次数 */
    uint8_t  undervoltage_trigger_count; /* 欠压触发次数 */
    uint16_t current_hysteresis_ma;      /* 电流滞回 (mA) */
    uint16_t current_release_ms;         /* 电流解除时间窗口 (ms) */
    uint8_t  overcurrent_trigger_count;  /* 过流触发次数（时间模式可选使用）*/
    uint16_t  motor_hall_dir;               /* 0x3710: 霍尔方向 0=正常 1=反转 */
    uint16_t  motor_dir;                    /* 0x3711: 电机方向 0=正常 1=反转 */
    uint16_t  rturn_reduction_ratio;          /* 0x3712: 减速比x10 */
    uint16_t  motor_hall_pole_pairs;          /* 0x3713: 电机极对数 */
    uint8_t  reserved[3];                /* 保留字节，对齐到4字节边界 */

    /* 尾部信息 */
    uint32_t checksum;              /* CRC32校验和 */
    uint32_t tail_magic;            /* 尾部魔数 (0xAA44AA44) */
} AppParamRecord_t;
#pragma pack()

/*=============================================================================
 * 实时数据结构体（不存 Flash，仅运行时使用）
 *============================================================================*/
typedef struct {
    int16_t  real_speed;        /* 0x2730: 实时转速 (r/min) */
    int16_t  real_angle;        /* 0x2731: 实时角度 (0.1度) */
    uint16_t real_voltage;      /* 0x2732: 电压 (0.1V) */
    uint16_t real_current;      /* 0x2733: 电流 (1mA) */
    int16_t  real_direction;    /* 0x2737: 实时转向 */
    uint16_t fault_status;      /* 0x2740: 故障状态 (按 FAULT_BIT_xxx 定义) */
} AppRealTimeData_t;

/*=============================================================================
 * 模拟实时数据结构体（用于 Keil Debug 在线修改）
 *============================================================================*/
typedef struct {
    volatile int16_t  speed;        /* 实时转速 r/min */
    volatile int16_t  angle;        /* 实时角度 0.1度 */
    volatile uint16_t voltage;      /* 电压 0.1V */
    volatile uint16_t current;      /* 电流 1mA */
    volatile int16_t  direction;    /* 实时转向 */
    volatile uint16_t fault_set;    /* 故障设置：写入 FAULT_BIT_xxx 会设置对应故障位 */
    volatile uint16_t fault_clear;  /* 故障清除：写入 FAULT_BIT_xxx 会清除对应故障位 */
} SimRealtimeData_t;

/*=============================================================================
 * 全局变量声明
 *============================================================================*/
extern AppParamRecord_t g_AppParam;         /* 应用程序参数（Flash 存储） */
extern AppRealTimeData_t g_RealTimeData;    /* 实时数据（不存 Flash） */
extern volatile int32_t g_s32HallPulseAccum;  /* 霍尔脉冲累积值 */
extern SimRealtimeData_t g_SimRealtimeData; /* 模拟实时数据（Keil Debug 修改） */

/*=============================================================================
 * 参数读写接口函数声明
 *============================================================================*/

/**
 * @brief  根据寄存器地址读取参数值
 * @param  regAddr  寄存器地址 (REG_xxx)
 * @param  pValue   输出参数值指针
 * @return 0 成功，-3 无效地址
 */
int32_t Param_ReadByReg(uint16_t regAddr, uint16_t *pValue);

/**
 * @brief  根据寄存器地址写入参数值
 * @param  regAddr  寄存器地址 (REG_xxx)
 * @param  value    要写入的值
 * @return 0 成功，-3 无效地址
 */
int32_t Param_WriteByReg(uint16_t regAddr, uint16_t value);

/*=============================================================================
 * 实时数据操作函数声明
 *============================================================================*/

/**
 * @brief  设置故障位（按位或）
 * @param  bitMask  故障位掩码 (FAULT_BIT_xxx)
 */
void RealTime_SetFault(uint16_t bitMask);

/**
 * @brief  清除故障位（按位与取反）
 * @param  bitMask  故障位掩码 (FAULT_BIT_xxx)
 */
void RealTime_ClearFault(uint16_t bitMask);

/**
 * @brief  检查故障位
 * @param  bitMask  故障位掩码 (FAULT_BIT_xxx)
 * @retval true     该故障位为 1
 * @retval false    该故障位为 0
 */
bool RealTime_CheckFault(uint16_t bitMask);

/**
 * @brief  更新实时数据（供其他设备模块调用）
 * @param  speed      实时转速 (r/min)，传 NULL 表示不更新
 * @param  angle      实时角度 (0.1度)，传 NULL 表示不更新
 * @param  voltage    电压 (0.1V)，传 NULL 表示不更新
 * @param  current    电流 (1mA)，传 NULL 表示不更新
 * @param  direction  实时转向，传 NULL 表示不更新
 */
void RealTime_Update(const int16_t  *speed,
                     const int16_t  *angle,
                     const uint16_t *voltage,
                     const uint16_t *current,
                     const int16_t  *direction);

/**
 * @brief  从设备读取实时数据并更新到 g_RealTimeData
 *         仅在 APP_PARAMS_USE_DEV_RTURN 宏开启时有效
 * @param  u8RTurnDevId   dev_rturn 设备 ID
 * @param  u8VoltageDevId dev_voltage 设备 ID
 * @param  u8SensorDevId  dev_sensor 设备 ID
 */
void RealTime_UpdateFromDevice(uint8_t u8RTurnDevId,
                               uint8_t u8VoltageDevId,
                               uint8_t u8SensorDevId);

/**
 * @brief  打印实时数据（调试用）
 */
void RealTime_PrintDebug(void);

/**
 * @brief  实时数据模拟（每5秒各参数+1，用于测试上位机读取）
 *         仅在 APP_PARAMS_REALTIME_SIMULATE 宏开启时有效
 */
void RealTime_Simulate(void);

/*=============================================================================
 * 模拟实时数据操作函数声明（用于 Keil Debug 在线修改）
 *============================================================================*/

/**
 * @brief  初始化模拟实时数据
 */
void SimRealtime_Init(void);

/**
 * @brief  同步模拟数据到实际实时数据（在主循环中调用）
 *         将 g_SimRealtimeData 中的值同步到 g_RealTimeData
 */
void SimRealtime_Sync(void);

/**
 * @brief  打印模拟实时数据当前值（调试用）
 */
void SimRealtime_PrintDebug(void);

#endif /* __APP_PARAMS_H__ */
