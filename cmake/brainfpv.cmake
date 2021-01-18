include(main)
include(stm32f4)


set(CHIBIOS_DIR "${MAIN_LIB_DIR}/main/ChibiOS")

set(CHIBIOS_STARTUP_DIR "${CHIBIOS_DIR}/os/common/startup/ARMCMx/compilers/GCC")
set(CHIBIOS_HAL_DIR "${CHIBIOS_DIR}/os/hal")
set(CHIBIOS_F4_STARTUP_DIR "${CHIBIOS_DIR}/os/common/startup/ARMCMx/devices/STM32F4xx")
set(CHIBIOS_F4_HAL_DIR "${CHIBIOS_DIR}/os/hal/ports/STM32/STM32F4xx")


set(CHIBIOS_HAL_LLD_DIR "${CHIBIOS_DIR}/os/hal/ports/STM32/LLD")
set(CHIBIOS_HAL_PORT_DIR "${CHIBIOS_DIR}/os/hal/ports/common/ARMCMx")
set(CHIBIOS_HAL_INC_DIR "${CHIBIOS_DIR}/os/hal/include")
set(CHIBIOS_OSAL_DIR "${CHIBIOS_DIR}/os/hal/osal/rt")
set(CHIBIOS_RT_DIR "${CHIBIOS_DIR}/os/rt")
set(CHIBIOS_PORT_DIR "${CHIBIOS_DIR}/os/common/ports/ARMCMx")


file(GLOB_RECURSE CHIBIOS_F4_HAL_SRC ${CHIBIOS_F4_HAL_DIR}/*.c)
file(GLOB_RECURSE CHIBIOS_HAL_LLD_TIM_SRC ${CHIBIOS_HAL_LLD_DIR}/TIMv1/*.c)
file(GLOB_RECURSE CHIBIOS_HAL_PORT_SRC ${CHIBIOS_HAL_PORT_DIR}/*.c)


file(GLOB_RECURSE CHIBIOS_OSAL_SRC ${CHIBIOS_OSAL_DIR}/*.c)
file(GLOB_RECURSE CHIBIOS_RT_SRC ${CHIBIOS_RT_DIR}/src/*.c)

set(CHIBIOS_PORT_SRC
    "${CHIBIOS_PORT_DIR}/chcore.c"
    "${CHIBIOS_PORT_DIR}/chcore_v7m.c"
    "${CHIBIOS_PORT_DIR}/compilers/GCC/chcoreasm_v7m.S"
)

set(CHIBIOS_INCLUDE_DIRS
    "${CHIBIOS_F4_STARTUP_DIR}"
    "${CHIBIOS_F4_HAL_DIR}"
    "${CHIBIOS_HAL_LLD_DIR}/TIMv1"
    "${CHIBIOS_HAL_LLD_DIR}/EXTIv1"
    "${CHIBIOS_HAL_LLD_DIR}/DMAv2"
    "${CHIBIOS_HAL_LLD_DIR}/RTCv2"
    "${CHIBIOS_HAL_PORT_DIR}"
    "${CHIBIOS_HAL_INC_DIR}"
    "${CHIBIOS_OSAL_DIR}"
    "${CHIBIOS_RT_DIR}/include"
    "${CHIBIOS_PORT_DIR}"
    "${CHIBIOS_PORT_DIR}/compilers/GCC"
    "${CHIBIOS_DIR}/os/license"
    "${CHIBIOS_DIR}/os/oslib/include"
)

set(CHIBIOS_SRC
    "${CHIBIOS_STARTUP_DIR}/crt0_v7m.S"
    "${CHIBIOS_STARTUP_DIR}/crt1.c"
    "${CHIBIOS_DIR}/os/hal/src/hal_st.c"
    "${CHIBIOS_HAL_DIR}/src/hal.c"
    "${CHIBIOS_F4_HAL_SRC}"
    "${CHIBIOS_HAL_LLD_TIM_SRC}"
    "${CHIBIOS_HAL_LLD_DIR}/EXTIv1/stm32_exti.c"
    "${CHIBIOS_HAL_LLD_DIR}/DMAv2/stm32_dma.c"
    "${CHIBIOS_HAL_LLD_DIR}/RTCv2/hal_rtc_lld.c"
    "${CHIBIOS_HAL_PORT_SRC}"
    "${CHIBIOS_OSAL_SRC}"
    "${CHIBIOS_RT_SRC}"
    "${CHIBIOS_PORT_SRC}"
)

set(BRAINFPV_DIR "${MAIN_SRC_DIR}/brainfpv")
file(GLOB_RECURSE BRAINFPV_INAV_SRC ${BRAINFPV_DIR}/*.c)


set(BRAINFPV_INCLUDE_DIRS
    "${CHIBIOS_INCLUDE_DIRS}"
    "${BRAINFPV_DIR}"
)

set(BRAINFPV_SRC
  "${MAIN_SRC_DIR}/main_chibios.c"
  "${MAIN_SRC_DIR}/cms/cms_menu_brainfpv.c"
  "${STM32F411_OR_F427_STDPERIPH_SRC}"
  "${STM32F4_STDPERIPH_SRC_DIR}/stm32f4xx_qspi.c"
  "${STM32F4_STDPERIPH_SRC_DIR}/stm32f4xx_rcc.c"
  "${CHIBIOS_SRC}"
  "${BRAINFPV_INAV_SRC}"
)


set(STM32F446_BRAINFPV_COMPILE_DEFINITIONS
    STM32F446xx
    FLASH_SIZE=512
    USE_CHIBIOS
    CORTEX_USE_FPU=TRUE
    CORTEX_SIMPLIFIED_PRIORITY=TRUE
)

function(target_brainfpv_stm32f446 name)
    target_stm32f4xx(
        NAME ${name}
        HSE_MHZ 16
        DISABLE_MSC
        STARTUP startup_chibios_stm32f446xx.s
        SOURCES ${BRAINFPV_SRC}
        INCLUDE_DIRECTORIES ${BRAINFPV_INCLUDE_DIRS}
        COMPILE_DEFINITIONS ${STM32F446_BRAINFPV_COMPILE_DEFINITIONS}
        LINKER_SCRIPT stm32_flash_f446_chibios
        SVD STM32F446
        ${ARGN}
    )
endfunction()

SET(ASM_OPTIONS "-x assembler-with-cpp")


set(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} ${ASM_OPTIONS}")
set(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} ${ASM_OPTIONS}")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "${CMAKE_ASM_FLAGS_RELWITHDEBINFO} ${ASM_OPTIONS}")
