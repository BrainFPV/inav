/*
 * This file is part of Cleanflight and Betaflight.
 *
 * Cleanflight and Betaflight are free software. You can redistribute
 * this software and/or modify this software under the terms of the
 * GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Cleanflight and Betaflight are distributed in the hope that they
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.
 *
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

#include "platform.h"
#include "drivers/io.h"

#include "drivers/dma.h"
#include "drivers/timer.h"
#include "drivers/timer_def.h"

#include "fpga_drv.h"
#include "brainfpv/brainfpv_osd.h"
#include "brainfpv/brainfpv_system.h"

const timerHardware_t timerHardware[] = {
    DEF_TIM(TIM12, CH1, PB14,  TIM_USE_PPM,                         0,  0), // PPM input

    DEF_TIM(TIM2,  CH1, PA0,   TIM_USE_MC_MOTOR | TIM_USE_FW_MOTOR, 0,  0), // S1
	DEF_TIM(TIM3,  CH2, PB5,   TIM_USE_MC_MOTOR | TIM_USE_FW_MOTOR, 0,  1), // S2
	DEF_TIM(TIM4,  CH1, PD12,  TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO, 0,  2), // S3
    DEF_TIM(TIM4,  CH2, PD13,  TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO, 0,  3), // S4

    DEF_TIM(TIM8,  CH4, PC9,   TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO | TIM_USE_MC_SERVO, 0,  4), // S5
    DEF_TIM(TIM8,  CH3, PC8,   TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO | TIM_USE_MC_SERVO, 0,  5), // S6
    DEF_TIM(TIM1,  CH3, PE13,  TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO | TIM_USE_MC_SERVO, 0,  6), // S7
    DEF_TIM(TIM1,  CH2, PE11,  TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO | TIM_USE_MC_SERVO, 0,  7), // S8

    //DEF_TIM(TIM14, CH1, PA7,   TIM_USE_CAMERA_CONTROL,      0,  0),
};

const int timerHardwareCount = sizeof(timerHardware) / sizeof(timerHardware[0]);

bool brainfpv_settings_updated = true;
bool brainfpv_settings_updated_from_cms = false;

extern bfOsdConfig_t bfOsdConfigCms;
extern brainFpvSystemConfig_t brainFpvSystemConfigCms;

void brainFPVUpdateSettings(void) {
#if defined(USE_BRAINFPV_OSD)
    const bfOsdConfig_t * bfOsdConfigUse;
#endif
    const brainFpvSystemConfig_t * brainFpvSystemConfigUse;

    if (brainfpv_settings_updated_from_cms) {
#if defined(USE_BRAINFPV_OSD)
        bfOsdConfigUse = &bfOsdConfigCms;
#endif
        brainFpvSystemConfigUse = &brainFpvSystemConfigCms;
    }
    else {
#if defined(USE_BRAINFPV_OSD)
        bfOsdConfigUse = bfOsdConfig();
#endif
        brainFpvSystemConfigUse = brainFpvSystemConfig();
    }

#if defined(USE_BRAINFPV_OSD)
    brainFpvOsdSetSyncThreshold(bfOsdConfigUse->sync_threshold);

    BRAINFPVFPGA_SetXOffset(bfOsdConfigUse->x_offset);
    BRAINFPVFPGA_SetXScale(bfOsdConfigUse->x_scale);
#endif
    BRAINFPVFPGA_SetStatusLEDColor(brainFpvSystemConfigUse->status_led_color, brainFpvSystemConfigUse->status_led_brightness);

    brainfpv_settings_updated_from_cms = false;
}
