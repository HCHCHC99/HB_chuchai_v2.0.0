/**
 *******************************************************************************
 * @file  App_RunAngle.c
 * @brief Absolute angle offset — tracks angular displacement from user zero
 *        using Hall pulse delta, persisted to Flash via param_manager.
 *******************************************************************************
 */

#include "App_RunAngle.h"
#include "App_Params.h"         /* g_s32HallPulseAccum, g_AppParam */
#include "param_manager.h"
#include "EventBus.h"           /* TOPIC_MANUAL_RS485 */
#include "dev_motor.h"          /* MotorManualIOEvent_t, CMD_TYPE_*, DIR_* */
#include <stdlib.h>             /* abs() */
#include <string.h>

/*=============================================================================
 * RAM state
 *============================================================================*/
static int32_t s_abs_offset_x10 = 0;      /* real-time absolute angle offset (0.1 deg) */
static int32_t s_last_accum     = 0;      /* snapshot of g_s32HallPulseAccum at last update */
static bool    s_goto_zero_active = false;/* goto-zero in progress */
static bool    s_goto_zero_initial_sign;  /* sign of offset when goto-zero started */

AbsAngleRecord_t g_AbsAngle;              /* Flash mirror */

/*=============================================================================
 * Runtime + Config for param_manager multi-instance (sector 55)
 *============================================================================*/
static Param_Runtime_t s_Runtime;

static const Param_Config_t s_Config = {
    .pParamBuf      = &g_AbsAngle,
    .paramSize      = sizeof(AbsAngleRecord_t),
    .magicHead      = ABS_ANGLE_MAGIC_HEAD,
    .magicTail      = ABS_ANGLE_MAGIC_TAIL,
    .checksumOffset = offsetof(AbsAngleRecord_t, checksum),
    .seqOffset      = offsetof(AbsAngleRecord_t, sequence_id),
    .eraseCntOffset = offsetof(AbsAngleRecord_t, erase_count),
    .secStart       = 55,
    .secEnd         = 55,
};

/*=============================================================================
 * Defaults callback
 *============================================================================*/
static void RunAngle_SetDefaults(void)
{
    g_AbsAngle.magic                 = ABS_ANGLE_MAGIC_HEAD;
    g_AbsAngle.sequence_id           = 0;
    g_AbsAngle.erase_count           = 0;
    g_AbsAngle.abs_offset_x10        = 0;
    g_AbsAngle.goto_zero_thresh_x10  = ABS_THRESH_DEFAULT_X10;
    g_AbsAngle.reserved              = 0;
    g_AbsAngle.checksum              = 0;
    g_AbsAngle.tail_magic            = ABS_ANGLE_MAGIC_TAIL;
}

/*=============================================================================
 * deg_x10 per pulse = 3600 / (pole_pairs * hall_count * 2 * ratio)
 *   pole_pairs from g_AppParam.motor_hall_pole_pairs (default 3)
 *   hall_count = 2 (hardcoded)
 *   ratio = g_AppParam.rturn_reduction_ratio / 10.0
 *============================================================================*/
static float DegPerPulse_x10(void)
{
    uint8_t pp = (uint8_t)g_AppParam.motor_hall_pole_pairs;
    float   ratio = (float)g_AppParam.rturn_reduction_ratio / 10.0f;

    if (pp == 0 || g_AppParam.rturn_reduction_ratio == 0) {
        return 0.0f;
    }
    return 3600.0f / (float)(pp * 2 * 2) / ratio;
}

/*=============================================================================
 * Public API
 *============================================================================*/

void RunAngle_Init(void)
{
    /* Load latest valid record from Flash (or write defaults on first boot) */
    Param_Init(&s_Config, &s_Runtime, RunAngle_SetDefaults);

    /* Validate threshold */
    if (g_AbsAngle.goto_zero_thresh_x10 < ABS_THRESH_MIN_X10
        || g_AbsAngle.goto_zero_thresh_x10 > ABS_THRESH_MAX_X10) {
        g_AbsAngle.goto_zero_thresh_x10 = ABS_THRESH_DEFAULT_X10;
    }

    /* RAM = Flash value at power-up */
    s_abs_offset_x10 = g_AbsAngle.abs_offset_x10;

    /* Snapshot current pulse accumulator so subsequent delta starts from zero */
    __disable_irq();
    s_last_accum = g_s32HallPulseAccum;
    __enable_irq();

    MAIN_D("[ABSA] Init: ram=%ld (0.1deg), flash=%ld, accum_base=%ld\r\n",
           (long)s_abs_offset_x10, (long)g_AbsAngle.abs_offset_x10, (long)s_last_accum);
}

void RunAngle_Update(void)
{
    int32_t accum;
    __disable_irq();
    accum = g_s32HallPulseAccum;
    __enable_irq();

    int32_t delta = accum - s_last_accum;

    /* Update angle offset from pulse delta (if any) */
    if (delta != 0) {
        s_last_accum = accum;

        float k = DegPerPulse_x10();
        if (k != 0.0f) {
            s_abs_offset_x10 += (int32_t)((float)delta * k);
        }
    }

    /* Goto-zero: once motor moves past zero or enters threshold, stop.
     * Only one direction command was sent at start — no continuous re-issue. */
    if (s_goto_zero_active) {
        if (abs(s_abs_offset_x10) <= g_AbsAngle.goto_zero_thresh_x10
            || (s_abs_offset_x10 > 0) != s_goto_zero_initial_sign) {
            MotorManualIOEvent_t ev;
            memset(&ev, 0, sizeof(ev));
            ev.type = CMD_TYPE_STOP;        /* ESTOP (same as CTRL_CMD_ESTOP) */
            ev.dir  = DIR_NONE;
            EventBus_Publish(TOPIC_MANUAL_RS485, &ev);
            s_goto_zero_active = false;
            MAIN_D("[ABSA] Goto-zero complete: offset=%ld\r\n", (long)s_abs_offset_x10);
        }
    }
}

int32_t RunAngle_GetOffset_x10(void)
{
    return s_abs_offset_x10;
}

int32_t RunAngle_GetFlashOffset_x10(void)
{
    return g_AbsAngle.abs_offset_x10;
}

void RunAngle_Cmd(uint16_t cmd)
{
    if (cmd == ABS_CMD_SET_ZERO) {
        /* Set current position as absolute zero */
        s_abs_offset_x10 = 0;
        g_AbsAngle.abs_offset_x10 = 0;
        Param_Save(&s_Config, &s_Runtime);

        MAIN_D("[ABSA] Zero set: RAM=0, Flash=0\r\n");
    } else if (cmd == ABS_CMD_SAVE) {
        /* Persist current RAM value to Flash */
        g_AbsAngle.abs_offset_x10 = s_abs_offset_x10;
        Param_Save(&s_Config, &s_Runtime);

        MAIN_D("[ABSA] Saved: ram=%ld -> flash=%ld\r\n",
               (long)s_abs_offset_x10, (long)g_AbsAngle.abs_offset_x10);
    } else if (cmd == ABS_CMD_GOTO_ZERO) {
        RunAngle_GotoZero();
    }
}

void RunAngle_GotoZero(void)
{
    MotorManualIOEvent_t ev;
    memset(&ev, 0, sizeof(ev));

    if (s_abs_offset_x10 > g_AbsAngle.goto_zero_thresh_x10) {
        /* Forward of zero → REV to go back */
        ev.dir  = DIR_REV;
        ev.type = CMD_TYPE_RUN_REV;
        s_goto_zero_active = true;
        s_goto_zero_initial_sign = true;   /* positive → expect sign flip to false */
        MAIN_D("[ABSA] Goto-zero: offset=%ld (>0), REV to zero\r\n", (long)s_abs_offset_x10);
    } else if (s_abs_offset_x10 < -g_AbsAngle.goto_zero_thresh_x10) {
        /* Reverse of zero → FWD to go forward */
        ev.dir  = DIR_FWD;
        ev.type = CMD_TYPE_RUN_FWD;
        s_goto_zero_active = true;
        s_goto_zero_initial_sign = false;  /* negative → expect sign flip to true */
        MAIN_D("[ABSA] Goto-zero: offset=%ld (<0), FWD to zero\r\n", (long)s_abs_offset_x10);
    } else {
        /* Already at zero — nothing to do */
        MAIN_D("[ABSA] Goto-zero: offset=%ld, already at zero\r\n", (long)s_abs_offset_x10);
        return;
    }

    EventBus_Publish(TOPIC_MANUAL_RS485, &ev);
}

void RunAngle_SetThreshold(uint16_t thresh_x10)
{
    /* Clamp to valid range */
    if (thresh_x10 < ABS_THRESH_MIN_X10) {
        thresh_x10 = ABS_THRESH_MIN_X10;
    } else if (thresh_x10 > ABS_THRESH_MAX_X10) {
        thresh_x10 = ABS_THRESH_MAX_X10;
    }

    g_AbsAngle.goto_zero_thresh_x10 = thresh_x10;
    Param_Save(&s_Config, &s_Runtime);

    MAIN_D("[ABSA] Threshold set: %u (0.1 deg)\r\n", (unsigned int)thresh_x10);
}

void RunAngle_OnCalibration(void)
{
    __disable_irq();
    s_abs_offset_x10 = 0;
    s_last_accum = g_s32HallPulseAccum;   /* accum was just zeroed by dev_rturn */
    __enable_irq();

    g_AbsAngle.abs_offset_x10 = 0;
    Param_Save(&s_Config, &s_Runtime);    /* persist to Flash sector 55 */

    MAIN_D("[ABSA] Calibration zero: RAM=0, Flash=0\r\n");
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
