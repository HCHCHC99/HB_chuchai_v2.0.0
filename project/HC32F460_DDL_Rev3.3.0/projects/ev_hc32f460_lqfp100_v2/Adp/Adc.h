/**
 *******************************************************************************
 * @file  Adc.h
 * @brief ADC Driver for HC32F460 - Generic framework with multiple instances
 *******************************************************************************
 */

#ifndef __ADC_H__
#define __ADC_H__

#include "main.h"
#include "Hardware.h"
#include "TickTimer.h"

#define ADC_RTT 1

#ifdef DEBUG_ADC_Adp
    #define ADC_Adp_DEBUG(fmt, ...)    MAIN_D("[ADC_DEBUG] " fmt, ##__VA_ARGS__)
#else
    #define ADC_Adp_DEBUG(fmt, ...)    ((void)0)
#endif

#ifdef ADC_OUTPUT_Adp
    #define ADC_Adp_OUT(fmt, ...)      MAIN_D("[ADC_OUT] " fmt, ##__VA_ARGS__)
#else
    #define ADC_Adp_OUT(fmt, ...)      ((void)0)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/* DMA��������С���� */
#define ADC_DMA_BUFFER_SIZE              (8U)  /* ÿ�δ����ɼ�8�� */

/* ADCӲ�����ú� */
#define ADC_UNIT                        (CM_ADC1)
#define ADC_PERIPH_CLK                  (FCG3_PERIPH_ADC1)

/* ===== ����A�������ã���һ����TMR0 �� ADC��===== */
#define ADC_SEQA_HARDTRIG               (ADC_HARDTRIG_EVT0)        /* ʹ���¼����� */
#define ADC_SEQA_AOS_TRIG_SEL           (AOS_ADC1_0)               /* ADC1���¼����� */
#define ADC_SEQA_TRIG_EVT               (EVT_SRC_TMR0_1_CMP_B)     /* ��ʱ���Ƚ��¼� */

/* ===== DMA�������ã��ڶ�����ADC �� DMA��===== */
#define DMA_UNIT                        (CM_DMA1)
#define DMA_PERIPH_CLK                  (FCG0_PERIPH_DMA1)
#define DMA_DATA_WIDTH                  (DMA_DATAWIDTH_16BIT)      /* 16λ���� */
#define DMA_TRANS_CNT                   (0U)                        /* 0:���޴��� */
#define DMA_INT_PRIO                    (DDL_IRQ_PRIO_03)

/* Timer0 for sequence A */
#define TMR0_UNIT                       (CM_TMR0_1)
#define TMR0_CH                         (TMR0_CH_B)
#define TMR0_PERIPH_CLK                 (FCG2_PERIPH_TMR0_1)
#define TMR0_CLK_DIV                    (TMR0_CLK_DIV256)

/* ADC����A�ж����� */
#define ADC_SEQA_INT_PRIO               (DDL_IRQ_PRIO_06)
#define ADC_SEQA_INT_SRC                (INT_SRC_ADC1_EOCA)
#define ADC_SEQA_INT_IRQn               (INT116_IRQn)

/* ADC�ο���ѹ */
#define ADC_VREF                        (3.3F)
#define ADC_ACCURACY                    (1UL << 12U)
#define ADC_CAL_VOL(adcVal)             (uint16_t)((((float32_t)(adcVal) * ADC_VREF) / ((float32_t)ADC_ACCURACY)) * 1000.F)

/* ���ADCʵ������ */
#define ADC_MAX_INSTANCES               (8U)

/* ���ADCͨ���� */
#define ADC_MAX_CHANNEL                 (32U)

/* ���DMAͨ���� */
#define ADC_MAX_DMA_CH                  (8U)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/**
 * @brief  ADCͨ������ģʽ����
 */
typedef enum {
    ADC_MODE_INTERRUPT = 0,    /* �ж�ģʽ */
    ADC_MODE_DMA = 1,          /* DMAģʽ */
} en_adc_mode_t;

/**
 * @brief  ADC����ӳ��ṹ��
 */
typedef struct {
    uint8_t u8Port;      /* GPIO�˿� (�� GPIO_PORT_A) */
    uint8_t u8Pin;       /* GPIO���� (�� GPIO_PIN_01) */
} stc_adc_pin_t;

/**
 * @brief  ADCͨ�����ýṹ�� (�û�����ʱ��д)
 */
typedef struct {
    uint8_t u8Channel;              /* ADCͨ���� (�� ADC_CH1, ADC_CH4 ��) */
    en_adc_mode_t enMode;           /* ����ģʽ */
    stc_adc_pin_t stcPin;           /* �������� */
    void (*pfnCallback)(uint16_t u16AdcValue);  /* �ж�ģʽ�ص����� */
    /* DMA������� - ֻ��ģʽΪDMAʱ����Ч */
    struct {
        uint16_t u16BufferSize;       /* DMA��������С */
        uint8_t u8DmaChannel;         /* ʹ�õ�DMAͨ�� */
    } stcDmaConfig;
} stc_adc_config_t;


typedef struct {
    uint8_t u8Id;                    /* ADCʵ��ID (0-7) */
    uint8_t u8Channel;              /* ADCͨ���� */
    en_adc_mode_t enMode;           /* ����ģʽ */
    uint8_t u8Port;                 /* GPIO�˿� */
    uint8_t u8Pin;                  /* GPIO���� */
    void (*pfnCallback)(uint16_t u16AdcValue);  /* �ж�ģʽ�ص����� */
    /* DMA��� */
    uint16_t *pu16DmaBuffer;         /* DMA������ָ�� */
    uint16_t u16DmaBufferSize;       /* DMA��������С */
    uint8_t u8DmaChannel;            /* ʹ�õ�DMAͨ�� */
    /* ����״̬ */
    uint32_t u32SampleCount;         /* �������� */
    uint16_t u16LatestValue;         /* ����ֵ (�ж�ģʽ) */
    uint8_t u8ValueUpdated;          /* ���ݸ��±�־ */
} stc_adc_instance_t;


/*******************************************************************************
 * Global function prototypes
 ******************************************************************************/

/* ADC ʵ������ */
uint8_t Adc_Create(stc_adc_config_t *pstcConfig);
void Adc_Init(void);
void Adc_DeInit(void);

/* ADC ���ƺ��� */
void Adc_Start(void);
void Adc_Stop(void);
void Adc_ProcessData(uint32_t u32PrintIntervalMs);

/* ADC ���ݻ�ȡ���� */
uint16_t Adc_GetLatestValue(uint8_t u8AdcId);
uint16_t Adc_GetAverageValue(uint8_t u8AdcId);
uint32_t Adc_GetSampleCount(uint8_t u8AdcId);

/* ����ͨ���Ų���ADC ID */
int8_t Adc_FindIdByChannel(uint8_t u8Channel);

/* ����ʹ��ADC�жϣ�����������DMAͨ���������ж�ģʽͨ���� */
void Adc_EnableInterrupt(void);

/* ������Ϣ */
#ifdef DEBUG
void Adc_PrintDebugInfo(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H__ */
