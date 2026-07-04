#ifndef __APP_RUNANGLE_H__
#define __APP_RUNANGLE_H__

#include <stdint.h>
#include <stdbool.h>
#include "param_manager.h"

/*=============================================================================
 * Absolute angle tracking module
 *
 * Registers:
 *   0x3716/0x3717  Read RAM offset (int32_t, 0.1 deg)
 *   0x3718/0x3719  Read Flash offset (int32_t, 0.1 deg)
 *   0x371A         Write: 0=set zero, 1=save, 2=goto zero
 *   0x371B         R/W: goto-zero threshold (uint16_t, 0.1 deg, default 1)
 *============================================================================*/

/*=============================================================================
 * Flash record (sector 55)
 *============================================================================*/
#pragma pack(4)
typedef struct {
    uint32_t magic;                 /* 0xAB5AAB5A */
    uint32_t sequence_id;
    uint32_t erase_count;
    int32_t  abs_offset_x10;        /* persisted absolute angle offset (0.1 deg) */
    uint16_t goto_zero_thresh_x10;  /* goto-zero stop threshold (0.1 deg, default 1, max 200) */
    uint16_t reserved;              /* padding to 4-byte alignment */
    uint32_t checksum;              /* CRC32 */
    uint32_t tail_magic;            /* 0xBA5ABA5A */
} AbsAngleRecord_t;
#pragma pack()

#define ABS_ANGLE_MAGIC_HEAD   0xAB5AAB5A
#define ABS_ANGLE_MAGIC_TAIL   0xBA5ABA5A

/* Default threshold: 0.1 deg */
#define ABS_THRESH_DEFAULT_X10    1
#define ABS_THRESH_MIN_X10        0
#define ABS_THRESH_MAX_X10      200   /* 20.0 deg max */

/* Command values for 0x371A */
#define ABS_CMD_SET_ZERO       0x0000U
#define ABS_CMD_SAVE           0x0001U
#define ABS_CMD_GOTO_ZERO      0x0002U

/*=============================================================================
 * Public API
 *============================================================================*/

void RunAngle_Init(void);
void RunAngle_Update(void);
int32_t RunAngle_GetOffset_x10(void);
int32_t RunAngle_GetFlashOffset_x10(void);
void RunAngle_Cmd(uint16_t cmd);
void RunAngle_GotoZero(void);

/** @brief Set goto-zero threshold (called on Modbus write 0x371B, hot-reload) */
void RunAngle_SetThreshold(uint16_t thresh_x10);

extern AbsAngleRecord_t g_AbsAngle;

#endif /* __APP_RUNANGLE_H__ */
