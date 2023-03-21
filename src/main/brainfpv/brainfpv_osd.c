/**
 ******************************************************************************
 * @addtogroup OnScreenDisplay OSD Module
 * @brief Process OSD information
 *
 *
 * @file       brainfpv_osd.c
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010-2014.
 * @brief      OSD gen module, handles OSD draw. Parts from CL-OSD and SUPEROSD projects
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */


#include <string.h>
#include <math.h>

#include "platform.h"

#if defined(USE_BRAINFPV_OSD)

#include "ch.h"
#include "brainfpv_osd.h"
#include "video.h"
#include "images.h"
#include "osd_utils.h"
#include "auto_sync_threshold.h"

#include "common/maths.h"
#include "common/axis.h"
#include "common/color.h"
#include "common/utils.h"
#include "common/printf.h"
#include "common/typeconversion.h"
#include "common/log.h"

#include "drivers/sensor.h"
#include "drivers/system.h"
#include "drivers/serial.h"
#include "drivers/timer.h"
#include "drivers/time.h"
#include "drivers/light_led.h"
#include "drivers/light_ws2811strip.h"
#include "drivers/sound_beeper.h"
#include "drivers/max7456.h"
#include "drivers/osd.h"
#include "drivers/display.h"
#include "drivers/osd_symbols.h"

#include "sensors/sensors.h"
#include "sensors/boardalignment.h"
#include "sensors/compass.h"
#include "sensors/acceleration.h"
#include "sensors/barometer.h"
#include "sensors/gyro.h"
#include "sensors/battery.h"

#include "navigation/navigation.h"
#include "navigation/navigation_private.h"

#include "io/flashfs.h"
#include "io/gps.h"
#include "io/ledstrip.h"
#include "io/serial.h"
#include "io/beeper.h"
#include "io/osd.h"
#include "io/displayport_max7456.h"

#include "telemetry/telemetry.h"

#include "flight/mixer.h"
#include "flight/failsafe.h"
#include "flight/imu.h"

#include "config/feature.h"
#include "config/parameter_group.h"
#include "config/parameter_group_ids.h"
#include "settings_generated.h"

//#include "pg/pg_ids.h"
//#include "pg/vcd.h"
//#include "pg/max7456.h"

#include "fc/rc_controls.h"
#include "rx/rx.h"
#include "rx/crsf.h"
#include "fc/runtime_config.h"
#include "fc/rc_modes.h"

//#define OSD_SHOW_DRAW_TIME

#define SW_BLINK_CYCLE_MS 200 // 200ms on / 200ms off

PG_REGISTER_WITH_RESET_TEMPLATE(bfOsdConfig_t, bfOsdConfig, PG_BRAINFPV_OSD_CONFIG, 0);

#if !defined(BRAINFPV_OSD_WHITE_LEVEL_DEFAULT)
#define BRAINFPV_OSD_WHITE_LEVEL_DEFAULT SETTING_BRAINFPV_OSD_WHITE_LEVEL_DEFAULT
#endif

#if !defined(BRAINFPV_OSD_BLACK_LEVEL_DEFAULT)
#define BRAINFPV_OSD_BLACK_LEVEL_DEFAULT SETTING_BRAINFPV_OSD_BLACK_LEVEL_DEFAULT
#endif

#if defined(USE_BRAINFPV_AUTO_SYNC_THRESHOLD)
#define BRAINFPV_OSD_SYNC_TH_MODE_DEFAULT SYNC_THRESHOLD_AUTO
#else
#define BRAINFPV_OSD_SYNC_TH_MODE_DEFAULT SYNC_THRESHOLD_MANUAL
#endif

PG_RESET_TEMPLATE(bfOsdConfig_t, bfOsdConfig,
  .sync_threshold = BRAINFPV_OSD_SYNC_TH_DEFAULT,
  .white_level    = BRAINFPV_OSD_WHITE_LEVEL_DEFAULT,
  .black_level    = BRAINFPV_OSD_BLACK_LEVEL_DEFAULT,
  .x_offset       = -3,
  .x_scale        = 8,
  .sbs_3d_enabled = 0,
  .sbs_3d_right_eye_offset = 30,
  .font = 0,
  .ir_system = 0,
  .ir_trackmate_id = 0,
  .ir_ilap_id = 0,
  .ahi_steps = 2,
  .altitude_scale = 1,
  .speed_scale = 1,
  .radar_max_dist_m = 500,
  .sticks_display = 0,
  .show_logo_on_arm = 1,
  .show_pilot_logo = 1,
  .invert = 0,
  .center_mark_offset = 0,
  .sync_threshold_mode = BRAINFPV_OSD_SYNC_TH_MODE_DEFAULT,
);

void video_qspi_enable(void);
extern binary_semaphore_t onScreenDisplaySemaphore;

extern displayPort_t max7456DisplayPort;

extern bool cmsInMenu;
bool brainfpv_user_avatar_set = false;
bool osd_arming_or_stats = false;
uint32_t osd_draw_time_ms;
bool hide_blinking_items;

#if defined(USE_BRAINFPV_AUTO_SYNC_THRESHOLD)
extern bool useAutoSyncThreshold;
#endif

static void simple_artificial_horizon(int16_t roll, int16_t pitch, int16_t x, int16_t y,
        int16_t width, int16_t height, int8_t max_pitch,
        uint8_t n_pitch_steps);
void draw_stick(int16_t x, int16_t y, int16_t horizontal, int16_t vertical);


/*******************************************************************************/
// MAX7456 Emulation
#define MAX_X(x) (x * 12)
#define MAX_Y(y) (y * 18)

//uint16_t maxScreenSize = VIDEO_BUFFER_CHARS_PAL;


static uint8_t current_font(void)
{
    uint8_t font = bfOsdConfig()->font;

    if (font >= NUM_FONTS) {
        font = NUM_FONTS - 1;
    }

    return font;
}

void max7456Init(const videoSystem_e videoSystem)
{
    (void)videoSystem;
}

void max7456Update(void)
{
}

void max7456Invert(bool invert)
{
    (void)invert;
}

void max7456Brightness(uint8_t black, uint8_t white)
{
    (void)black;
    (void)white;
}

bool max7456DmaInProgress(void)
{
    return false;
}

void max7456DrawScreenPartial(void)
{}


void max7456ReadNvm(uint16_t char_address, osdCharacter_t *chr)
{
    (void)char_address; (void)chr;
}

void max7456WriteNvm(uint16_t char_address, const osdCharacter_t *chr)
{
    (void)char_address; (void)chr;
}

uint16_t max7456GetScreenSize(void)
{

    if (Video_GetType() == VIDEO_TYPE_NTSC) {
        return MAX7456_BUFFER_CHARS_NTSC;
    }
    else {
        return MAX7456_BUFFER_CHARS_PAL;
    }
}

uint8_t max7456GetRowsCount(void)
{
    if (Video_GetType() == VIDEO_TYPE_NTSC) {
        return MAX7456_LINES_NTSC;
    }
    else {
        return MAX7456_LINES_PAL;
    }
}

void max7456Write(uint8_t x, uint8_t y, const char *buff, uint8_t mode)
{
    if (hide_blinking_items && (mode & MAX7456_MODE_BLINK)) {
        return;
    }

    draw_string(buff, MAX_X(x), MAX_Y(y), 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, current_font());
}

void max7456WriteChar(uint8_t x, uint8_t y, uint16_t c, uint8_t mode)
{
    if (hide_blinking_items && (mode & MAX7456_MODE_BLINK)) {
        return;
    }
    char buff[2] = {c, 0};
    draw_string(buff, MAX_X(x), MAX_Y(y), 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, current_font());
}

void max7456ClearScreen(void)
{
    clearGraphics();
}

void  max7456RefreshAll(void)
{
}

static uint8_t dummyBuffer[MAX7456_BUFFER_CHARS_PAL + 40];
uint8_t* max7456GetScreenBuffer(void)
{
    return dummyBuffer;
}

bool max7456ReadChar(uint8_t x, uint8_t y, uint16_t *c, uint8_t *mode)
{
    (void)x;
    (void)y;
    (void)c;
    (void)mode;

    return true;
}

/*******************************************************************************/


#if defined(BRAINFPV_OSD_USE_STM32CMP)

#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_comp.h"

DAC_HandleTypeDef hdac_video_cmp;
COMP_HandleTypeDef hcomp_video_cmp;

static void Error_Handler(void) { while (1) { } }

static void brainFpvOsdInitStm32Cmp(void)
{
    DAC_ChannelConfTypeDef sConfig;

    __HAL_RCC_DAC12_CLK_ENABLE();
    __HAL_RCC_COMP12_CLK_ENABLE();

    IO_t cmp_input = IOGetByTag(IO_TAG(BRAINFPV_OSD_STM32CMP_CMP_INPUT_PIN));
    IO_t cmp_output = IOGetByTag(IO_TAG(BRAINFPV_OSD_STM32CMP_CMP_OUTPUT_PIN));

    IOInit(cmp_input, OWNER_OSD, 0, 0);
    IOConfigGPIO(cmp_input, IO_CONFIG(GPIO_MODE_ANALOG, 0, GPIO_NOPULL));

    IOInit(cmp_output, OWNER_OSD, 0, 0);
    IOConfigGPIOAF(cmp_output, IOCFG_AF_PP, GPIO_AF13_COMP2);

    hdac_video_cmp.Instance = BRAINFPV_OSD_STM32CMP_DAC_INSTANCE;
    if (HAL_DAC_Init(&hdac_video_cmp) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
    sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_ENABLE;
    sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
    if (HAL_DAC_ConfigChannel(&hdac_video_cmp, &sConfig, DAC_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    hcomp_video_cmp.Instance = BRAINFPV_OSD_STM32CMP_CMP_INSTANCE;
    hcomp_video_cmp.Init.InvertingInput = COMP_INPUT_MINUS_DAC1_CH1;
    hcomp_video_cmp.Init.NonInvertingInput = COMP_INPUT_PLUS_IO1;
    hcomp_video_cmp.Init.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
    hcomp_video_cmp.Init.Hysteresis = COMP_HYSTERESIS_NONE;
    hcomp_video_cmp.Init.BlankingSrce = COMP_BLANKINGSRC_NONE;
    hcomp_video_cmp.Init.Mode = COMP_POWERMODE_HIGHSPEED;
    hcomp_video_cmp.Init.WindowMode = COMP_WINDOWMODE_DISABLE;
    hcomp_video_cmp.Init.TriggerMode = COMP_TRIGGERMODE_NONE;

    if (HAL_COMP_Init(&hcomp_video_cmp) != HAL_OK)
    {
        Error_Handler();
    }

    if(HAL_DAC_Start(&hdac_video_cmp, DAC_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    brainFpvOsdSetSyncThresholdMv(4 * bfOsdConfig()->sync_threshold);

    HAL_COMP_Start(&hcomp_video_cmp);
}

void brainFpvOsdSetSyncThresholdMv(uint16_t threshold_mv)
{
    if (hcomp_video_cmp.Instance) {
        HAL_DAC_SetValue(&hdac_video_cmp, DAC_CHANNEL_1, DAC_ALIGN_12B_R, ((uint32_t)threshold_mv * 4095) / ADCVREF);
    }
}
#endif /* defined(BRAINFPV_OSD_USE_STM32CMP) */


void brainFpvOsdInit(void)
{
#if defined(BRAINFPV_OSD_USE_STM32CMP)
    brainFpvOsdInitStm32Cmp();
#endif

#if VIDEO_BITS_PER_PIXEL == 4
    set_text_color(OSD_COLOR_WHITE, OSD_COLOR_BLACK);
    fill_2bit_mask_table();
#endif

    for (uint16_t i=0; i<(image_userlogo.width * image_userlogo.height) / 4; i++) {
        if (image_userlogo.data[i] != 0) {
            brainfpv_user_avatar_set = true;
            break;
        }
    }

    // update number of rows
    chThdSleep(TIME_MS2I(200));

    // Update elements that are being shown
    osdUpdateActiveElements();
}

void brainFpvOsdWelcome(void)
{
    char string_buffer[100];

#define GY (GRAPHICS_BOTTOM / 2 - 30)
    brainFpvOsdMainLogo(GRAPHICS_X_MIDDLE, GY);

    tfp_sprintf(string_buffer, "VERSION: %s", __REVISION__);
    draw_string(string_buffer, GRAPHICS_X_MIDDLE, GRAPHICS_BOTTOM - 60, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, FONT8X10);
    draw_string("MENU: THRT MID YAW LEFT PITCH UP", GRAPHICS_X_MIDDLE, GRAPHICS_BOTTOM - 35, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, FONT8X10);
}

static int32_t getAltitude(void)
{
    int32_t alt;

    alt = getEstimatedActualPosition(Z);

    switch (osdConfig()->units) {
        case OSD_UNIT_IMPERIAL:
            return (alt * 328) / 100; // Convert to feet / 100
        default:
            return alt;               // Already in meters / 100
    }
}

static float getVelocity(void)
{
    float vel = gpsSol.groundSpeed;

    switch (osdConfig()->units) {
        case OSD_UNIT_IMPERIAL:
            return (vel * 224.0f) / 10000.0f; // Convert to mph
        default:
            return (vel * 36.0f) / 1000.0f;   // Convert to kmh
        }
    // Unreachable
    return -1.0f;
}

void osdUpdateLocal(void)
{
    if (bfOsdConfig()->altitude_scale) {
        float altitude = getAltitude() / 100.f;
        osd_draw_vertical_scale(altitude, 100, 1, GRAPHICS_RIGHT - 10, GRAPHICS_Y_MIDDLE, 120, 10, 20, 5, 8, 11, 0);
    }

    if (sensors(SENSOR_GPS)) {
        if (bfOsdConfig()->speed_scale) {
            float speed = getVelocity();
            osd_draw_vertical_scale(speed, 100, -1, GRAPHICS_LEFT + 5, GRAPHICS_Y_MIDDLE, 120, 10, 20, 5, 8, 11, 0);
        }
    }

    if (bfOsdConfig()->sticks_display == 1) {
        // Mode 2
        draw_stick(GRAPHICS_LEFT + 30, GRAPHICS_BOTTOM - 30, rxGetChannelValue(YAW), rxGetChannelValue(THROTTLE));
        draw_stick(GRAPHICS_RIGHT - 30, GRAPHICS_BOTTOM - 30, rxGetChannelValue(ROLL), rxGetChannelValue(PITCH));
    }
    else if (bfOsdConfig()->sticks_display == 2) {
        // Mode 1
        draw_stick(GRAPHICS_LEFT + 30, GRAPHICS_BOTTOM - 30, rxGetChannelValue(YAW), rxGetChannelValue(PITCH));
        draw_stick(GRAPHICS_RIGHT - 30, GRAPHICS_BOTTOM - 30, rxGetChannelValue(ROLL), rxGetChannelValue(THROTTLE));
    }
}

#define IS_HI(X)  (rxGetChannelValue(X) > 1750)
#define IS_LO(X)  (rxGetChannelValue(X) < 1250)
#define IS_MID(X) (rxGetChannelValue(X) > 1250 && rxGetChannelValue(X) < 1750)

void osdRefreshBrainFpv(timeUs_t currentTimeUs);

void brainFpvOsdMain(void) {
#if defined(USE_BRAINFPV_AUTO_SYNC_THRESHOLD)
    uint32_t draw_cnt = 0;
    uint16_t syncTh;
#endif

    while (1) {
        if (chBSemWaitTimeout(&onScreenDisplaySemaphore, TIME_MS2I(500)) == MSG_TIMEOUT) {
            // No trigger received within 500ms, re-enable the video
            video_qspi_enable();

#if defined(USE_BRAINFPV_AUTO_SYNC_THRESHOLD)
            if (useAutoSyncThreshold) {
                syncTh = autoSyncThresholdGet();
                brainFpvOsdSetSyncThresholdMv(syncTh);
            }
#endif

            // Don't do anything. Wait for next interrupt.
            continue;
        }

        osd_draw_time_ms = millis();
        hide_blinking_items = (((osd_draw_time_ms / SW_BLINK_CYCLE_MS) % 2) == 0);
        clearGraphics();

        /* Hide OSD when OSDSW mode is active */
        if (IS_RC_MODE_ACTIVE(BOXOSD))
          continue;

        if (osd_draw_time_ms < 5000) {
            brainFpvOsdWelcome();
        }
        else {
            // draw normal OSD
            uint32_t currentTimeUs = micros();
            osdRefreshBrainFpv(currentTimeUs);
            if (!cmsInMenu && !osd_arming_or_stats){
                osdUpdateLocal();
            }
        }

        // Update elements that are being shown
        osdUpdateActiveElements();

#if defined(USE_BRAINFPV_AUTO_SYNC_THRESHOLD)
        if (useAutoSyncThreshold && (draw_cnt & 0x04)) {
            syncTh = autoSyncThresholdGet();
            brainFpvOsdSetSyncThresholdMv(syncTh);
        }
        //char tmp[30];
        //tfp_sprintf(tmp, "SYNC TH: %d mV", syncTh);
        //draw_string(tmp, GRAPHICS_LEFT, GRAPHICS_BOTTOM - 20, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, FONT8X10);
#endif

#if defined(OSD_SHOW_DRAW_TIME)
        osd_draw_time_ms = millis() - osd_draw_time_ms;
        char string_buffer[20];
        tfp_sprintf(string_buffer, "draw: %lu ms", osd_draw_time_ms);
        draw_string(string_buffer, GRAPHICS_LEFT + 10, GRAPHICS_BOTTOM - 10, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, FONT8X10);
#endif

        // Update number of lines
        if (Video_GetType() == VIDEO_TYPE_NTSC) {
            max7456DisplayPort.rows = MAX7456_LINES_NTSC;
        }
        else {
            max7456DisplayPort.rows = MAX7456_LINES_PAL;
        }

#if defined(USE_BRAINFPV_AUTO_SYNC_THRESHOLD)
        draw_cnt += 1;
#endif
    }
}

void brainFpvOsdArtificialHorizon(void)
{
	uint16_t y_pos = GRAPHICS_Y_MIDDLE + bfOsdConfig()->center_mark_offset;

    simple_artificial_horizon(attitude.values.roll, -1 * attitude.values.pitch,
                              GRAPHICS_X_MIDDLE, y_pos,
                              GRAPHICS_RIGHT * 0.8f, GRAPHICS_BOTTOM, 30,
                              bfOsdConfig()->ahi_steps);
}

#define CENTER_BODY       3
#define CENTER_WING       7
#define CENTER_RUDDER     5
#define PITCH_STEP       10
void brainFpvOsdCenterMark(void)
{
	uint16_t y_pos = GRAPHICS_Y_MIDDLE + bfOsdConfig()->center_mark_offset;

	draw_line_outlined(GRAPHICS_X_MIDDLE - CENTER_WING - CENTER_BODY, y_pos ,
            GRAPHICS_X_MIDDLE - CENTER_BODY, y_pos, 2, 0, OSD_COLOR_WHITE, OSD_COLOR_BLACK);
	draw_line_outlined(GRAPHICS_X_MIDDLE + 1 + CENTER_BODY, y_pos,
            GRAPHICS_X_MIDDLE + 1 + CENTER_BODY + CENTER_WING, y_pos, 0, 2, OSD_COLOR_WHITE, OSD_COLOR_BLACK);
	draw_line_outlined(GRAPHICS_X_MIDDLE, y_pos - CENTER_RUDDER - CENTER_BODY, GRAPHICS_X_MIDDLE,
            y_pos - CENTER_BODY, 2, 0, OSD_COLOR_WHITE, OSD_COLOR_BLACK);
}


static void simple_artificial_horizon(int16_t roll, int16_t pitch, int16_t x, int16_t y,
        int16_t width, int16_t height, int8_t max_pitch, uint8_t n_pitch_steps)
{
    width /= 2;
    height /= 2;

    float sin_roll = sinf(DECIDEGREES_TO_RADIANS(roll));
    float cos_roll = cosf(DECIDEGREES_TO_RADIANS(roll));

    int pitch_step_offset = pitch / (PITCH_STEP * 10);

    /* how many degrees the "lines" are offset from their ideal pos
         * since we need both, don't do fmodf.. */
    float modulo_pitch =DECIDEGREES_TO_DEGREES(pitch) - pitch_step_offset * 10.0f;

    // roll to pitch transformation
    int16_t pp_x = x + width * ((sin_roll * modulo_pitch) / (float)max_pitch);
    int16_t pp_y = y + height * ((cos_roll * modulo_pitch) / (float)max_pitch);

    int16_t d_x, d_x2; // delta x
    int16_t d_y, d_y2; // delta y

    d_x = cos_roll * width / 2;
    d_y = sin_roll * height / 2;

    d_x = 3 * d_x / 4;
    d_y = 3 * d_y / 4;
    d_x2 = 3 * d_x / 4;
    d_y2 = 3 * d_y / 4;

    int16_t d_x_10 = width * sin_roll * PITCH_STEP / (float)max_pitch;
    int16_t d_y_10 = height * cos_roll * PITCH_STEP / (float)max_pitch;

    int16_t d_x_2 = d_x_10 / 6;
    int16_t d_y_2 = d_y_10 / 6;

    for (int i = (-max_pitch / 10)-1; i<(max_pitch/10)+1; i++) {
        int angle = (pitch_step_offset + i);

        if (angle < -n_pitch_steps) continue;
        if (angle > n_pitch_steps) continue;

        angle *= PITCH_STEP;

        /* Wraparound */
        if (angle > 90) {
            angle = 180 - angle;
        } else if (angle < -90) {
            angle = -180 - angle;
        }

        int16_t pp_x2 = pp_x - i * d_x_10;
        int16_t pp_y2 = pp_y - i * d_y_10;

        char tmp_str[5];

        tfp_sprintf(tmp_str, "%d", angle);

        if (angle < 0) {
            draw_line_outlined_dashed(pp_x2 - d_x2, pp_y2 + d_y2, pp_x2 + d_x2, pp_y2 - d_y2, 2, 2, OSD_COLOR_BLACK, OSD_COLOR_WHITE, 5);
            draw_line_outlined(pp_x2 - d_x2, pp_y2 + d_y2, pp_x2 - d_x2 - d_x_2, pp_y2 + d_y2 - d_y_2, 2, 2, OSD_COLOR_BLACK, OSD_COLOR_WHITE);
            draw_line_outlined(pp_x2 + d_x2, pp_y2 - d_y2, pp_x2 + d_x2 - d_x_2, pp_y2 - d_y2 - d_y_2, 2, 2, OSD_COLOR_BLACK, OSD_COLOR_WHITE);

            draw_string(tmp_str, pp_x2 - d_x - 4, pp_y2 + d_y, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, FONT_OUTLINED8X8);
            draw_string(tmp_str, pp_x2 + d_x + 4, pp_y2 - d_y, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, FONT_OUTLINED8X8);
        } else if (angle > 0) {
            draw_line_outlined(pp_x2 - d_x2, pp_y2 + d_y2, pp_x2 + d_x2, pp_y2 - d_y2, 2, 2, OSD_COLOR_BLACK, OSD_COLOR_WHITE);
            draw_line_outlined(pp_x2 - d_x2, pp_y2 + d_y2, pp_x2 - d_x2 + d_x_2, pp_y2 + d_y2 + d_y_2, 2, 2, OSD_COLOR_BLACK, OSD_COLOR_WHITE);
            draw_line_outlined(pp_x2 + d_x2, pp_y2 - d_y2, pp_x2 + d_x2 + d_x_2, pp_y2 - d_y2 + d_y_2, 2, 2, OSD_COLOR_BLACK, OSD_COLOR_WHITE);

            draw_string(tmp_str, pp_x2 - d_x - 4, pp_y2 + d_y, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, FONT_OUTLINED8X8);
            draw_string(tmp_str, pp_x2 + d_x + 4, pp_y2 - d_y, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, FONT_OUTLINED8X8);
        } else {
            draw_line_outlined(pp_x2 - d_x, pp_y2 + d_y, pp_x2 - d_x / 3, pp_y2 + d_y / 3, 2, 2, OSD_COLOR_BLACK, OSD_COLOR_WHITE);
            draw_line_outlined(pp_x2 + d_x / 3, pp_y2 - d_y / 3, pp_x2 + d_x, pp_y2 - d_y, 2, 2, OSD_COLOR_BLACK, OSD_COLOR_WHITE);
        }
    }
}



#define FIX_RC_RANGE(x) (MIN(MAX(-500, x - 1500), 500))
#define STICK_WIDTH 2
#define STICK_LENGTH 20
#define STICK_BOX_SIZE 4
#define STICK_MOVEMENT_EXTENT (STICK_LENGTH - STICK_BOX_SIZE / 2 + 1)

void draw_stick(int16_t x, int16_t y, int16_t horizontal, int16_t vertical)
{

    draw_filled_rectangle(x - STICK_LENGTH, y - STICK_WIDTH / 2, 2 * STICK_LENGTH, STICK_WIDTH, OSD_COLOR_BLACK);
    draw_filled_rectangle(x - STICK_WIDTH / 2, y - STICK_LENGTH, STICK_WIDTH, 2 * STICK_LENGTH, OSD_COLOR_BLACK);

    draw_hline(x - STICK_LENGTH - 1, x - STICK_WIDTH / 2 -1, y - STICK_WIDTH / 2 - 1, OSD_COLOR_WHITE);
    draw_hline(x - STICK_LENGTH - 1, x - STICK_WIDTH / 2 -1, y + STICK_WIDTH / 2 + 1, OSD_COLOR_WHITE);

    draw_hline(x + STICK_WIDTH / 2 + 1, x + STICK_LENGTH + 1, y - STICK_WIDTH / 2 - 1, OSD_COLOR_WHITE);
    draw_hline(x + STICK_WIDTH / 2 + 1, x + STICK_LENGTH + 1, y + STICK_WIDTH / 2 + 1, OSD_COLOR_WHITE);

    draw_hline(x - STICK_WIDTH / 2 -1, x + STICK_WIDTH / 2 + 1 , y - STICK_LENGTH -1, OSD_COLOR_WHITE);
    draw_hline(x - STICK_WIDTH / 2 -1, x + STICK_WIDTH / 2 + 1 , y + STICK_LENGTH + 1, OSD_COLOR_WHITE);

    draw_vline(x - STICK_WIDTH / 2 - 1, y - STICK_WIDTH / 2 - 1, y - STICK_LENGTH -1, OSD_COLOR_WHITE);
    draw_vline(x + STICK_WIDTH / 2 + 1, y - STICK_WIDTH / 2 - 1, y - STICK_LENGTH -1, OSD_COLOR_WHITE);

    draw_vline(x - STICK_WIDTH / 2 - 1, y + STICK_LENGTH  + 1, y + STICK_WIDTH / 2 + 1, OSD_COLOR_WHITE);
    draw_vline(x + STICK_WIDTH / 2 + 1, y + STICK_LENGTH  + 1, y + STICK_WIDTH / 2 + 1, OSD_COLOR_WHITE);

    draw_vline(x - STICK_LENGTH - 1, y -STICK_WIDTH / 2 -1, y + STICK_WIDTH / 2 + 1, OSD_COLOR_WHITE);
    draw_vline(x + STICK_LENGTH + 1, y -STICK_WIDTH / 2 -1, y + STICK_WIDTH / 2 + 1, OSD_COLOR_WHITE);

    int16_t stick_x =  x + (STICK_MOVEMENT_EXTENT * FIX_RC_RANGE(horizontal)) / 500.f;
    int16_t stick_y =  y - (STICK_MOVEMENT_EXTENT * FIX_RC_RANGE(vertical)) / 500.f;

    draw_filled_rectangle(stick_x - (STICK_BOX_SIZE) / 2 - 1, stick_y - (STICK_BOX_SIZE) / 2 - 1, STICK_BOX_SIZE + 2, STICK_BOX_SIZE + 2, OSD_COLOR_BLACK);
    draw_filled_rectangle(stick_x - (STICK_BOX_SIZE) / 2, stick_y - (STICK_BOX_SIZE) / 2, STICK_BOX_SIZE, STICK_BOX_SIZE, OSD_COLOR_WHITE);
}


void brainFpvOsdUserLogo(uint16_t x, uint16_t y)
{
    draw_image(MAX_X(x) - image_userlogo.width / 2, MAX_Y(y) - image_userlogo.height / 2, &image_userlogo);
}

void brainFpvOsdMainLogo(uint16_t x, uint16_t y)
{
    draw_image(x - image_mainlogo.width / 2, y - image_mainlogo.height / 2, &image_mainlogo);
}

const point_t HOME_ARROW[] = {
    {
        .x = 0,
        .y = -10,
    },
    {
        .x = 9,
        .y = 1,
    },
    {
        .x = 3,
        .y = 1,
    },
    {
        .x = 3,
        .y = 8,
    },
    {
        .x = -3,
        .y = 8,
    },
    {
        .x = -3,
        .y = 1,
    },
    {
        .x = -9,
        .y = 1,
    }
};

void brainFfpvOsdHomeArrow(int16_t home_dir, uint16_t x, uint16_t y)
{
    x = MAX_X(x);
    y = MAX_Y(y);
    draw_polygon(x, y, home_dir, HOME_ARROW, 7, 0, 1);
}


const point_t UAV_SYM[] = {
    {
        .x = 0,
        .y = -3,
    },
    {
        .x = 7,
        .y = 3,
    },
    {
        .x = 0,
        .y = -1,
    },
    {
        .x = -7,
        .y = 3,
    }
};


#define MAP_MAX_DIST_PX 70
void brainFpvRadarMap(void)
{
    uint16_t x, y;
	uint16_t y_pos = GRAPHICS_Y_MIDDLE + bfOsdConfig()->center_mark_offset;

    //===========================================================================================
    // Draw Home location on map

    float distance = GPS_distanceToHome;

    if (distance > bfOsdConfig()->radar_max_dist_m) {
        distance = bfOsdConfig()->radar_max_dist_m;
    }

    float distance_px = MAP_MAX_DIST_PX * (float)distance / (float)bfOsdConfig()->radar_max_dist_m;

    // don't draw home if we are very close to home
    if (distance_px >= 1.0f) {
        // Get home direction relative to UAV
        int16_t home_dir = GPS_directionToHome - DECIDEGREES_TO_DEGREES(attitude.values.yaw);

        x = GRAPHICS_X_MIDDLE + roundf(distance_px * sin_approx(DEGREES_TO_RADIANS(home_dir)));
        y = y_pos - roundf(distance_px * cos_approx(DEGREES_TO_RADIANS(home_dir)));

        // draw H to indicate home
        draw_string("H", x + 1, y - 3, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, FONT_OUTLINED8X8);
    }

    //===========================================================================================
    // Draw Radar POI's
    for (uint8_t i = 0; i < RADAR_MAX_POIS; i++) {
        if (radar_pois[i].gps.lat == 0 || radar_pois[i].gps.lon == 0 || radar_pois[i].state >= 2) {
            // skip
            continue;
        }
        // Get NED of other UAV relative to Home
        fpVector3_t poi;
        geoConvertGeodeticToLocalOrigin(&poi, &radar_pois[i].gps, GEO_ALT_ABSOLUTE);

        distance = calculateDistanceToDestination(&poi) / 100.f; // In meters

        if (distance > (float)bfOsdConfig()->radar_max_dist_m) {
            distance = bfOsdConfig()->radar_max_dist_m;
        }

        int16_t direction = CENTIDEGREES_TO_DEGREES(calculateBearingToDestination(&poi)) - DECIDEGREES_TO_DEGREES(attitude.values.yaw);
        distance_px = MAP_MAX_DIST_PX * distance / (float)bfOsdConfig()->radar_max_dist_m;

        x = GRAPHICS_X_MIDDLE + roundf(distance_px * sin_approx(DEGREES_TO_RADIANS(direction)));
        y = y_pos - roundf(distance_px * cos_approx(DEGREES_TO_RADIANS(direction)));

        // Toggle between showing UAV with heading and name / alt difference
        if ((osd_draw_time_ms / 1000) % 2 == 0) {
            const navEstimatedPosVel_t *posvel = navGetCurrentActualPositionAndVelocity();
            int16_t delta_alt = poi.z - posvel->pos.z;

            if ((osd_unit_e)osdConfig()->units == OSD_UNIT_IMPERIAL) {
                delta_alt = CENTIMETERS_TO_FEET(delta_alt);
            }
            else {
                delta_alt = CENTIMETERS_TO_METERS(delta_alt);
            }

            char buff[20];
            tfp_sprintf(buff, "%c %d", 'A' + i, delta_alt);
            draw_string(buff, x, y - 3, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, FONT_OUTLINED8X8);
        }
        else {
            direction = radar_pois[i].heading - DECIDEGREES_TO_DEGREES(attitude.values.yaw);
            draw_polygon(x, y, direction, UAV_SYM, 4, OSD_COLOR_WHITE, OSD_COLOR_BLACK);
        }
    }

    //===========================================================================================
    // Draw waypoints
    for (uint8_t i = 1; i <= getWaypointCount(); i++) {
    	navWaypoint_t wpData;
        gpsLocation_t wpLLH;
        fpVector3_t poi;

        getWaypoint(i, &wpData);

        wpLLH.lat = wpData.lat;
        wpLLH.lon = wpData.lon;
        wpLLH.alt = wpData.alt;

        geoConvertGeodeticToLocalOrigin(&poi, &wpLLH, GEO_ALT_ABSOLUTE);

        distance = calculateDistanceToDestination(&poi) / 100.f; // In meters

        if (distance > (float)bfOsdConfig()->radar_max_dist_m) {
            distance = bfOsdConfig()->radar_max_dist_m;
        }

        int16_t direction = CENTIDEGREES_TO_DEGREES(calculateBearingToDestination(&poi)) - DECIDEGREES_TO_DEGREES(attitude.values.yaw);
        distance_px = MAP_MAX_DIST_PX * distance / (float)bfOsdConfig()->radar_max_dist_m;

        x = GRAPHICS_X_MIDDLE + roundf(distance_px * sin_approx(DEGREES_TO_RADIANS(direction)));
        y = y_pos - roundf(distance_px * cos_approx(DEGREES_TO_RADIANS(direction)));

        char buff[10];
        tfp_sprintf(buff, "%d", i);
        draw_string(buff, x, y - 3, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, FONT_OUTLINED8X8);
    }

}




static int round_up_multiple(int16_t num, int16_t multiple)
{
    int16_t pos = (int16_t)(num >= 0);
    return ((num + pos * (multiple - 1)) / multiple) * multiple;
}

#define HGRAPH_WIDTH_PX       90
#define HGRAPH_DEG_PER_PX      2
#define HGRAPH_WIDTH_DEG     (HGRAPH_WIDTH_PX * HGRAPH_DEG_PER_PX)
#define HGRAPH_SUB_TICKS_DEG  15
#define HGRAPH_MAIN_TICKS_DEG 45 /* must be multiple of HGRAPH_SUB_TICKS_DEG */
#define HGRAPH_MAIN_TICK_LEN   4
#define HGRAPH_SUB_TICK_LEN    2

void brainFpvOsdHeadingGraph(uint16_t x, uint16_t y)
{
    int16_t xp;
    int16_t pos_g_360;
    char tmp_str[2] = {0, 0};
    x = MAX_X(x);
    y = MAX_Y(y);

    int16_t heading = DECIDEGREES_TO_DEGREES(attitude.values.yaw);

    int16_t edge_g_left = heading - HGRAPH_WIDTH_DEG / 2;
    int16_t edge_g_right = heading + HGRAPH_WIDTH_DEG / 2;

    // Draw ticks
    int16_t pos_g = round_up_multiple(edge_g_left, HGRAPH_SUB_TICKS_DEG);
    while (pos_g <= edge_g_right) {
        xp = x + (pos_g - edge_g_left) / HGRAPH_DEG_PER_PX;

        if (pos_g % HGRAPH_MAIN_TICKS_DEG == 0) {
            // major tick
            draw_vline(xp, y, y + HGRAPH_MAIN_TICK_LEN, OSD_COLOR_WHITE);

            pos_g_360 = pos_g;
            if (pos_g_360 < 0) {
                pos_g_360 += 360;
            }
            if (pos_g_360 >= 360) {
                pos_g_360 -= 360;
            }
            tmp_str[0] = 0;
            switch (pos_g_360) {
                case 0:
                    tmp_str[0] = 'N';
                    break;
                case 90:
                    tmp_str[0] = 'E';
                    break;
                case 180:
                    tmp_str[0] = 'S';
                    break;
                case 270:
                    tmp_str[0] = 'W';
                    break;
                default:
                    break;
            }
            if (tmp_str[0]) {
                draw_string(tmp_str, xp + 1, y + HGRAPH_MAIN_TICK_LEN + 3, 0, 0,
                        TEXT_VA_TOP, TEXT_HA_CENTER, FONT_OUTLINED8X8);
            }
        }
        else {
            // minor tick
            draw_vline(xp, y, y + HGRAPH_SUB_TICK_LEN, OSD_COLOR_BLACK);
        }

        pos_g += HGRAPH_SUB_TICKS_DEG;
    }

    // Draw center mark
    xp = x + HGRAPH_WIDTH_PX / 2;
    draw_hline(xp - 2, xp + 2, y, OSD_COLOR_WHITE);
    draw_line(xp - 2, y, xp, y + 2, OSD_COLOR_WHITE);
    draw_line(xp + 2, y, xp, y + 2, OSD_COLOR_WHITE);
}

#endif /* USE_BRAINFPV_OSD */
