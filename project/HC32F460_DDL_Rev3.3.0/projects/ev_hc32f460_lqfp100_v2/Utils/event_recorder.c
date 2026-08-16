/*------------------------------------------------------------------------------
 * event_recorder.c - Event Recorder 轻量封装层（实现）
 *
 * 编译期检测原理：
 *   工程启用 RTE 时，uVision 会生成 RTE_Components.h 并自动加入包含路径；
 *   只要在 RTE 中勾选了 Compiler:Event Recorder 组件，该文件内就会定义
 *   RTE_Compiler_EventRecorder 宏。据此实现"组件是否加载"的编译期判断。
 *
 * 边界说明：
 *   - 本文件依赖 RTE_Components.h 存在（即工程启用了 RTE）；
 *   - 若整个工程完全不用 RTE，请直接删除本文件并移除相关调用。
 *------------------------------------------------------------------------------*/
#include "event_recorder.h"

#include "RTE_Components.h"

#if defined(RTE_Compiler_EventRecorder)
  #include "EventRecorder.h"
  #define EVTREC_ENABLED  1U
#else
  #define EVTREC_ENABLED  0U
#endif

/* B 级"配对门控"标志：StartB_Gated 置位，StopB_Gated 清零；
 * 只有 Start 过才允许记录 Stop，避免出现无配对的孤立 Stop 事件。
 * 跨 ISR / 主循环访问，故用 volatile。 */
static volatile uint8_t s_u8EvtRecBGate = 0U;

/*------------------------------------------------------------------------------
 * 初始化
 *------------------------------------------------------------------------------*/
uint32_t EvtRec_Init(void)
{
#if EVTREC_ENABLED
    return EventRecorderInitialize(EventRecordAll, 1U);
#else
    return 0U;
#endif
}

/*------------------------------------------------------------------------------
 * 事件开始 / 结束（测耗时），slot 0~15
 *------------------------------------------------------------------------------*/
uint32_t EvtRec_EventStartA(uint32_t slot)
{
#if EVTREC_ENABLED
    return EventStartA(slot);
#else
    (void)slot;
    return 0U;
#endif
}

uint32_t EvtRec_EventStopA(uint32_t slot)
{
#if EVTREC_ENABLED
    return EventStopA(slot);
#else
    (void)slot;
    return 0U;
#endif
}

uint32_t EvtRec_EventStartB(uint32_t slot)
{
#if EVTREC_ENABLED
    return EventStartB(slot);
#else
    (void)slot;
    return 0U;
#endif
}

uint32_t EvtRec_EventStopB(uint32_t slot)
{
#if EVTREC_ENABLED
    return EventStopB(slot);
#else
    (void)slot;
    return 0U;
#endif
}

/*------------------------------------------------------------------------------
 * B 级"配对门控"版本
 *   只有在 StartB_Gated 之后调用 StopB_Gated 才会真正记录 Stop，
 *   否则直接忽略，避免出现无配对的孤立 Stop 事件。
 *------------------------------------------------------------------------------*/
uint32_t EvtRec_EventStartBGated(uint32_t slot)
{
    s_u8EvtRecBGate = 1U;
#if EVTREC_ENABLED
    return EventStartB(slot);
#else
    (void)slot;
    return 0U;
#endif
}

uint32_t EvtRec_EventStopBGated(uint32_t slot)
{
    if (s_u8EvtRecBGate == 0U) {
        (void)slot;
        return 0U;   /* 未启动：不记录 Stop */
    }
    s_u8EvtRecBGate = 0U;
#if EVTREC_ENABLED
    return EventStopB(slot);
#else
    (void)slot;
    return 0U;
#endif
}

uint32_t EvtRec_EventStartC(uint32_t slot)
{
#if EVTREC_ENABLED
    return EventStartC(slot);
#else
    (void)slot;
    return 0U;
#endif
}

uint32_t EvtRec_EventStopC(uint32_t slot)
{
#if EVTREC_ENABLED
    return EventStopC(slot);
#else
    (void)slot;
    return 0U;
#endif
}

uint32_t EvtRec_EventStartD(uint32_t slot)
{
#if EVTREC_ENABLED
    return EventStartD(slot);
#else
    (void)slot;
    return 0U;
#endif
}

uint32_t EvtRec_EventStopD(uint32_t slot)
{
#if EVTREC_ENABLED
    return EventStopD(slot);
#else
    (void)slot;
    return 0U;
#endif
}

/*------------------------------------------------------------------------------
 * 记录事件 + 参数
 *------------------------------------------------------------------------------*/
uint32_t EvtRec_Record2(uint32_t id, uint32_t val1, uint32_t val2)
{
#if EVTREC_ENABLED
    return EventRecord2(id, val1, val2);
#else
    (void)id; (void)val1; (void)val2;
    return 0U;
#endif
}

uint32_t EvtRec_Record4(uint32_t id, uint32_t val1, uint32_t val2, uint32_t val3, uint32_t val4)
{
#if EVTREC_ENABLED
    return EventRecord4(id, val1, val2, val3, val4);
#else
    (void)id; (void)val1; (void)val2; (void)val3; (void)val4;
    return 0U;
#endif
}

/*------------------------------------------------------------------------------
 * 使能 / 关闭事件记录（按等级与组件范围）
 *------------------------------------------------------------------------------*/
uint32_t EvtRec_Enable(uint32_t recording, uint32_t comp_start, uint32_t comp_end)
{
#if EVTREC_ENABLED
    return EventRecorderEnable(recording, comp_start, comp_end);
#else
    (void)recording; (void)comp_start; (void)comp_end;
    return 0U;
#endif
}

uint32_t EvtRec_Disable(uint32_t recording, uint32_t comp_start, uint32_t comp_end)
{
#if EVTREC_ENABLED
    return EventRecorderDisable(recording, comp_start, comp_end);
#else
    (void)recording; (void)comp_start; (void)comp_end;
    return 0U;
#endif
}
