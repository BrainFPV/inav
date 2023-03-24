include(main)
include(stm32f4)
include(stm32h7)
include(GetGitRevisionDescription)

set(CHIBIOS_DIR "${MAIN_LIB_DIR}/main/ChibiOS")

#set(CHIBIOS_STARTUP_DIR "${CHIBIOS_DIR}/os/common/startup/ARMCMx/compilers/GCC")
#set(CHIBIOS_HAL_DIR "${CHIBIOS_DIR}/os/hal")

set(CHIBIOS_F4_STARTUP_DIR "${CHIBIOS_DIR}/os/common/startup/ARMCMx/devices/STM32F4xx")
set(CHIBIOS_F4_HAL_DIR "${CHIBIOS_DIR}/os/hal/ports/STM32/STM32F4xx")

set(CHIBIOS_H7_STARTUP_DIR "${CHIBIOS_DIR}/os/common/startup/ARMCMx/devices/STM32H7xx")
set(CHIBIOS_H7_HAL_DIR "${CHIBIOS_DIR}/os/hal/ports/STM32/STM32H7xx")

#
set(CHIBIOS_RT_DIR "${CHIBIOS_DIR}/os/rt")
set(CHIBIOS_HAL_INC_DIR "${CHIBIOS_DIR}/os/hal/include")
set(CHIBIOS_OSAL_DIR "${CHIBIOS_DIR}/os/hal/osal/rt-nil")
set(CHIBIOS_PORT_DIR "${CHIBIOS_DIR}/os/common/ports/ARMCMx")
set(CHIBIOS_HAL_LLD_DIR "${CHIBIOS_DIR}/os/hal/ports/STM32/LLD")
set(CHIBIOS_HAL_PORT_DIR "${CHIBIOS_DIR}/os/hal/ports/common/ARMCMx")

#file(GLOB_RECURSE CHIBIOS_HAL_LLD_TIM_SRC ${CHIBIOS_HAL_LLD_DIR}/TIMv1/*.c)
#file(GLOB_RECURSE CHIBIOS_HAL_PORT_SRC ${CHIBIOS_HAL_PORT_DIR}/*.c)

#file(GLOB_RECURSE CHIBIOS_F4_HAL_SRC ${CHIBIOS_F4_HAL_DIR}/*.c)
#file(GLOB_RECURSE CHIBIOS_H7_HAL_SRC ${CHIBIOS_H7_HAL_DIR}/*.c)

file(GLOB_RECURSE CHIBIOS_RT_SRC ${CHIBIOS_RT_DIR}/src/*.c)
file(GLOB_RECURSE CHIBIOS_OSAL_SRC ${CHIBIOS_OSAL_DIR}/*.c)

set(CHIBIOS_PORT_SRC
    "${CHIBIOS_PORT_DIR}/chcore.c"
    "${CHIBIOS_PORT_DIR}/chcore_v7m.c"
    "${CHIBIOS_PORT_DIR}/compilers/GCC/chcoreasm_v7m.S"
)

set(CHIBIOS_HAL_LLD_TIM_SRC
    "${CHIBIOS_HAL_LLD_DIR}/TIMv1/hal_st_lld.c"
)

set(CHIBIOS_INCLUDE_DIRS
#    "${CHIBIOS_HAL_LLD_DIR}/EXTIv1"
#    "${CHIBIOS_HAL_LLD_DIR}/DMAv2"
#    "${CHIBIOS_HAL_LLD_DIR}/RTCv2"
#    "${CHIBIOS_HAL_LLD_DIR}/FDCANv1"
#    "${CHIBIOS_HAL_LLD_DIR}/QUADSPIv2"
#    "${CHIBIOS_HAL_LLD_DIR}/SDMMCv2"
#    "${CHIBIOS_HAL_LLD_DIR}/USARTv2"
    "${CHIBIOS_OSAL_DIR}"
    "${CHIBIOS_RT_DIR}/include"
    "${CHIBIOS_PORT_DIR}"
    "${CHIBIOS_HAL_INC_DIR}"
    "${CHIBIOS_HAL_PORT_DIR}"
    "${CHIBIOS_PORT_DIR}/compilers/GCC"
    "${CHIBIOS_DIR}/os/license"
    "${CHIBIOS_DIR}/os/oslib/include"
    "${CHIBIOS_HAL_LLD_DIR}/TIMv1"
)

set(CHIBIOS_F4_INCLUDE_DIRS
    "${CHIBIOS_F4_STARTUP_DIR}"
    "${CHIBIOS_F4_HAL_DIR}"
)

set(CHIBIOS_H7_INCLUDE_DIRS
    "${CHIBIOS_H7_STARTUP_DIR}"
    "${CHIBIOS_H7_HAL_DIR}"
#    "${CHIBIOS_HAL_LLD_DIR}/BDMAv1"
#    "${CHIBIOS_HAL_LLD_DIR}/MDMAv1"
)

set(CHIBIOS_SRC
    "${CHIBIOS_RT_SRC}"
    "${CHIBIOS_OSAL_SRC}"
    "${CHIBIOS_PORT_SRC}"
    "${CHIBIOS_DIR}/os/hal/src/hal_st.c"
    "${CHIBIOS_HAL_PORT_DIR}/nvic.c"
#    "${CHIBIOS_HAL_DIR}/src/hal.c"
    "${CHIBIOS_HAL_LLD_TIM_SRC}"
    #"${CHIBIOS_HAL_LLD_DIR}/EXTIv1/stm32_exti.c"
    #"${CHIBIOS_HAL_LLD_DIR}/DMAv2/stm32_dma.c"
    #"${CHIBIOS_HAL_LLD_DIR}/RTCv2/hal_rtc_lld.c"
    #"${CHIBIOS_HAL_PORT_SRC}"
)

set(BRAINFPV_DIR "${MAIN_SRC_DIR}/brainfpv")
file(GLOB_RECURSE BRAINFPV_INAV_SRC ${BRAINFPV_DIR}/*.c)


set(BRAINFPV_F4_INCLUDE_DIRS
    "${BRAINFPV_DIR}"
    "${CHIBIOS_INCLUDE_DIRS}"
    "${CHIBIOS_F4_INCLUDE_DIRS}"
)

set(BRAINFPV_H7_INCLUDE_DIRS
    "${BRAINFPV_DIR}"
    "${STM32H7_INCLUDE_DIRS}"
    "${CHIBIOS_INCLUDE_DIRS}"
    "${CHIBIOS_H7_INCLUDE_DIRS}"
)

set(BRAINFPV_SRC
  "${MAIN_SRC_DIR}/main_chibios.c"
  "${MAIN_SRC_DIR}/cms/cms_menu_brainfpv.c"
  "${CHIBIOS_SRC}"
  "${BRAINFPV_INAV_SRC}"
)

set(BRAINFPV_F4_SRC
  "${BRAINFPV_SRC}"
  "${STM32F411_OR_F427_STDPERIPH_SRC}"
  "${STM32F4_STDPERIPH_SRC_DIR}/stm32f4xx_qspi.c"
  "${STM32F4_STDPERIPH_SRC_DIR}/stm32f4xx_rcc.c" 
#  "${CHIBIOS_STARTUP_DIR}/crt0_v7m.S"
#  "${CHIBIOS_STARTUP_DIR}/crt1.c"
  "${CHIBIOS_SRC}"
#  "${CHIBIOS_F4_HAL_SRC}"
)

set(BRAINFPV_H7_SRC
  "${BRAINFPV_SRC}"
  "${STM32H7_HAL_SRC}" 
  "${STM32H7_SRC}"
  "${STM32H7_HAL_DIR}/Src/stm32h7xx_hal_mdma.c"
  "${STM32H7_HAL_DIR}/Src/stm32h7xx_hal_comp.c"
  "${CHIBIOS_SRC}"
 # "${CHIBIOS_H7_HAL_DIR}/hal_lld.c"
 # "${CHIBIOS_H7_HAL_SRC}"
)

set(BRAINFPV_COMPILE_DEFINITIONS
    BRAINFPV
    USE_CHIBIOS
    CORTEX_USE_FPU=TRUE
    CORTEX_SIMPLIFIED_PRIORITY=TRUE
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BRAINFPV_COMPILE_DEFINITIONS
        ${BRAINFPV_COMPILE_DEFINITIONS}
        DEBUG_BUILD)
endif()

set(STM32F446_BRAINFPV_COMPILE_DEFINITIONS
    ${BRAINFPV_COMPILE_DEFINITIONS}
    STM32F446xx
    MCU_FLASH_SIZE=512
)

set(STM32H750_BRAINFPV_COMPILE_DEFINITIONS
    ${BRAINFPV_COMPILE_DEFINITIONS}
    ${STM32H7_DEFINITIONS}
    STM32H7
    STM32H750xx
    STM32H7XX
    MCU_FLASH_SIZE=1024
    USE_BRAINFPV_BOOTLOADER
)

function(target_brainfpv_stm32f446 name)
    target_stm32f4xx(
        NAME ${name}
        HSE_MHZ 16
        DISABLE_MSC
        STARTUP startup_stm32f446xx.s
        SOURCES ${BRAINFPV_F4_SRC}
        INCLUDE_DIRECTORIES ${BRAINFPV_F4_INCLUDE_DIRS}
        COMPILE_DEFINITIONS ${STM32F446_BRAINFPV_COMPILE_DEFINITIONS}
        LINKER_SCRIPT stm32_flash_f446_brainfpv
        SVD STM32F446
        
        #OPTIMIZATION -Os
        
        ${ARGN}
    )
endfunction()

function(pack_brainfpv_fw name)
    set(fw_packer_py "${MAIN_DIR}/brainfpv_fw_packer/brainfpv_fw_packer/brainfpv_fw_packer.py")
    set(elf_file "${CMAKE_BINARY_DIR}/bin/${name}.elf")
    set(header_addr_file "${CMAKE_BINARY_DIR}/bin/${name}.start_addr")
    set(hex_file "${CMAKE_BINARY_DIR}/inav_${FIRMWARE_VERSION}_${name}.hex")

    git_local_changes(git_status)
    get_git_head_revision(git_rev git_hash)
    git_get_exact_tag(git_tag)
    git_describe(build_ver)
    
    if (git_status STREQUAL "DIRTY")
        set(build_ver "${build_ver}-D")
    endif()

    set(fw_packer_output "${CMAKE_BINARY_DIR}/INAV_${build_ver}_${name}.bin")
        
    add_custom_command(TARGET ${name}
        POST_BUILD
        DEPENDS ${name}.elf
        COMMAND bash -c "PATH=$ENV{PATH} ${MAIN_DIR}/cmake/get_header_addr.sh ${elf_file} BRAINFPV_BL_HEADER ${header_addr_file}" 
        COMMAND python3 ${fw_packer_py} --in ${hex_file} --out ${fw_packer_output} --dev ${name} -b ${header_addr_file} -z --name INAV --version ${build_ver} --sha1 ${git_hash}
    )
endfunction()

function(target_brainfpv_stm32h750 name)
    target_stm32(
        NAME ${name}
        HSE_MHZ 16
        STARTUP startup_stm32h743xx.s
        SOURCES ${BRAINFPV_H7_SRC}
        COMPILE_DEFINITIONS ${STM32H750_BRAINFPV_COMPILE_DEFINITIONS}
        COMPILE_OPTIONS ${CORTEX_M7_COMMON_OPTIONS} ${CORTEX_M7_COMPILE_OPTIONS}
        INCLUDE_DIRECTORIES ${BRAINFPV_H7_INCLUDE_DIRS}
        LINK_OPTIONS ${CORTEX_M7_COMMON_OPTIONS} ${CORTEX_M7_LINK_OPTIONS}
        
        MSC_SOURCES ${STM32H7_USBMSC_SRC} ${STM32H7_MSC_SRC}
        VCP_SOURCES ${STM32H7_USB_SRC} ${STM32H7_VCP_SRC}
        VCP_INCLUDE_DIRECTORIES ${STM32H7_USB_INCLUDE_DIRS} ${STM32H7_VCP_DIR}

        OPTIMIZATION -Os

        OPENOCD_TARGET stm32h7x

        DISABLE_MSC # This should be temporary
        
        LINKER_SCRIPT stm32_flash_h750_brainfpv
        ${ARGN}
    )
    set_target_properties(${name}.elf PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON)
    pack_brainfpv_fw(${name})
endfunction()


SET(ASM_OPTIONS "-x assembler-with-cpp")


set(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} ${ASM_OPTIONS}")
set(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} ${ASM_OPTIONS}")
set(CMAKE_ASM_FLAGS_RELWITHDEBINFO "${CMAKE_ASM_FLAGS_RELWITHDEBINFO} ${ASM_OPTIONS}")
