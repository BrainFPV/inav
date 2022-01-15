/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_VIDEO Code for OSD video generator
 * @brief Output video (black & white pixels) over SPI
 * @{
 *
 * @file       pios_video.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2015
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
 */

#include <stdbool.h>
#include <stdint.h>

#include "ch.h"
#include "video.h"
#include "brainfpv_osd.h"
#include "auto_sync_threshold.h"

#include "platform.h"
//#include "system.h"
#include "drivers/io.h"
#include "drivers/io_impl.h"
#include "drivers/exti.h"
#include "drivers/nvic.h"
#include "drivers/light_led.h"
#include "drivers/time.h"

#include "common/log.h"

#if defined(USE_BRAINFPV_OSD) && defined(INCLUDE_VIDEO_QUADSPI)

#if defined(STM32F446xx)
#include <stm32f4xx_qspi.h>
#else
#if defined(STM32H750xx)
#include "stm32h7xx_hal_qspi.h"
#include "stm32h7xx_hal_mdma.h"

QSPI_HandleTypeDef hqspi;
MDMA_HandleTypeDef hmdma;
#else
#error "MCU not supported"
#endif
#endif

static IO_t hsync_io;
static IO_t vsync_io;

#if !defined(VIDEO_QUADSPI_Y_OFFSET)
#define VIDEO_QUADSPI_Y_OFFSET 0
#endif /* !defined(VIDEO_QUADSPI_Y_OFFSET) */

#if defined(VIDEO_QSPI_IO2_PIN) && defined(VIDEO_QSPI_IO3_PIN)
#define VIDEO_QSPI_USE_4_LINES
#endif

// How many frames until we redraw
#define VSYNC_REDRAW_CNT 2

// Minimum micro seconds between VSYNCS
#define MIN_DELTA_VSYNC 10000
#define MAX_SPURIOUS_VSYNCS 10

extiCallbackRec_t vsyncIntCallbackRec;
extiCallbackRec_t hsyncIntCallbackRec;

extern binary_semaphore_t onScreenDisplaySemaphore;

#define GRPAHICS_RIGHT_NTSC 351
#define GRPAHICS_RIGHT_PAL  359

// Line counts for format detection
#define VIDEO_TYPE_PAL_LINES 315
#define VIDEO_TYPE_NTSC_LINES 265
#define VIDEO_TYPE_DET_MAX_LINE_COUNT_ERR 10

static const struct video_type_boundary video_type_boundary_ntsc = {
    .graphics_right  = GRPAHICS_RIGHT_NTSC, // must be: graphics_width_real - 1
    .graphics_bottom = 239,                 // must be: graphics_hight_real - 1
};

static const struct video_type_boundary video_type_boundary_pal = {
    .graphics_right  = GRPAHICS_RIGHT_PAL, // must be: graphics_width_real - 1
    .graphics_bottom = 265,                // must be: graphics_hight_real - 1
};

#define NTSC_BYTES (GRPAHICS_RIGHT_NTSC / (8 / VIDEO_BITS_PER_PIXEL) + 1)
#define PAL_BYTES (GRPAHICS_RIGHT_PAL / (8 / VIDEO_BITS_PER_PIXEL) + 1)

static const struct video_type_cfg video_type_cfg_ntsc = {
    .graphics_hight_real   = 240,   // Real visible lines
    .graphics_column_start = 260,   // First visible OSD column (after Hsync)
    .graphics_line_start   = 22,    // First visible OSD line
    .dma_buffer_length     = NTSC_BYTES + NTSC_BYTES % 4, // DMA buffer length in bytes (has to be multiple of 4)
};

static const struct video_type_cfg video_type_cfg_pal = {
    .graphics_hight_real   = 266,   // Real visible lines
    .graphics_column_start = 420,   // First visible OSD column (after Hsync)
    .graphics_line_start   = 28,    // First visible OSD line
    .dma_buffer_length     = PAL_BYTES + PAL_BYTES % 4, // DMA buffer length in bytes (has to be multiple of 4)
};

// OSD buffer
uint8_t draw_buffer[BUFFER_HEIGHT * BUFFER_WIDTH] __attribute__ ((section(".video_ram"), aligned(4)));

const struct video_type_boundary *video_type_boundary_act = &video_type_boundary_pal;

// Private variables
static bool video_initialized = false;
static uint8_t spurious_vsync_cnt = 0;
static int16_t active_line = 0;
static uint32_t buffer_offset;
static int8_t y_offset = 0;
static uint16_t num_video_lines = 0;
static bool trigger_redraw;
static VideoType_t video_type_tmp = VIDEO_TYPE_PAL;
static VideoType_t video_type_act = VIDEO_TYPE_NONE;
static const struct video_type_cfg *video_type_cfg_act = &video_type_cfg_pal;

uint8_t black_pal = 30;
uint8_t white_pal = 110;
uint8_t black_ntsc = 10;
uint8_t white_ntsc = 110;

#if defined(USE_BRAINFPV_AUTO_SYNC_THRESHOLD)
bool useAutoSyncThreshold = false;
#endif

// Re-enable the video if it has been disabled
void video_qspi_enable(void)
{
    // re-enable interrupts
    spurious_vsync_cnt = 0;
    EXTIEnable(vsync_io, true);
    EXTIEnable(hsync_io, true);
}

/**
 * @brief Vsync interrupt service routine
 */
FAST_CODE void Vsync_ISR(extiCallbackRec_t *cb)
{
    (void)cb;
    static uint32_t t_last = 0;
    static uint16_t Vsync_update = 0;

    uint32_t t_now;

    t_now = microsISR();

    if (t_now - t_last < MIN_DELTA_VSYNC) {
        spurious_vsync_cnt += 1;
        if (spurious_vsync_cnt >= MAX_SPURIOUS_VSYNCS) {
            // spurious detections: disable interrupts
            EXTIEnable(vsync_io, false);
            EXTIEnable(hsync_io, false);
        }
        return;
    }
    else {
        spurious_vsync_cnt = 0;
    }
    t_last = t_now;

    // discard spurious vsync pulses (due to improper grounding), so we don't overload the CPU
    if (active_line > 0 && active_line < video_type_cfg_ntsc.graphics_hight_real - 10) {
        return;
    }

    // Update the number of video lines
    num_video_lines = active_line + video_type_cfg_act->graphics_line_start + y_offset;

    //LOG_E(OSD, "num_video_lines %d\r", num_video_lines);

    // detect video type
    if ((num_video_lines >= (VIDEO_TYPE_NTSC_LINES - VIDEO_TYPE_DET_MAX_LINE_COUNT_ERR)) &&
        (num_video_lines <= (VIDEO_TYPE_NTSC_LINES + VIDEO_TYPE_DET_MAX_LINE_COUNT_ERR))) {
        video_type_tmp = VIDEO_TYPE_NTSC;
    }
    else if ((num_video_lines >= (VIDEO_TYPE_PAL_LINES - VIDEO_TYPE_DET_MAX_LINE_COUNT_ERR)) &&
             (num_video_lines <= (VIDEO_TYPE_PAL_LINES + VIDEO_TYPE_DET_MAX_LINE_COUNT_ERR))) {
        video_type_tmp = VIDEO_TYPE_PAL;
    }
    else {
        video_type_tmp = VIDEO_TYPE_NONE;
    }

    // if video type has changed set new active values
    if (video_type_act != video_type_tmp) {
        video_type_act = video_type_tmp;
        if (video_type_act == VIDEO_TYPE_PAL) {
            video_type_boundary_act = &video_type_boundary_pal;
            video_type_cfg_act = &video_type_cfg_pal;
        } else {
            video_type_boundary_act = &video_type_boundary_ntsc;
            video_type_cfg_act = &video_type_cfg_ntsc;
        }
    }

    // Every VSYNC_REDRAW_CNT field: swap buffers and trigger redraw
    if (++Vsync_update >= VSYNC_REDRAW_CNT) {
        Vsync_update = 0;
        trigger_redraw = true;
    }
    else {
        trigger_redraw = false;
    }

    // Get ready for the first line. We will start outputting data at line zero.
    active_line = 0 - (video_type_cfg_act->graphics_line_start + y_offset);

    buffer_offset = 0;
}

FAST_CODE void Hsync_ISR(extiCallbackRec_t *cb)
{
    (void)cb;
    active_line++;

#if defined(STM32F446xx)
    EXTI->PR = 0x04;
#endif

    if ((active_line >= 0) && (active_line < video_type_cfg_act->graphics_hight_real)) {
        // Check if QUADSPI is busy
        if (QUADSPI->SR & 0x20)
            return;

#if defined(STM32F446xx)
        // Disable DMA
        DMA2_Stream7->CR &= ~(uint32_t)DMA_SxCR_EN;

        // Clear the DMA interrupt flags
        DMA2->HIFCR  |= DMA_FLAG_TCIF7 | DMA_FLAG_HTIF7 | DMA_FLAG_FEIF7 | DMA_FLAG_TEIF7 | DMA_FLAG_DMEIF7;

        // Load new line
        DMA2_Stream7->M0AR = (uint32_t)&draw_buffer[buffer_offset];

        // Set length
        DMA2_Stream7->NDTR = (uint16_t)video_type_cfg_act->dma_buffer_length;
        QUADSPI->DLR = (uint32_t)video_type_cfg_act->dma_buffer_length - 1;

        if (trigger_redraw && (active_line == video_type_cfg_act->graphics_hight_real - 1)) {
            // Last line: Enable DMA TC interrupt
            DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
        }

        // Enable DMA
        uint32_t cr = DMA2_Stream7->CR;
        DMA2_Stream7->CR = cr | (uint32_t)DMA_SxCR_EN;
#endif /* defined(STM32F446xx) */

#if defined(STM32H750xx)
        // Disable
        __HAL_MDMA_DISABLE(&hmdma);

        // Clear interrupt flags
        __HAL_MDMA_CLEAR_FLAG(&hmdma, MDMA_FLAG_TE | MDMA_FLAG_CTC | MDMA_FLAG_BFTC | MDMA_FLAG_BT | MDMA_FLAG_BRT);

        // Load new line
        hmdma.Instance->CSAR = (uint32_t)&draw_buffer[buffer_offset];

        // Set length
        hmdma.Instance->CBNDTR = (uint32_t)video_type_cfg_act->dma_buffer_length;
        QUADSPI->DLR = (uint32_t)video_type_cfg_act->dma_buffer_length - 1;

#if !defined(DEBUG_BUILD)
        // Write-back cache
        uint32_t start_addr = (uint32_t)&draw_buffer[buffer_offset];
        uint32_t end_addr = start_addr + (uint32_t)video_type_cfg_act->dma_buffer_length;
        start_addr &= ~0x1F; // 32-byte alignment
        SCB_CleanDCache_by_Addr((uint32_t *)start_addr, end_addr - start_addr + 1);
#endif

        if (trigger_redraw && (active_line == video_type_cfg_act->graphics_hight_real - 1)) {
            // Last line: Enable DMA TC interrupt
            __HAL_MDMA_ENABLE_IT(&hmdma, MDMA_IT_CTC);
        }

        // Enable DMA
        __HAL_MDMA_ENABLE(&hmdma);
#endif /* defined(STM32H750xx) */

        buffer_offset += BUFFER_WIDTH;
    }
}

// ISR runs after last OSD line has been output
#if defined(STM32F446xx)
FAST_CODE void DMA2_Stream7_IRQHandler(void)
{
    CH_IRQ_PROLOGUE();

    if (DMA2->HISR & DMA_FLAG_TCIF7) {
        // Clear flag and disable interrupt
        DMA2->HIFCR  |= DMA_FLAG_TCIF7;
		DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, DISABLE);

	    if (!onScreenDisplaySemaphore.sem.queue.next || !onScreenDisplaySemaphore.sem.queue.prev) {
	        // semaphore not initialized
	        return;
	    }

		// Trigger OSD redraw
        chSysLockFromISR();
        chBSemSignalI(&onScreenDisplaySemaphore);
        chSysUnlockFromISR();
	}

    CH_IRQ_EPILOGUE();
}
#endif /* defined(STM32F446xx) */

#if defined(STM32H750xx)
FAST_CODE void MDMA_IRQHandler(void)
{
    CH_IRQ_PROLOGUE();

    if (__HAL_MDMA_GET_FLAG(&hmdma, MDMA_FLAG_CTC)) {
        // Clear flag and disable interrupt
        __HAL_MDMA_CLEAR_FLAG(&hmdma, MDMA_FLAG_CTC);
        __HAL_MDMA_DISABLE_IT(&hmdma, MDMA_IT_CTC);

        if (!onScreenDisplaySemaphore.sem.queue.next || !onScreenDisplaySemaphore.sem.queue.prev) {
            // semaphore not initialized
            return;
        }

        // Trigger OSD redraw
        chSysLockFromISR();
        chBSemSignalI(&onScreenDisplaySemaphore);
        chSysUnlockFromISR();
    }

    CH_IRQ_EPILOGUE();
}
#endif

// Configure exti for falling edge
void EXTIConfigFalling(IO_t io, extiCallbackRec_t *cb, int irqPriority)
{
#if defined(STM32F446xx)
    EXTIConfig(io, cb, irqPriority, EXTI_Trigger_Falling);
#endif /* defined(STM32F446xx) */

#if defined(STM32H750xx)

    EXTIConfig(io, cb, irqPriority, IO_CONFIG(GPIO_MODE_INPUT, 0, GPIO_NOPULL));

    GPIO_InitTypeDef init = {
        .Pin = IO_Pin(io),
        .Mode = GPIO_MODE_IT_FALLING,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Pull = GPIO_NOPULL,
    };
    HAL_GPIO_Init(IO_GPIO(io), &init);
#endif /* defined(STM32H750xx) */
}

/**
 * Init
 */
void Video_Init(void)
{
    /* Map pins to QUADSPI */
    IOInit(IOGetByTag(IO_TAG(VIDEO_QSPI_CLOCK_PIN)),  OWNER_OSD, RESOURCE_SPI_SCK, 0);
    IOInit(IOGetByTag(IO_TAG(VIDEO_QSPI_IO0_PIN)), OWNER_OSD, RESOURCE_SPI_MOSI, 0);
    IOInit(IOGetByTag(IO_TAG(VIDEO_QSPI_IO1_PIN)), OWNER_OSD, RESOURCE_SPI_MOSI, 0);

    IOConfigGPIOAF(IOGetByTag(IO_TAG(VIDEO_QSPI_CLOCK_PIN)), IOCFG_AF_PP, GPIO_AF9_QUADSPI);
    IOConfigGPIOAF(IOGetByTag(IO_TAG(VIDEO_QSPI_IO0_PIN)), IOCFG_AF_PP, GPIO_AF9_QUADSPI);
    IOConfigGPIOAF(IOGetByTag(IO_TAG(VIDEO_QSPI_IO1_PIN)), IOCFG_AF_PP, GPIO_AF9_QUADSPI);

#if defined(VIDEO_QSPI_USE_4_LINES)
    IOInit(IOGetByTag(IO_TAG(VIDEO_QSPI_IO2_PIN)), OWNER_OSD, RESOURCE_SPI_MOSI, 0);
    IOInit(IOGetByTag(IO_TAG(VIDEO_QSPI_IO3_PIN)), OWNER_OSD, RESOURCE_SPI_MOSI, 0);
    IOConfigGPIOAF(IOGetByTag(IO_TAG(VIDEO_QSPI_IO2_PIN)), IOCFG_AF_PP, GPIO_AF9_QUADSPI);
    IOConfigGPIOAF(IOGetByTag(IO_TAG(VIDEO_QSPI_IO3_PIN)), IOCFG_AF_PP, GPIO_AF9_QUADSPI);
#endif

#if defined(STM32F446xx)
    /* Enable QUADSPI clock */
    RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_QSPI, ENABLE);

    /* Configure QUADSPI */
    QSPI_InitTypeDef qspi_init = {
        .QSPI_SShift     = QSPI_SShift_NoShift,
        .QSPI_Prescaler  = 12,  // 180MHz / 12 = 15MHz
        .QSPI_CKMode     = QSPI_CKMode_Mode0,
        .QSPI_CSHTime    = QSPI_CSHTime_1Cycle,
        .QSPI_FSize      = 0x1F,
        .QSPI_FSelect    = QSPI_FSelect_1,
        .QSPI_DFlash     = QSPI_DFlash_Disable};

    QSPI_Init(&qspi_init);

    QSPI_ComConfig_InitTypeDef qspi_com_config;
    QSPI_ComConfig_StructInit(&qspi_com_config);

    qspi_com_config.QSPI_ComConfig_FMode       = QSPI_ComConfig_FMode_Indirect_Write;
    qspi_com_config.QSPI_ComConfig_DDRMode     = QSPI_ComConfig_DDRMode_Disable;
    qspi_com_config.QSPI_ComConfig_DHHC        = QSPI_ComConfig_DHHC_Disable;
    qspi_com_config.QSPI_ComConfig_SIOOMode    = QSPI_ComConfig_SIOOMode_Disable;
#if defined(VIDEO_QSPI_USE_4_LINES)
    qspi_com_config.QSPI_ComConfig_DMode       = QSPI_ComConfig_DMode_4Line;
#else
    qspi_com_config.QSPI_ComConfig_DMode       = QSPI_ComConfig_DMode_2Line;
#endif
    qspi_com_config.QSPI_ComConfig_DummyCycles = 0;
    qspi_com_config.QSPI_ComConfig_ABMode      = QSPI_ComConfig_ABMode_NoAlternateByte;
    qspi_com_config.QSPI_ComConfig_ADMode      = QSPI_ComConfig_ADMode_NoAddress;
    qspi_com_config.QSPI_ComConfig_IMode       = QSPI_ComConfig_IMode_NoInstruction;
    QSPI_ComConfig_Init(&qspi_com_config);

    QSPI_SetFIFOThreshold(3);

    /* Configure DMA */
    DMA_InitTypeDef dma_cfg = {
        .DMA_Channel            = DMA_Channel_3,
        .DMA_PeripheralBaseAddr = (uint32_t)&(QUADSPI->DR),
        .DMA_DIR                = DMA_DIR_MemoryToPeripheral,
        .DMA_BufferSize         = 400,
        .DMA_PeripheralInc      = DMA_PeripheralInc_Disable,
        .DMA_MemoryInc          = DMA_MemoryInc_Enable,
        .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte,
        .DMA_MemoryDataSize     = DMA_MemoryDataSize_Word,
        .DMA_Mode               = DMA_Mode_Normal,
        .DMA_Priority           = DMA_Priority_VeryHigh,
        .DMA_FIFOMode           = DMA_FIFOMode_Enable,
        .DMA_FIFOThreshold      = DMA_FIFOThreshold_Full,
        .DMA_MemoryBurst        = DMA_MemoryBurst_INC4,
        .DMA_PeripheralBurst    = DMA_PeripheralBurst_Single};

    DMA_Init(DMA2_Stream7, &dma_cfg);


    /* Enable TC interrupt */
    QSPI_ITConfig(QSPI_IT_TC, ENABLE);
    QSPI_ITConfig(QSPI_IT_FT, ENABLE);

    /* Enable DMA */
    QSPI_DMACmd(ENABLE);

    // Enable the QUADSPI
    QSPI_Cmd(ENABLE);
#endif /* defined(STM32F446xx) */

#if defined(STM32H750xx)
    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();

    hqspi.Instance = QUADSPI;

    hqspi.Init.ClockPrescaler = 15; // 240MHz / 16 = 15MHz
    hqspi.Init.FifoThreshold = 16;
    hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;
    hqspi.Init.FlashSize = 0x1F;
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
    hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;

    if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
        return;
    }

    // MDMA
    __HAL_RCC_MDMA_CLK_ENABLE();

    hmdma.Instance = MDMA_Channel0;
    hmdma.Init.Request = MDMA_REQUEST_QUADSPI_FIFO_TH;
    hmdma.Init.TransferTriggerMode = MDMA_BUFFER_TRANSFER;
    hmdma.Init.Priority = MDMA_PRIORITY_VERY_HIGH;
    hmdma.Init.Endianness = MDMA_LITTLE_ENDIANNESS_PRESERVE;
    hmdma.Init.SourceInc = MDMA_SRC_INC_WORD;
    hmdma.Init.DestinationInc = MDMA_DEST_INC_DISABLE;
    hmdma.Init.SourceDataSize = MDMA_SRC_DATASIZE_WORD;
    hmdma.Init.DestDataSize = MDMA_DEST_DATASIZE_WORD;
    hmdma.Init.DataAlignment = MDMA_DATAALIGN_PACKENABLE;
    hmdma.Init.BufferTransferLength = 16;
    hmdma.Init.SourceBurst = MDMA_SOURCE_BURST_SINGLE;
    hmdma.Init.DestBurst = MDMA_SOURCE_BURST_SINGLE;
    hmdma.Init.SourceBlockAddressOffset = 0;
    hmdma.Init.DestBlockAddressOffset = 0;

    if (HAL_MDMA_Init(&hmdma) != HAL_OK) {
        return;
    }

    __HAL_LINKDMA(&hqspi, hmdma, hmdma);

    QSPI_CommandTypeDef cmd;
    cmd.InstructionMode   = QSPI_INSTRUCTION_NONE;
    cmd.AddressMode       = QSPI_ADDRESS_NONE;
    cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
#if defined(VIDEO_QSPI_USE_4_LINES)
    cmd.DataMode          = QSPI_DATA_4_LINES;
#else
    cmd.DataMode          = QSPI_DATA_2_LINES;
#endif
    cmd.DummyCycles       = 0;
    cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
    cmd.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
    cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;
    cmd.NbData            = 132;

    HAL_QSPI_Command(&hqspi, &cmd, 100);

    // Set DMA dest address
    hmdma.Instance->CDAR = (uint32_t)&(QUADSPI->DR);
#endif /* defined(STM32H750xx) */

    // VSYNC interrupt
    vsync_io = IOGetByTag(IO_TAG(VIDEO_VSYNC));
    IOInit(vsync_io, OWNER_OSD, RESOURCE_EXTI, 0);
    IOConfigGPIO(vsync_io, IOCFG_IN_FLOATING);
    EXTIHandlerInit(&vsyncIntCallbackRec, Vsync_ISR);
    EXTIConfigFalling(vsync_io, &vsyncIntCallbackRec, STM32_ST_IRQ_PRIORITY);

    // HSYNC interrupt
    hsync_io = IOGetByTag(IO_TAG(VIDEO_HSYNC));
    IOInit(hsync_io, OWNER_OSD, RESOURCE_EXTI, 0);
    IOConfigGPIO(hsync_io, IOCFG_IN_FLOATING);
    EXTIHandlerInit(&hsyncIntCallbackRec, Hsync_ISR);
    EXTIConfigFalling(hsync_io, &hsyncIntCallbackRec, STM32_ST_IRQ_PRIORITY);

    // DMA TC interrupt for last line
#if defined(STM32F446xx)
    NVIC_SetPriority(DMA2_Stream7_IRQn, STM32_ST_IRQ_PRIORITY);
    NVIC_EnableIRQ(DMA2_Stream7_IRQn);
    DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, DISABLE); // will be enabled later
#endif /* defined(STM32F446xx) */

#if defined(STM32H750xx)
    NVIC_SetPriority(MDMA_IRQn, STM32_ST_IRQ_PRIORITY);
    NVIC_EnableIRQ(MDMA_IRQn);
    __HAL_MDMA_DISABLE_IT(&hmdma, MDMA_IT_BFTC); // will be enabled later
#endif /* defined(STM32H750xx) */

    // Enable interrupts
    EXTIEnable(vsync_io, true);
    EXTIEnable(hsync_io, true);

#if defined(USE_BRAINFPV_AUTO_SYNC_THRESHOLD)
   if (bfOsdConfig()->sync_threshold_mode == SYNC_THRESHOLD_AUTO) {
       if (autoSyncThresholdInit() == 0) {
           useAutoSyncThreshold = true;
       }
   }
#endif

    video_initialized = true;
}

bool VideoIsInitialized(void)
{
    return video_initialized;
}
/**
 *
 */
uint16_t Video_GetLines(void)
{
    return num_video_lines;
}

/**
 *
 */
VideoType_t Video_GetType(void)
{
    return video_type_act;
}

#endif /* INCLUDE_VIDEO_QUADSPI */
