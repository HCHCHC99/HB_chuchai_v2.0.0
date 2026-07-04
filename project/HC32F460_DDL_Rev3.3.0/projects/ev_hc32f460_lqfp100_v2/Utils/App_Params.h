#ifndef __APP_PARAMS_H__
#define __APP_PARAMS_H__

#include <stdint.h>
#include <stdbool.h>
#include "rtt_manager.h"

/*=============================================================================
 * ���Ժ궨��
 * ������ rtt_manager.h ��ͳһ������APP_PARAMS_DBG
 *============================================================================*/
#ifdef APP_PARAMS_DBG
    #define PARAMS_DBG(fmt, ...)    MAIN_D("[PARAMS] " fmt, ##__VA_ARGS__)
#else
    #define PARAMS_DBG(fmt, ...)    ((void)0)
#endif

/*=============================================================================
 * �ͻ�Э��Ĵ�����ַ����
 * ���ּĴ��� (Holding Register, ������ 03/06/10)
 *============================================================================*/

/* --- ���ò������ϵ籣�棬Flash �洢�� --- */
#define REG_NODE_ID                 (0x2710U)   /* �豸��ַ 1~247 (uint16_t) */
#define REG_TARGET_SPEED            (0x2711U)   /* ת�� r/min (int16_t) */
#define REG_TARGET_ANGLE            (0x2712U)   /* �Ƕ� 0.1�� (int16_t) */
#define REG_VOLTAGE_UPPER_LIMIT     (0x2714U)   /* ��ѹ���� 0.1V (uint16_t) */
#define REG_VOLTAGE_LOWER_LIMIT     (0x2715U)   /* ��ѹ���� 0.1V (uint16_t) */
#define REG_CURRENT_UPPER_LIMIT     (0x2716U)   /* �������� 1mA (uint16_t) */
#define REG_CLOSE_LIMIT_ANGLE       (0x271CU)   /* �رռ��޽Ƕ� 0.1�� (int16_t) */
#define REG_OPEN_LIMIT_ANGLE        (0x271DU)   /* �򿪼��޽Ƕ� 0.1�� (int16_t) */
#define REG_CURRENT_DETECT_MS       (0x271EU)   /* �������ʱ�� 1ms (uint16_t) */


/* --- ����������üĴ��� (���� Flash ���淶Χ���� RAM ��Ч) --- */
#define REG_MOTOR_HALL_DIR          (0x3710U)   /* ��������: 0=����, 1=��ת (uint16_t) */
#define REG_MOTOR_DIR               (0x3711U)   /* �������: 0=����, 1=��ת (uint16_t) */
#define REG_RTURN_REDUCTION_RATIO    (0x3712U)   /* ���ٱ� (uint16_t) */
#define REG_MOTOR_HALL_POLE_PAIRS   (0x3713U)   /* ��������� (uint16_t) */
#define REG_MOTOR_HALL_COUNT_LO    (0x3714U)   /* ���������ۼƵ�16λ (int32_t�ۼ�ֵ) */
#define REG_MOTOR_HALL_COUNT_HI    (0x3715U)   /* ���������ۼƸ�16λ */
#define REG_ABS_ANGLE_LO            (0x3716U)   /* RAM实时偏移低16位 (int32_t, 0.1度) */
#define REG_ABS_ANGLE_HI            (0x3717U)   /* RAM实时偏移高16位 */
#define REG_FLASH_ABS_LO            (0x3718U)   /* Flash已保存偏移低16位 (int32_t, 0.1度) */
#define REG_FLASH_ABS_HI            (0x3719U)   /* Flash已保存偏移高16位 */
#define REG_ABS_CMD                 (0x371AU)   /* W: 0=设零点, 1=保存到Flash */

/* 注意:0x2713, 0x2717-0x271B, 0x271F 为保留地址,不暴露给 Modbus */

/* --- �������д����������ʵ��״̬�� --- */
#define REG_CTRL_CMD                (0x2720U)   /* ��������Ĵ��� (uint16_t) */

/* REG_CTRL_CMD (0x2720) λ���壺
 * д������
 *   bit0 = 1: ������ʹ�� RS485 ���ƣ��������ȷ���ָ��
 *   bit1 = 1: ֹͣ���ر� RS485 ���ƣ�
 *   bit2 = 1: ��ͣ��ȡ����ת/��ת�����ر� RS485 ���ƣ�
 *   bit4 = 1: ��ת������������״̬����Ч���� bit5 ���⣩
 *   bit5 = 1: ��ת������������״̬����Ч���� bit4 ���⣩
 * ��������
 *   bit4 = 1: ��ǰ������ת
 *   bit5 = 1: ��ǰ���ڷ�ת
 *   bit4=0 �� bit5=0: ��ǰֹͣ
 */
#define CTRL_CMD_START              (0x0001U)   /* bit0: ����ָ�� */
#define CTRL_CMD_STOP               (0x0002U)   /* bit1: ָֹͣ�� */
#define CTRL_CMD_ESTOP              (0x0004U)   /* bit2: ��ָͣ�� */
#define CTRL_CMD_ABS_SAVE           (0x0040U)   /* bit6: save absolute position to Flash */
#define CTRL_CMD_FWD                (0x0010U)   /* bit4: ��תָ��/״̬ */
#define CTRL_CMD_REV                (0x0020U)   /* bit5: ��תָ��/״̬ */

/* --- ʵʱ���ݣ�ֻ�������� Flash�� --- */
#define REG_REAL_SPEED              (0x2730U)   /* ʵʱת�� r/min (int16_t) */
#define REG_REAL_ANGLE              (0x2731U)   /* ʵʱ�Ƕ� 0.1�� (int16_t) */
#define REG_REAL_VOLTAGE            (0x2732U)   /* ��ѹ 0.1V (uint16_t) */
#define REG_REAL_CURRENT            (0x2733U)   /* ���� 1mA (uint16_t) */
#define REG_REAL_DIRECTION          (0x2737U)   /* ʵʱת�� (int16_t) */

/* --- ʵʱ���ϣ�ֻ�������� Flash�� --- */
#define REG_FAULT_STATUS            (0x2740U)   /* ����״̬ (uint16_t) */

/*=============================================================================
 * ����״̬λ���� (REG_FAULT_STATUS, 0x2740)
 * 1=����, 0=����
 *============================================================================*/
#define FAULT_BIT_OVERVOLTAGE       (0x0001U)   /* bit0: ��ѹ */
#define FAULT_BIT_OVERCURRENT       (0x0002U)   /* bit1: ���� */
#define FAULT_BIT_OVERTEMP          (0x0004U)   /* bit2: ���£���������ǰδʹ�ã�*/
#define FAULT_BIT_RESET             (0x0008U)   /* bit3: ��λ */
#define FAULT_BIT_OVERLOAD          (0x0010U)   /* bit4: ���� */
#define FAULT_BIT_STALL             (0x0020U)   /* bit5: ��ת */
#define FAULT_BIT_UNDERVOLTAGE      (0x0040U)   /* bit6: Ƿѹ */

/*=============================================================================
 * Flash �洢����Ĭ��ֵ�궨��
 * ��Ӧ AppParamRecord_t �ṹ���ֶΣ��ϵ�� Flash ����
 * ע�⣺��ѹ�ͻ�ֵ��λ 0.1V�������ͻ�ֵ��λ mA
 *============================================================================*/

/* --- Modbus Э����������ּĴ������ϵ籣�棬��¶�� Modbus�� --- */
#define PARAM_DEFAULT_NODE_ID                   (1U)        /* �豸��ַ 1~247 */
#define PARAM_DEFAULT_TARGET_SPEED              (0)         /* ת�� r/min */
#define PARAM_DEFAULT_TARGET_ANGLE              (0)         /* �Ƕ� 0.1�� */
#define PARAM_DEFAULT_VOLTAGE_UPPER_LIMIT       (270U)      /* ��ѹ���� 0.1V (26.0V) */
#define PARAM_DEFAULT_VOLTAGE_LOWER_LIMIT       (210U)      /* ��ѹ���� 0.1V (21.0V) */
#define PARAM_DEFAULT_CURRENT_UPPER_LIMIT       (2300)       /* �������� 1mA (5A) */
#define PARAM_DEFAULT_CURRENT_DETECT_MS         (20)        /* �������ʱ�� 1ms */
#define PARAM_DEFAULT_MOTOR_HALL_DIR        (1)   /* �������� 0=����, 1=��ת */
#define PARAM_DEFAULT_MOTOR_DIR             (0)   /* ������� 0=����, 1=��ת */
#define PARAM_DEFAULT_RTURN_REDUCTION_RATIO   (11830)  /* ���ٱ� x0.1 */
#define PARAM_DEFAULT_MOTOR_HALL_POLE_PAIRS  (3)       /* ��������� */
#define PARAM_DEFAULT_CLOSE_LIMIT_ANGLE         (-20)       /* �رռ��޽Ƕ� 0.1�� */
#define PARAM_DEFAULT_OPEN_LIMIT_ANGLE          (880)       /* �򿪼��޽Ƕ� 0.1�� */
#define PARAM_DEFAULT_BAUD_RATE                 (9600UL)    /* Ĭ�ϲ����� */

/* --- �ڲ�����������¶�� Modbus���� Flash �洢���ϵ���أ� --- */
#define PARAM_DEFAULT_VOLTAGE_UPPER_HYSTERESIS  (20U)       /* ��ѹ�ͻ� 0.1V (2.0V) */
#define PARAM_DEFAULT_VOLTAGE_LOWER_HYSTERESIS  (20U)       /* Ƿѹ�ͻ� 0.1V (2.0V) */
#define PARAM_DEFAULT_OVERVOLTAGE_TRIGGER_CNT   (1U)        /* ��ѹ�������� */
#define PARAM_DEFAULT_UNDERVOLTAGE_TRIGGER_CNT  (1U)        /* Ƿѹ�������� */
#define PARAM_DEFAULT_CURRENT_HYSTERESIS_MA     (0U)        /* �����ͻ� 1mA (0A) - ��Ϊ0�����ͻ� */
#define PARAM_DEFAULT_CURRENT_RELEASE_MS        (200U)      /* �������ʱ�� 1ms (0.2s) */
#define PARAM_DEFAULT_OVERCURRENT_TRIGGER_CNT   (1U)        /* ��������������ʱ��ģʽ��ѡʹ�ã�*/

/*=============================================================================
 * ����ʱ���ú궨�壨�� Flash �洢�����궨�忪�أ�
 *============================================================================*/

/* --- ���Կ��� --- */
#define APP_PARAMS_REALTIME_DBG                 /* ����ʵʱ���������Դ�ӡ��ÿ5��һ�Σ� */
// #define APP_PARAMS_REALTIME_SIMULATE          /* ����ʵʱ����ģ�⣨ÿ5�������+1�� */
#define APP_PARAMS_SIM_DBG                      /* ����ģ��ʵʱ���ݵ��Դ�ӡ */

/* --- ʵʱ������Դѡ�� --- */
/* ע�͵�������ʹ��ģ��ʵʱ���ݣ�SimRealtimeData��Keil Debug �����޸ģ�
 * ȡ��ע�����ʵ�ʵ� dev_rturn �豸��ȡʵʱ���� */
#define APP_PARAMS_USE_DEV_RTURN

/*=============================================================================
 * ���Ĳ�����¼�ṹ�� (�ͻ� Modbus Э�鶨��)
 * ע�⣺�ṹ�岼�ֱ����� Flash �д洢�Ĳ���һ��
 *       head_magic �� tail_magic ���� Flash ��ʶ��
 *       checksum ��������������У��
 *============================================================================*/
#pragma pack(4)
typedef struct {
    /* ͷ����Ϣ */
    uint32_t head_magic;        /* ͷ��ħ�� (0x55AA55AA) */
    uint32_t sequence_id;       /* ���кţ�ÿ���޸ĵ��� */
    uint32_t erase_count;       /* Flash����������¼ */

    /* �ͻ� Modbus Э����� (���ּĴ������ϵ籣�棬��¶�� Modbus) */
    uint16_t node_id;               /* 0x2710: �豸��ַ 1~247 */
    int16_t  target_speed;          /* 0x2711: ת�� (r/min) */
    int16_t  target_angle;          /* 0x2712: �Ƕ� (0.1��) */
    uint16_t voltage_upper_limit;   /* 0x2714: ��ѹ���� (0.1V) */
    uint16_t voltage_lower_limit;   /* 0x2715: ��ѹ���� (0.1V) */
    uint16_t current_upper_limit;   /* 0x2716: �������� (1mA) */
    uint16_t current_detect_ms;     /* 0x271E: �������ʱ�� (1ms) */
    int16_t  close_limit_angle;     /* 0x271C: �رռ��޽Ƕ� (0.1��) */
    int16_t  open_limit_angle;      /* 0x271D: �򿪼��޽Ƕ� (0.1��) */
    uint32_t baud_rate;             /* ������ (�ڲ�����������Modbus) */

    /* �ڲ�����������¶�� Modbus���� Flash �洢���ϵ���أ� */
    uint16_t voltage_upper_hysteresis;   /* ��ѹ�ͻ� (0.1V) */
    uint16_t voltage_lower_hysteresis;   /* Ƿѹ�ͻ� (0.1V) */
    uint8_t  overvoltage_trigger_count;  /* ��ѹ�������� */
    uint8_t  undervoltage_trigger_count; /* Ƿѹ�������� */
    uint16_t current_hysteresis_ma;      /* �����ͻ� (mA) */
    uint16_t current_release_ms;         /* �������ʱ�䴰�� (ms) */
    uint8_t  overcurrent_trigger_count;  /* ��������������ʱ��ģʽ��ѡʹ�ã�*/
    uint16_t  motor_hall_dir;               /* 0x3710: �������� 0=���� 1=��ת */
    uint16_t  motor_dir;                    /* 0x3711: ������� 0=���� 1=��ת */
    uint16_t  rturn_reduction_ratio;          /* 0x3712: ���ٱ�x10 */
    uint16_t  motor_hall_pole_pairs;          /* 0x3713: ��������� */
    uint8_t  reserved[3];                /* �����ֽڣ����뵽4�ֽڱ߽� */

    /* β����Ϣ */
    uint32_t checksum;              /* CRC32У��� */
    uint32_t tail_magic;            /* β��ħ�� (0xAA44AA44) */
} AppParamRecord_t;
#pragma pack()

/*=============================================================================
 * ʵʱ���ݽṹ�壨���� Flash��������ʱʹ�ã�
 *============================================================================*/
typedef struct {
    int16_t  real_speed;        /* 0x2730: ʵʱת�� (r/min) */
    int16_t  real_angle;        /* 0x2731: ʵʱ�Ƕ� (0.1��) */
    uint16_t real_voltage;      /* 0x2732: ��ѹ (0.1V) */
    uint16_t real_current;      /* 0x2733: ���� (1mA) */
    int16_t  real_direction;    /* 0x2737: ʵʱת�� */
    uint16_t fault_status;      /* 0x2740: ����״̬ (�� FAULT_BIT_xxx ����) */
} AppRealTimeData_t;

/*=============================================================================
 * ģ��ʵʱ���ݽṹ�壨���� Keil Debug �����޸ģ�
 *============================================================================*/
typedef struct {
    volatile int16_t  speed;        /* ʵʱת�� r/min */
    volatile int16_t  angle;        /* ʵʱ�Ƕ� 0.1�� */
    volatile uint16_t voltage;      /* ��ѹ 0.1V */
    volatile uint16_t current;      /* ���� 1mA */
    volatile int16_t  direction;    /* ʵʱת�� */
    volatile uint16_t fault_set;    /* �������ã�д�� FAULT_BIT_xxx �����ö�Ӧ����λ */
    volatile uint16_t fault_clear;  /* ���������д�� FAULT_BIT_xxx �������Ӧ����λ */
} SimRealtimeData_t;

/*=============================================================================
 * ȫ�ֱ�������
 *============================================================================*/
extern AppParamRecord_t g_AppParam;         /* Ӧ�ó��������Flash �洢�� */
extern AppRealTimeData_t g_RealTimeData;    /* ʵʱ���ݣ����� Flash�� */
extern volatile int32_t g_s32HallPulseAccum;  /* ���������ۻ�ֵ */
extern SimRealtimeData_t g_SimRealtimeData; /* ģ��ʵʱ���ݣ�Keil Debug �޸ģ� */

/*=============================================================================
 * ������д�ӿں�������
 *============================================================================*/

/**
 * @brief  ���ݼĴ�����ַ��ȡ����ֵ
 * @param  regAddr  �Ĵ�����ַ (REG_xxx)
 * @param  pValue   �������ֵָ��
 * @return 0 �ɹ���-3 ��Ч��ַ
 */
int32_t Param_ReadByReg(uint16_t regAddr, uint16_t *pValue);

/**
 * @brief  ���ݼĴ�����ַд�����ֵ
 * @param  regAddr  �Ĵ�����ַ (REG_xxx)
 * @param  value    Ҫд���ֵ
 * @return 0 �ɹ���-3 ��Ч��ַ
 */
int32_t Param_WriteByReg(uint16_t regAddr, uint16_t value);

/*=============================================================================
 * ʵʱ���ݲ�����������
 *============================================================================*/

/**
 * @brief  ���ù���λ����λ��
 * @param  bitMask  ����λ���� (FAULT_BIT_xxx)
 */
void RealTime_SetFault(uint16_t bitMask);

/**
 * @brief  �������λ����λ��ȡ����
 * @param  bitMask  ����λ���� (FAULT_BIT_xxx)
 */
void RealTime_ClearFault(uint16_t bitMask);

/**
 * @brief  ������λ
 * @param  bitMask  ����λ���� (FAULT_BIT_xxx)
 * @retval true     �ù���λΪ 1
 * @retval false    �ù���λΪ 0
 */
bool RealTime_CheckFault(uint16_t bitMask);

/**
 * @brief  ����ʵʱ���ݣ��������豸ģ����ã�
 * @param  speed      ʵʱת�� (r/min)���� NULL ��ʾ������
 * @param  angle      ʵʱ�Ƕ� (0.1��)���� NULL ��ʾ������
 * @param  voltage    ��ѹ (0.1V)���� NULL ��ʾ������
 * @param  current    ���� (1mA)���� NULL ��ʾ������
 * @param  direction  ʵʱת�򣬴� NULL ��ʾ������
 */
void RealTime_Update(const int16_t  *speed,
                     const int16_t  *angle,
                     const uint16_t *voltage,
                     const uint16_t *current,
                     const int16_t  *direction);

/**
 * @brief  ���豸��ȡʵʱ���ݲ����µ� g_RealTimeData
 *         ���� APP_PARAMS_USE_DEV_RTURN �꿪��ʱ��Ч
 * @param  u8RTurnDevId   dev_rturn �豸 ID
 * @param  u8VoltageDevId dev_voltage �豸 ID
 * @param  u8SensorDevId  dev_sensor �豸 ID
 */
void RealTime_UpdateFromDevice(uint8_t u8RTurnDevId,
                               uint8_t u8VoltageDevId,
                               uint8_t u8SensorDevId);

/**
 * @brief  ��ӡʵʱ���ݣ������ã�
 */
void RealTime_PrintDebug(void);

/**
 * @brief  ʵʱ����ģ�⣨ÿ5�������+1�����ڲ�����λ����ȡ��
 *         ���� APP_PARAMS_REALTIME_SIMULATE �꿪��ʱ��Ч
 */
void RealTime_Simulate(void);

/*=============================================================================
 * ģ��ʵʱ���ݲ����������������� Keil Debug �����޸ģ�
 *============================================================================*/

/**
 * @brief  ��ʼ��ģ��ʵʱ����
 */
void SimRealtime_Init(void);

/**
 * @brief  ͬ��ģ�����ݵ�ʵ��ʵʱ���ݣ�����ѭ���е��ã�
 *         �� g_SimRealtimeData �е�ֵͬ���� g_RealTimeData
 */
void SimRealtime_Sync(void);

/**
 * @brief  ��ӡģ��ʵʱ���ݵ�ǰֵ�������ã�
 */
void SimRealtime_PrintDebug(void);

#endif /* __APP_PARAMS_H__ */
