/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdint.h>

#include "platform.h"

#include "drivers/time.h"
#include "drivers/io.h"

#include "drivers/timer.h"
#include "drivers/pwm_mapping.h"
#include "drivers/pwm_output.h"
#include "fc/config.h"

#include "sound_beeper.h"

#if defined(USE_BRAINFPV_FPGA_BUZZER)
#include "fpga_drv.h"
#endif

#ifdef BEEPER

#if !defined(USE_BRAINFPV_FPGA_BUZZER)
static IO_t beeperIO = DEFIO_IO(NONE);
static bool beeperInverted = false;
#endif

static bool beeperState = false;

#endif

void systemBeep(bool onoff)
{
#if !defined(USE_BRAINFPV_FPGA_BUZZER)
#if !defined(BEEPER)
    UNUSED(onoff);
#else

    if (beeperConfig()->pwmMode) {
        pwmWriteBeeper(onoff);
        beeperState = onoff;
    } else {
        IOWrite(beeperIO, beeperInverted ? onoff : !onoff);
        beeperState = onoff;
    }

#endif
#else
    BRAINFPVFPGA_Buzzer(onoff);
    beeperState = onoff;
#endif

}

void systemBeepToggle(void)
{
#if defined(BEEPER)
    systemBeep(!beeperState);
#endif
}

void beeperInit(const beeperDevConfig_t *config)
{
#if !defined(BEEPER) || defined(USE_BRAINFPV_FPGA_BUZZER)
    UNUSED(config);
#else
    beeperIO = IOGetByTag(config->ioTag);
    beeperInverted = config->isInverted;

    if (beeperIO) {
        IOInit(beeperIO, OWNER_BEEPER, RESOURCE_OUTPUT, 0);
        if (beeperConfig()->pwmMode) {
            beeperPwmInit(config->ioTag, BEEPER_PWM_FREQUENCY);
        } else {
            IOConfigGPIO(beeperIO, config->isOD ? IOCFG_OUT_OD : IOCFG_OUT_PP);
        }
    }

    systemBeep(false);
#endif
}
