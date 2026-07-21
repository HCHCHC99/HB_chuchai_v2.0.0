#ifndef EVENT_BUS_H_
#define EVENT_BUS_H_

// EventBus - �¼�����ϵͳ�����ڷ�������ģʽ���¼�����
#include <stdint.h>
#include <stdbool.h>
#include "rtt_manager.h"

// ========== ���Ժ궨�� ==========
// ������ rtt_manager.h ��ͳһ������DEV_EVENT_BUS / DEV_EVENT_BUS_VERBOSE

#ifdef DEV_EVENT_BUS
    #define EVENT_BUS_DEBUG_PRINT(fmt, ...)    MAIN_D("[EVENT_BUS] " fmt, ##__VA_ARGS__)
#else
    #define EVENT_BUS_DEBUG_PRINT(fmt, ...)    ((void)0)
#endif

#ifdef DEV_EVENT_BUS_VERBOSE
    #define EVENT_BUS_VERBOSE_PRINT(fmt, ...)  MAIN_D("[EVENT_BUS] " fmt, ##__VA_ARGS__)
#else
    #define EVENT_BUS_VERBOSE_PRINT(fmt, ...)  ((void)0)
#endif

typedef enum {
    TOPIC_POWER = 0,
    TOPIC_LIMIT_HARD,
    TOPIC_LIMIT_SOFT,
    TOPIC_CAN_EVENT,
    TOPIC_MOTOR_CMD,
    TOPIC_MOTOR_SPEED_FEEDBACK, 
    TOPIC_MOTOR_DRIVE_EXEC,
    TOPIC_MANUAL_IO,
    TOPIC_ALARM,
    TOPIC_VOLTAGE_ALARM,
    TOPIC_CURRENT_ALARM,
    TOPIC_RTURN_LIMIT,
    TOPIC_FAULT_CLEAR,
    TOPIC_MANUAL_RS485,
    TOPIC_OVERCURRENT_FWD,
    TOPIC_OVERCURRENT_REV,
    TOPIC_MAX
} Topic_t;

// ��������ӳ�䣨���ڵ��Դ�ӡ��
static const char* const g_topic_names[] = {
    [TOPIC_POWER]               = "POWER",
    [TOPIC_LIMIT_HARD]          = "LIMIT_HARD",
    [TOPIC_LIMIT_SOFT]          = "LIMIT_SOFT",
    [TOPIC_CAN_EVENT]           = "CAN_EVENT",
    [TOPIC_MOTOR_CMD]           = "MOTOR_CMD",
    [TOPIC_MOTOR_SPEED_FEEDBACK] = "MOTOR_SPEED_FEEDBACK",
    [TOPIC_MOTOR_DRIVE_EXEC]    = "MOTOR_DRIVE_EXEC",
    [TOPIC_MANUAL_IO]           = "MANUAL_IO",
    [TOPIC_ALARM]               = "ALARM",
    [TOPIC_VOLTAGE_ALARM]       = "VOLTAGE_ALARM",
    [TOPIC_CURRENT_ALARM]       = "CURRENT_ALARM",
    [TOPIC_RTURN_LIMIT]         = "RTURN_LIMIT",
    [TOPIC_FAULT_CLEAR]         = "FAULT_CLEAR",
    [TOPIC_MANUAL_RS485]        = "MANUAL_RS485",
    [TOPIC_OVERCURRENT_FWD]     = "OVERCURRENT_FWD",
    [TOPIC_OVERCURRENT_REV]     = "OVERCURRENT_REV"
};

typedef void (*EventCallback)(void* payload);

/**
 * @brief ���Ļ��⣨�����ȼ���
 * @param topic ��ע�Ļ���
 * @param callback ����ʱ�Ļص�����
 * @param priority ���ȼ���0=��ߣ���ֵԽ�����ȼ�Խ�ͣ�
 * @return �Ƿ��ĳɹ�
 * 
 * @note ���ȼ��������
 *       - ��ֵԽС���ȼ�Խ�ߣ�0 > 1 > 2 > ...��
 *       - ͬ�����ȼ��£��ȶ��ĵ���ִ��
 */
bool EventBus_Subscribe(Topic_t topic, EventCallback callback, uint8_t priority);

void EventBus_Init(void);
void EventBus_Publish(Topic_t topic, void* payload);

/**
 * @brief �����¼��������ſػ��ƣ�
 *        ���ô˺��������л���� Publish �Ż�����ִ�лص���
 *        �ڴ�֮ǰ���õ� Publish ����¼�¼���־����ִ�лص���
 */
void EventBus_Enable(void);

/**
 * @brief ��ѯ�¼������Ƿ�������
 * @return true �����ã�false δ���ã���ʼ���׶Σ�
 */
bool EventBus_IsEnabled(void);

#endif /* EVENT_BUS_H_ */
