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

#include "brainfpv/brainfpv_system.h"

timerHardware_t timerHardware[] = {
    DEF_TIM(TIM1,  CH2, PE11,  TIM_USE_MC_MOTOR | TIM_USE_FW_MOTOR, 0,  0), // S1
    DEF_TIM(TIM1,  CH3, PE13,  TIM_USE_MC_MOTOR | TIM_USE_FW_MOTOR, 0,  1), // S2
    DEF_TIM(TIM2,  CH1, PA15,  TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO, 0,  2), // S3
    DEF_TIM(TIM2,  CH3, PA2,   TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO, 0,  3), // S4

    DEF_TIM(TIM3,  CH2, PB5,   TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO, 0,  4), // S5
    DEF_TIM(TIM3,  CH3, PB0,   TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO, 0,  5), // S6
    DEF_TIM(TIM4,  CH2, PD13,  TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO | TIM_USE_MC_SERVO, 0,  6), // S7
    DEF_TIM(TIM4,  CH3, PD14,  TIM_USE_MC_MOTOR | TIM_USE_FW_SERVO | TIM_USE_MC_SERVO, 0,  7), // S8

    // Additional timer outputs using the UART6 pins
    DEF_TIM(TIM8,  CH1, PC6,   TIM_USE_FW_SERVO | TIM_USE_MC_SERVO, 0,  0), // TX6 / S9
    DEF_TIM(TIM8,  CH2, PC7,   TIM_USE_FW_SERVO | TIM_USE_MC_SERVO, 0,  0), // RX6 / S10

    // RGB LED output (has inverter)
    DEF_TIM(TIM5,  CH4, PA3,   TIM_USE_LED, TIMER_OUTPUT_INVERTED,  8),
};

const int timerHardwareCount = sizeof(timerHardware) / sizeof(timerHardware[0]);

bool brainfpv_settings_updated = true;
bool brainfpv_settings_updated_from_cms = false;

void brainFPVUpdateSettings(void)
{
    brainfpv_settings_updated_from_cms = false;
}
