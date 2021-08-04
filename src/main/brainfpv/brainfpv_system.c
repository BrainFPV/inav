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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "platform.h"
#include "brainfpv/brainfpv_system.h"

#include "config/parameter_group_ids.h"

#include "flight/mixer.h"
#include "drivers/system.h"
#include "drivers/time.h"
#include "drivers/light_led.h"
#include "drivers/io.h"
#include "io/beeper.h"
#include "fc/config.h"
#include "fc/fc_core.h"

#if defined(BRAINFPV)

#if defined(USE_BRAINFPV_BOOTLOADER)
typedef struct __attribute__((packed)) {
    uint32_t target_magic;
    uint32_t isr_vector_base;
} BrainFPVBlHeader_t;

const BrainFPVBlHeader_t __attribute__((section (".bl_header_section"))) __attribute__((used)) BRAINFPV_BL_HEADER = {
    .target_magic = BOOTLOADER_TARGET_MAGIC,
    .isr_vector_base = VECT_TAB_BASE,
};
#endif /* defined(USE_BRAINFPV_BOOTLOADER) */

#if defined(USE_CUSTOM_RESET)
void CustomSystemReset(void)
{
    IO_t reset_pin = IOGetByTag(IO_TAG(CUSTOM_RESET_PIN));
    IOInit(reset_pin, OWNER_PULLDOWN, 0);
    IOConfigGPIO(reset_pin, IOCFG_OUT_OD);

    __DSB();                                                          /* Ensure all outstanding memory accesses included
                                                                         buffered write are completed before reset */

    IOLo(reset_pin);
    for(;;)                                                           /* wait until reset */
    {
      __NOP();
    }
}
#endif /* defined(USE_CUSTOM_RESET) */

#if defined(USE_DEBUG_PIN)
IO_t debug_pin;

void debug_pin_hi(void)
{
    IOHi(debug_pin);
}

void debug_pin_lo(void)
{
    IOLo(debug_pin);
}
#endif

static BrainFPVSystemReq_t brainfpv_req = BRAINFPV_REQ_NONE;

PG_RESET_TEMPLATE(brainFpvSystemConfig_t, brainFpvSystemConfig,
  .status_led_color = COLOR_BLUE,
  .status_led_brightness = 255,
);

PG_REGISTER_WITH_RESET_TEMPLATE(brainFpvSystemConfig_t, brainFpvSystemConfig, PG_BRAINFPV_SYSTEM_CONFIG, 0);


#if defined(USE_VTXFAULT_PIN)
IO_t vtx_fault_pin;

static void vtxFaultInit(void)
{
    vtx_fault_pin = IOGetByTag(IO_TAG(VTXFAULT_PIN));

    IOInit(vtx_fault_pin,  OWNER_OSD, 0);
    IOConfigGPIO(vtx_fault_pin, IO_CONFIG(GPIO_MODE_INPUT, 0, GPIO_PULLUP));
}

static void vtxFaultCheck(void)
{
    static bool fault_detected = false;

    if (IORead(vtx_fault_pin) == false) {
        // over current condition detected
        LED1_ON;
        fault_detected = true;
    }
    else {
        if (fault_detected) {
            // over current condition has been cleared
            LED1_OFF;
            fault_detected = false;
        }
    }
}
#endif /* defined(USE_VTXFAULT_PIN) */

void brainFPVSystemInit(void)
{
#if defined(USE_VTXFAULT_PIN)
    vtxFaultInit();
#endif /* defined(USE_VTXFAULT_PIN) */

#if defined(USE_DEBUG_PIN)
    debug_pin = IOGetByTag(IO_TAG(DEBUG_PIN));
    IOConfigGPIO(debug_pin, IOCFG_OUT_PP);
#endif
}

// Set the request
void brainFPVSystemSetReq(BrainFPVSystemReq_t req)
{
    brainfpv_req = req;
}


void saveConfigAndNotifyBrainFPV(void)
{
    writeEEPROM();
    readEEPROM();
    beeperConfirmationBeeps(1);
}

// Execute request (called from betaflight system task)
void brainFPVSystemCheck(void)
{
    switch(brainfpv_req) {
        case BRAINFPV_REQ_NONE:
            // Nothing to do
            break;
        case BRAINFPV_REQ_UPDATE_HW_SETTINGS:
            brainFPVUpdateSettings();
            break;
        case BRAINFPV_REQ_SAVE_SETTINGS:
            saveConfigAndNotifyBrainFPV();
            break;
        case BRAINFPV_REQ_SAVE_SETTINGS_REBOOT:
            saveConfigAndNotifyBrainFPV();
            delay(200);

            fcReboot(false);
            break;
    }
    brainfpv_req = BRAINFPV_REQ_NONE;

#if defined(USE_VTXFAULT_PIN)
        vtxFaultCheck();
#endif /* defined(USE_VTXFAULT_PIN) */
}

#endif
