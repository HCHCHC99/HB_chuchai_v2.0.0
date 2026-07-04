#ifndef APP_MOTOR_PROJECT_H_
#define APP_MOTOR_PROJECT_H_

#include "device_manager.h"
#include "EventBus.h"
#include "dev_motor.h"          // ����豸�������ٲ��߼���
#include "dev_pwm.h"            // PWM�豸
#include "dev_power.h"          // ��Դ�豸
#include "dev_io.h"             // IO�豸
#include "dev_hall.h"           // �����豸

// ��ʱע�͵�δʵ�ֵ��豸
#include "dev_adc.h"            // ADC�豸
#include "dev_voltage.h"        // ��ѹĸ���豸
#include "dev_sensor.h"         // �����������豸
// #include "dev_actuator.h"       // ִ�����豸
// #include "dev_can.h"            // CAN�豸
#include "dev_motor_hall.h"     // ��������豸

// --- ģ��ģʽ���ƺ꣨1=����ģ��, 0=ʹ����ʵӲ����---
#ifndef ENABLE_SIMULATION_MODE
#define ENABLE_SIMULATION_MODE  1
#endif

// ========== ���״̬ö�� ==========
#define MOTOR_STOPPED    0
#define MOTOR_FORWARD    1
#define MOTOR_REVERSE    2
#define MOTOR_FAULT      3

// ========== ��Դ״̬ö�� ==========
#define POWER_BOTH_OFF   0
#define POWER_POS_ON     1
#define POWER_NEG_ON     2
#define POWER_BOTH_ON    3

// ========== ����״̬ö�� ==========
#define HALL_NO_LIMIT    0
#define HALL_UP_LIMIT    1
#define HALL_DOWN_LIMIT  2
#define HALL_BOTH_LIMIT  3

// ========== ȫ���豸ID���� ==========
#define ID_PWM_MOTOR        9   // ���PWM�豸
#define ID_PWR_POS          1   // ����Դ
#define ID_PWR_NEG          2   // ����Դ
#define ID_PWR_TEST1        3   // ���Ե�Դ1 (PB10)
#define ID_PWR_TEST2        4   // ���Ե�Դ2 (PA02)
#define ID_HALL_UP          5   // ����λ���� (��ע��)
#define ID_HALL_DOWN        6   // ����λ���� (��ע��)
#define ID_IO_FWD           7   // ��תIO�豸
#define ID_IO_REV           8   // ��תIO�豸
#define ID_MOTOR            0   // ����ٲ��豸
#define ID_MOTOR_HALL       10  // �������
#define ID_ADC_CURRENT      11  // ��������
#define ID_ADC_VOLTAGE      12  // ĸ�ߵ�ѹ����
#define ID_VOLTAGE_BUS      13  // ��ѹĸ���豸������ADC_VOLTAGE���㣩
#define ID_SENSOR_CURRENT   14  // �����������豸������ADC_CURRENT���㣩
#define ID_RTURN            15  // Բ��ת�������豸

// ========== ��ѹ�澯��ֵ���ã������� App_Params.h�� ==========

// ������Щ�꣨����ʱ�̶���Ӳ����أ�
#define OVERCURRENT_MODE                    OVERCURRENT_MODE_TIME_WINDOW  // �̶�ʹ��ʱ��ģʽ
#define CURRENT_TRIGGER_WINDOW_SIZE         (0)      // ����ģʽʱ����
#define CURRENT_RELEASE_WINDOW_SIZE         (0)      // ����ģʽʱ����

// ����Բ��ת���������ã�����ʱ�̶���
#define RTURN_REDUCTION_RATIO           (11830.0f)  /* ʵ�� = �Ĵ���ֵ / 10 */
#define RTURN_MAX_ANGLE                 (88.0f)
#define RTURN_MIN_ANGLE                 (-2.0f)
#define RTURN_UPDATE_INTERVAL_MS        (1)
#define RTURN_REVERSE_OUTPUT            (0)

// ========== Ӳ���ܽź궨�� (���ϰ壬�� HandB һ��) ==========
//   HB_chuchai ԭʼ�ܽż�ע�ͣ�ȡ��ע�Ϳɻ�ԭ

// --- ����Դ���ƹܽ� ---
#define PIN_PWR_POS_PORT        GPIO_PORT_C
#define PIN_PWR_POS_PIN         GPIO_PIN_13

// --- ����Դ���ƹܽ� ---
#define PIN_PWR_NEG_PORT        GPIO_PORT_C
#define PIN_PWR_NEG_PIN         GPIO_PIN_14

// --- Hall �������ܽ� ---
#define PIN_HALL_A_PORT         GPIO_PORT_A
#define PIN_HALL_A_PIN          GPIO_PIN_10
#define PIN_HALL_B_PORT         GPIO_PORT_A
#define PIN_HALL_B_PIN          GPIO_PIN_09

// --- ADC ���������ܽ� ---
#define PIN_ADC_CURRENT_PORT    GPIO_PORT_A
#define PIN_ADC_CURRENT_PIN     GPIO_PIN_05
// --- Ӳ���屾��ADC ��ѹ���ܽ� ---
// �� main.h �� BOARD_VERSION ͳһ����
#if BOARD_VERSION == 0
    // ԭHB_chuchai��
    #define PIN_ADC_VOLTAGE_PORT    GPIO_PORT_A
    #define PIN_ADC_VOLTAGE_PIN     GPIO_PIN_04
    #define PIN_ADC_VOLTAGE_CH      (4)
#else
    // ���ϰ�
    #define PIN_ADC_VOLTAGE_PORT    GPIO_PORT_A
    #define PIN_ADC_VOLTAGE_PIN     GPIO_PIN_06
    #define PIN_ADC_VOLTAGE_CH      (6)
#endif

// --- IO ��ת/��ת�ܽ� ---
#define PIN_IO_FWD_PORT         GPIO_PORT_B
#define PIN_IO_FWD_PIN          GPIO_PIN_00
#define PIN_IO_REV_PORT         GPIO_PORT_B
#define PIN_IO_REV_PIN          GPIO_PIN_01

// --- ��� Hall ���������� ---
#define MOTOR_HALL_POLE_PAIRS   (3)
#define MOTOR_HALL_COUNT        (2)
#define MOTOR_HALL_UPDATE_MS    (1)

// --- Hall �ж����� ---
#define HALL_EIRQ_CH_A          EXTINT_CH10
#define HALL_EIRQ_CH_B          EXTINT_CH09
#define HALL_IRQN_A             INT010_IRQn
#define HALL_IRQN_B             INT009_IRQn
#define HALL_IRQ_SRC_A          INT_SRC_PORT_EIRQ10
#define HALL_IRQ_SRC_B          INT_SRC_PORT_EIRQ9
#define HALL_IRQ_PRIORITY       (0)





// ========== ģ�����ݽṹ�� ==========
typedef struct {
    uint8_t sim_pwr_pos;
    uint8_t sim_pwr_neg;
    uint8_t sim_hall_up;
    uint8_t sim_hall_down;
    uint8_t sim_io_fwd;
    uint8_t sim_io_rev;
    float   sim_io_speed;
    uint16_t sim_adc_val;
    int32_t sim_motor_speed;
    uint8_t sim_motor_dir;
} SystemSim_t;

// ========== ״ָ̬ʾ���� ==========
typedef struct {
    uint8_t motor_status;
    uint8_t power_status;
    uint8_t hall_status;
    uint8_t io_status;
    float   current_duty;
} SystemStatus_t;

extern SystemSim_t g_sim;
extern SystemStatus_t g_status;

void ESystem_Init(void);
void ESystem_MainLoop(void);
void App_ReloadConfig(void);

#if ENABLE_SIMULATION_MODE
void Sim_ProcessInput(void);
void Sim_PublishEvents(void);
#endif

#endif /* APP_MOTOR_PROJECT_H_ */
