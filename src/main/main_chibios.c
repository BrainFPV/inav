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
#include <string.h>

#include "ch.h"
#include "hal_st.h"
//#include "nvic.h"

#include "platform.h"
#include "build/debug.h"
#include "drivers/serial.h"
#include "drivers/serial_softserial.h"
#include "drivers/persistent.h"

#include "fc/fc_init.h"

#include "scheduler/scheduler.h"
#include "brainfpv/brainfpv_system.h"

extern uint32_t __process_stack_end__;
extern uint32_t __process_stack_base__;


volatile bool idleCounterClear = 0;
volatile uint32_t idleCounter = 0;
binary_semaphore_t gyroSem;

void appIdleHook(void)
{
    // Called when the scheduler has no tasks to run
    if (idleCounterClear) {
        idleCounter = 0;
        idleCounterClear = 0;
    } else {
        ++idleCounter;
    }
}

#if defined(USE_BRAINFPV_OSD)
#include "brainfpv/brainfpv_osd.h"

binary_semaphore_t onScreenDisplaySemaphore;

static THD_WORKING_AREA(waOSDThread, 2 * 1024);
static THD_FUNCTION(OSDThread, arg)
{
    (void)arg;
    chRegSetThreadName("OSD");
    brainFpvOsdInit();
    brainFpvOsdMain();
}
#endif

//#define USE_DUMMY_TASK
#if defined(USE_DUMMY_TASK)
static THD_WORKING_AREA(waDummyThread, 512);
static THD_FUNCTION(DummyThread, arg)
{
    (void)arg;
    chRegSetThreadName("Dummy");
    while (1) {
        chThdSleepMilliseconds(1);
    }
}
#endif

static THD_WORKING_AREA(waInavThread, 8 * 1024);
static THD_FUNCTION(InavThread, arg)
{
    (void)arg;
    chRegSetThreadName("INAV");

    // init
    init();

    brainFPVSystemInit();
    chBSemObjectInit(&gyroSem, FALSE);

#if defined(USE_BRAINFPV_OSD)
    if (VideoIsInitialized()) {
        chBSemObjectInit(&onScreenDisplaySemaphore, FALSE);
        chThdCreateStatic(waOSDThread, sizeof(waOSDThread), NORMALPRIO, OSDThread, NULL);
    }
#endif /* USE_BRAINFPV_OSD */

    // run INAV scheduler forever
    while (true) {
        scheduler();
    }
}

#define CONTROL_MODE_PRIVILEGED             0
#define CONTROL_USE_PSP                     2
#define CONTROL_FPCA                        4
#define CRT0_CONTROL_INIT (CONTROL_USE_PSP | CONTROL_MODE_PRIVILEGED | CONTROL_FPCA)

int main(void)
{
    // Fill process stack
    uint32_t fill_size = (uint32_t)&__process_stack_end__ - (uint32_t)&__process_stack_base__ - 4;
    memset((void*)&__process_stack_base__, CH_DBG_STACK_FILL_VALUE, fill_size);

    // init ChibiOS
    __set_PSP((uint32_t)&__process_stack_end__);
    asm("movs    r0, %[ctl]\n\t" // Switch to thread mode with PSP
        "msr     CONTROL, r0\n\t"
        "isb\n\t":: [ctl] "i" (CRT0_CONTROL_INIT) : "r0");

    stInit();
    chSysInit();

    chThdCreateStatic(waInavThread, sizeof(waInavThread), HIGHPRIO, InavThread, NULL);

#if defined(USE_DUMMY_TASK)
    chThdCreateStatic(waDummyThread, sizeof(waDummyThread), NORMALPRIO, DummyThread, NULL);
#endif

    // sleep forever
    chThdSleep(TIME_INFINITE);
}
