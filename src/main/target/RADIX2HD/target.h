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

#pragma once

#include <stdbool.h>

//#define DEBUG_PRINTF

#define TARGET_BOARD_IDENTIFIER "RDX2HD"
#define USBD_PRODUCT_STRING "BrainFPV RADIX 2 HD"

#define EEPROM_SIZE (4 * 4096)

#define BOOTLOADER_TARGET_MAGIC 0x785E9A14

// For ChibiOS
// Priority needs to be lower than any of the INAV interrupts that don't use CH_IRQ_EPILOGUE
#define STM32_ST_IRQ_PRIORITY               7
#define STM32_ST_USE_TIMER                  13

//#define USE_CUSTOM_RESET
#define CUSTOM_RESET_PIN PC13

#define VECT_TAB_BASE 0x24000000

#define USE_MULT_CPU_IDLE_COUNTS
#define IDLE_COUNTS_PER_SEC_AT_NO_LOAD_400 (18506775)
#define IDLE_COUNTS_PER_SEC_AT_NO_LOAD_480 (22208130)

#define USE_TARGET_CONFIG


// TODO: Add PWM RGB LED support
//#define USE_BRAINFPV_RGB_STATUS_LED
#define LED0 PA7
#define LED1 PE5

#define BEEPER                  PE4
#define BEEPER_INVERTED

#define USE_LED_STRIP
#define WS2811_PIN              PA3

#define USE_PINIO
#define USE_PINIOBOX
#define PINIO1_PIN              PC14 // VREG HD
#define PINIO1_FLAGS            PINIO_FLAGS_INVERTED

#define USE_UART

#define USE_UART1
#define UART1_RX_PIN            PB15
#define UART1_TX_PIN            PB14

#define USE_UART2
#define UART2_RX_PIN            PD6
#define UART2_TX_PIN            PD5

#define USE_UART3
#define UART3_RX_PIN            PB11
#define UART3_TX_PIN            PD8

#define USE_UART4
#define UART4_RX_PIN            PB8
#define UART4_TX_PIN            PA0

#define USE_UART5
#define UART5_RX_PIN            PB12
#define UART5_TX_PIN            PB13

#define USE_UART6
#define UART6_RX_PIN            PC7
#define UART6_TX_PIN            PC6

#define USE_UART7
#define UART7_RX_PIN            PA8
//#define UART7_TX_PIN            NONE

#define USE_VCP
#define VBUS_SENSING_PIN        PA9
#define VBUS_SENSING_ENABLED
#define USE_USB48MHZ_PLL

#define SERIAL_PORT_COUNT       8

#define USE_SPI
#define USE_SPI_DEVICE_1
#define SPI1_SCK_PIN            PA5
#define SPI1_MISO_PIN           PB4
#define SPI1_MOSI_PIN           PD7

#define USE_QUADSPI
#define USE_QUADSPI_DEVICE_1
#define QUADSPI1_SCK_PIN PB2
#define QUADSPI1_BK1_IO0_PIN PD11
#define QUADSPI1_BK1_IO1_PIN PD12
#define QUADSPI1_BK1_IO2_PIN PE2
#define QUADSPI1_BK1_IO3_PIN PA1
#define QUADSPI1_BK1_CS_PIN PB10

#define QUADSPI1_BK2_IO0_PIN NONE
#define QUADSPI1_BK2_IO1_PIN NONE
#define QUADSPI1_BK2_IO2_PIN NONE
#define QUADSPI1_BK2_IO3_PIN NONE
#define QUADSPI1_BK2_CS_PIN NONE

#define QUADSPI1_MODE QUADSPI_MODE_BK1_ONLY
#define QUADSPI1_CS_FLAGS (QUADSPI_BK1_CS_HARDWARE | QUADSPI_BK2_CS_NONE | QUADSPI_CS_MODE_LINKED)

#define USE_SDCARD
#define USE_SDCARD_SDIO
#define SDCARD_SDIO_DEVICE      SDIODEV_1
#define SDCARD_SDIO_4BIT
#define SDCARD_DETECT_INVERTED
#define SDCARD_DETECT_PIN       PD9

#define ENABLE_BLACKBOX_LOGGING_ON_SDCARD_BY_DEFAULT

#define USE_I2C
#define USE_I2C_DEVICE_1
#define I2C1_SCL                PB6
#define I2C1_SDA                PB7

#define USE_MAG
#define MAG_I2C_BUS             BUS_I2C1
#define USE_MAG_HMC5883
#define USE_MAG_QMC5883
#define USE_MAG_IST8310
#define USE_MAG_IST8308
#define USE_MAG_MAG3110
#define USE_MAG_LIS3MDL

#define USE_BARO
#define BARO_I2C_BUS BUS_I2C1
#define USE_BARO_DPS310

#define USE_FLASHFS
#define USE_FLASH_M25P16
#define M25P16_FIRST_SECTOR       32
#define M25P16_SECTORS_SPARE_END   3
#define M25P16_QUADSPI_DEVICE QUADSPIDEV_1

#define CONFIG_IN_EXTERNAL_FLASH
#undef USE_GYRO_REGISTER_DUMP

#define USE_IMU_BMI270

#define IMU_BMI270_ALIGN     CW0_DEG
#define BMI270_SPI_BUS       BUS_SPI1
#define BMI270_CS_PIN        PD3
#define GYRO_INT_EXTI        PB3

#define USE_ADC
#define ADC_INSTANCE ADC1
#define ADCVREF 3285

#define ADC_CHANNEL_1_PIN   PC0
#define ADC_CHANNEL_2_PIN   PA6
#define ADC_CHANNEL_3_PIN   PC1

#define VBAT_ADC_CHANNEL           ADC_CHN_1
#define CURRENT_METER_ADC_CHANNEL  ADC_CHN_2
#define RSSI_ADC_CHANNEL           ADC_CHN_3

#define BOARD_HAS_VOLTAGE_DIVIDER
#define VBAT_SCALE_DEFAULT    1760
#define CURRENT_METER_SCALE   200

#define DEFAULT_FEATURES        (FEATURE_OSD | FEATURE_TX_PROF_SEL | FEATURE_TELEMETRY | FEATURE_CURRENT_METER | FEATURE_VBAT | FEATURE_BLACKBOX)

#define DEFAULT_RX_TYPE         RX_TYPE_SERIAL
#define SERIALRX_UART           SERIAL_PORT_USART3
#define SERIALRX_PROVIDER       SERIALRX_CRSF

#define MAX_PWM_OUTPUT_PORTS    10
#define USE_SERIAL_4WAY_BLHELI_INTERFACE

#define TARGET_IO_PORTA 0xffff
#define TARGET_IO_PORTB 0xffff
#define TARGET_IO_PORTC 0xffff
#define TARGET_IO_PORTD 0xffff
#define TARGET_IO_PORTE 0xffff
#define TARGET_IO_PORTF 0xffff
#define TARGET_IO_PORTG 0xffff

#define USE_DSHOT
#define USE_ESC_SENSOR
#define USE_SERIALSHOT

#if defined(DEBUG_BUILD)
// Disable feature to free up space
#undef USE_DSHOT
#undef USE_ESC_SENSOR
#undef USE_SERIALSHOT
#undef USE_RANGEFINDER
#undef USE_RANGEFINDER_MSP
#undef USE_RANGEFINDER_BENEWAKE
#undef USE_RANGEFINDER_VL53L0X
#undef USE_RANGEFINDER_HCSR04_I2C

#undef USE_OPFLOW
#undef USE_OPFLOW_CXOF
#undef USE_OPFLOW_MSP

#undef USE_PITOT_MS4525
#undef USE_1WIRE
#undef USE_1WIRE_DS2482
#undef USE_TEMPERATURE_SENSOR
#undef USE_TEMPERATURE_LM75
#undef USE_TEMPERATURE_DS18B20
#undef USE_DASHBOARD
#undef DASHBOARD_ARMED_BITMAP
#undef USE_OLED_UG2864
//#undef USE_PWM_DRIVER_PCA9685
#undef USE_FRSKYOSD
#undef USE_USB_MSC
#undef USE_SERVO_SBUS

#undef USE_TELEMETRY_LTM
#undef USE_TELEMETRY_HOTT
#undef USE_TELEMETRY_MAVLINK

#undef USE_SERIALRX_SUMD
#undef USE_SERIALRX_SUMH
#undef USE_SERIALRX_XBUS
#undef USE_SERIALRX_SBUS
//#undef USE_SERIALRX_JETIEXBUS
#undef USE_GPS_PROTO_MSP
#undef USE_TELEMETRY_FRSKY
#undef USE_GPS_PROTO_NAZA
#endif

extern bool brainfpv_settings_updated;
extern bool brainfpv_settings_updated_from_cms;

void CustomSystemReset(void);
void brainFPVUpdateSettings(void);
