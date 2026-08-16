/*------------------------------------------------------------------------------
 * event_recorder.h - Event Recorder 轻量封装层
 *
 * 目的：
 *   1. 统一封装 Keil RTE 的 Event Recorder 组件接口；
 *   2. 编译期自动检测组件是否加载：
 *      - 已加载（RTE 中勾选了 Compiler:Event Recorder，即定义了
 *        RTE_Compiler_EventRecorder 宏）：正常转发到 EventRecorder 接口；
 *      - 未加载：所有接口退化为空操作，保证调用方代码不变、工程仍可编译。
 *
 * 使用约定：
 *   - EventStartA/B/C/D 的 slot（槽位）取值范围 0~15，且 Start/Stop 必须用同一槽位；
 *   - Record2/Record4 的 id 建议用（事件编号 + EventLevelAPI / EventLevelOp /
 *     EventLevelError）组合，与 Event Recorder 惯例一致。
 *
 * 注意：本头文件不依赖 EventRecorder.h，任何文件都可安全包含。
 *------------------------------------------------------------------------------*/
#ifndef EVENT_RECORDER_H
#define EVENT_RECORDER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化：记录全部等级事件，连接调试器后立即开始记录 */
uint32_t EvtRec_Init(void);

/* 事件开始/结束（用于测量耗时），slot 0~15，Start/Stop 用同一 slot */
uint32_t EvtRec_EventStartA(uint32_t slot);
uint32_t EvtRec_EventStopA(uint32_t slot);
uint32_t EvtRec_EventStartB(uint32_t slot);   /* 嵌套层级 B，可与 A 嵌套使用 */
uint32_t EvtRec_EventStopB(uint32_t slot);
/* B 级"配对门控"版本：只有 Start 过才允许记录 Stop，避免孤立 Stop 事件 */
uint32_t EvtRec_EventStartBGated(uint32_t slot);
uint32_t EvtRec_EventStopBGated(uint32_t slot);
uint32_t EvtRec_EventStartC(uint32_t slot);   /* 嵌套层级 C */
uint32_t EvtRec_EventStopC(uint32_t slot);
uint32_t EvtRec_EventStartD(uint32_t slot);   /* 嵌套层级 D */
uint32_t EvtRec_EventStopD(uint32_t slot);

/* 记录事件 + 参数 */
uint32_t EvtRec_Record2(uint32_t id, uint32_t val1, uint32_t val2);
uint32_t EvtRec_Record4(uint32_t id, uint32_t val1, uint32_t val2, uint32_t val3, uint32_t val4);

/* 按等级与组件范围使能 / 关闭事件记录 */
uint32_t EvtRec_Enable(uint32_t recording, uint32_t comp_start, uint32_t comp_end);
uint32_t EvtRec_Disable(uint32_t recording, uint32_t comp_start, uint32_t comp_end);

#ifdef __cplusplus
}
#endif

#endif /* EVENT_RECORDER_H */
