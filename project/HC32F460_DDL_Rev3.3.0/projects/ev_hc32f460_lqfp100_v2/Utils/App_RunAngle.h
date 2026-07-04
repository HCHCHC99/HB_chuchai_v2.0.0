#ifndef __APP_RUNANGLE_H__
#define __APP_RUNANGLE_H__

#include <stdint.h>
#include <stdbool.h>
#include "param_manager.h"

/*=============================================================================
 * Absolute angle tracking module
 *
 * Tracks angle offset from a user-defined zero point using Hall pulse
 * accumulator (g_s32HallPulseAccum). Updates every 1ms via RunAngle_Update(),
 * independent of dev_rturn calibration state.
 *
 * RAM:  s_abs_offset_x10  — real-time value, updated by pulse delta every ms
 * Flash: g_AbsAngle.abs_offset_x10 — persisted on save, restored on power-up
 *
 * Registers:
 *   0x3716/0x3717  Read RAM offset (int32_t, 0.1 deg)
 *   0x3718/0x3719  Read Flash offset (int32_t, 0.1 deg)
 *   0x371A         Write: 0=set zero, 1=save to Flash
 *============================================================================*/

/*=============================================================================
 * Flash record (sector 55, ~24 bytes per record)
 *============================================================================*/
#pragma pack(4)
typedef struct {
    uint32_t magic;             /* 0xAB5AAB5A */
    uint32_t sequence_id;
    uint32_t erase_count;
    int32_t  abs_offset_x10;    /* persisted absolute angle offset (0.1 deg) */
    uint32_t checksum;          /* CRC32 */
    uint32_t tail_magic;        /* 0xBA5ABA5A */
} AbsAngleRecord_t;
#pragma pack()

#define ABS_ANGLE_MAGIC_HEAD   0xAB5AAB5A
#define ABS_ANGLE_MAGIC_TAIL   0xBA5ABA5A

/* Command values for 0x371A */
#define ABS_CMD_SET_ZERO       0x0000U   /* set current position as absolute zero */
#define ABS_CMD_SAVE           0x0001U   /* save current RAM offset to Flash */
#define ABS_CMD_GOTO_ZERO      0x0002U   /* auto-rotate to absolute zero */

/* Goto-zero stop threshold (0.1 deg), absolute offset <= this → stop */
#define ABS_GOTO_ZERO_THRESH_X10   1

/*=============================================================================
 * Public API
 *============================================================================*/

/** @brief Initialize: load Flash offset into RAM, snapshot pulse accumulator */
void RunAngle_Init(void);

/** @brief Called every 1ms from main loop — updates s_abs_offset_x10 from pulse delta */
void RunAngle_Update(void);

/** @brief Get current RAM offset (int32_t, 0.1 deg) */
int32_t RunAngle_GetOffset_x10(void);

/** @brief Get Flash persisted offset (int32_t, 0.1 deg) */
int32_t RunAngle_GetFlashOffset_x10(void);

/** @brief Handle command write to 0x371A: 0=set zero, 1=save to Flash, 2=goto zero */
void RunAngle_Cmd(uint16_t cmd);

/** @brief Start auto-rotate to absolute zero (publishes TOPIC_MANUAL_RS485) */
void RunAngle_GotoZero(void);

/*=============================================================================
 * Global — Keil Watch
 *============================================================================*/
extern AbsAngleRecord_t g_AbsAngle;

#endif /* __APP_RUNANGLE_H__ */
