/*
*****************************************************************************
**
**  File        : stm32_flash_h7x3_2m.ld
**
**  Abstract    : Linker script for STM32H743xI Device with
**                512K AXI-RAM mapped onto AXI bus on D1 domain
**                128K SRAM1 mapped on D2 domain
**                128K SRAM2 mapped on D2 domain
**                 32K SRAM3 mapped on D2 domain
**                 64K SRAM4 mapped on D3 domain
**                 64K ITCM
**                128K DTCM
**
*****************************************************************************
*/


/*
0x00000000 to 0x0000FFFF   64K ITCM
0x20000000 to 0x2001FFFF  128K DTCM
0x24000000 to 0x2407FFFF  512K AXI SRAM, D1 domain, main RAM
0x30000000 to 0x3001FFFF  128K SRAM1, D2 domain, video ram
0x30020000 to 0x3003FFFF  128K SRAM2, D2 domain, unused
0x30040000 to 0x30047FFF   32K SRAM3, D2 domain, unused
0x38000000 to 0x3800FFFF   64K SRAM4, D3 domain, unused
0x38800000 to 0x38800FFF    4K BACKUP SRAM, Backup domain, unused
*/

/* Specify the memory areas */
MEMORY
{
    ITCM_RAM (rwx)      : ORIGIN = 0x00000010, LENGTH = 64K - 16
    DTCM_RAM_INIT (rwx) : ORIGIN = 0x20000000, LENGTH = 16K  /* DTCM RAM loaded by bootloader. 32K max */
    DTCM_RAM (rwx)      : ORIGIN = 0x20004000, LENGTH = 112K /* Not loaded by bootloader */
    
    CODE_RAM (rwx)    : ORIGIN = 0x24000000, LENGTH = 512K /* AXI SRAM */
    D2_RAM (rwx)      : ORIGIN = 0x30000000, LENGTH = 240K /* SRAM1 + SRAM2*/
    
    D2_DMA_RAM (rwx)  : ORIGIN = 0x3003C000, LENGTH = 16K  /* Needs to be at address that aligns to size for MPU to work */

    MEMORY_B1 (rx)    : ORIGIN = 0x60000000, LENGTH = 0K
}

REGION_ALIAS("STACKRAM", DTCM_RAM)
REGION_ALIAS("SLOW_RAM",  D2_RAM)

/* INCLUDE "stm32_flash_f7_split.ld" */
/*
*****************************************************************************
**
**  File        : stm32_flash_f7_split.ld
**
**  Abstract    : Common linker script for STM32 devices.
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(STACKRAM) + LENGTH(STACKRAM) - 8; /* Reserve 2 x 4bytes for info across reset */

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0;      /* required amount of heap  */
_Min_Stack_Size = 0xF00; /* required amount of stack */

__process_stack_size__  = 0x4000;

/* Define output sections */
SECTIONS
{
  /* The vector table is first */
  .isr_vector :
  {
    . = ALIGN(512);
    PROVIDE (isr_vector_table_base = .);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >CODE_RAM
  
  /* Bootloader header */
  .bl_header :
  {
  	. = ALIGN(4);
  	KEEP(*(.bl_header_section)) /* BL Header */
    . = ALIGN(4);
  } >CODE_RAM
  
  /* The program code and other data goes into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)           /* .text sections (code) */
    *(.text*)          /* .text* sections (code) */
    *(.rodata)         /* .rodata sections (constants, strings, etc.) */
    *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
    *(.glue_7)         /* glue arm to thumb code */
    *(.glue_7t)        /* glue thumb to arm code */
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;        /* define a global symbols at end of code */
  } >CODE_RAM

  /* Critical program code goes into ITCM RAM */
  /* Copy specific fast-executing code to ITCM RAM */ 
  tcm_code = LOADADDR(.tcm_code); 
  .tcm_code :
  {
    . = ALIGN(4);
    tcm_code_start = .; 
    *(.tcm_code)
    *(.tcm_code*)
    . = ALIGN(4);
    tcm_code_end = .; 
  } >ITCM_RAM

  .ARM.extab   : 
  { 
    *(.ARM.extab* .gnu.linkonce.armextab.*) 
  } >CODE_RAM
  
  .ARM : 
  {
    __exidx_start = .;
    *(.ARM.exidx*) __exidx_end = .;
  } >CODE_RAM

  /* Initialized data sections.  DTCM_RAM_INIT: loaded by bootloader*/
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    _edata = _sdata;   /* global symbol is normally at the end. We set it to _sdata, so startup code doesn't copy anything */
    _sidata = _sdata;  /* used by startup code. normally points to data in flash */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    
  } >DTCM_RAM_INIT

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss (NOLOAD) :
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _sbss = .;         /* define a global symbol at bss start */
    __bss_start__ = _sbss;
    *(.bss)
    *(SORT_BY_ALIGNMENT(.bss*))
    *(COMMON)

    . = ALIGN(4);
    _ebss = .;         /* define a global symbol at bss end */
    __bss_end__ = _ebss;
  } >DTCM_RAM

  . = ALIGN(4);
  .fastram_bss (NOLOAD) :
  {
    _sfastram_bss = .;
    __fastram_bss_start__ = _sfastram_bss;
    *(.fastram_bss)
    *(SORT_BY_ALIGNMENT(.fastram_bss*))

    . = ALIGN(4);
    _efastram_bss = .;
    __fastram_bss_end__ = _efastram_bss;
  } >DTCM_RAM
    
    
  /* Special section for process stack.*/
  .pstack :
  {
	__process_stack_base__ = .;
    __main_thread_stack_base__ = .;
    . += __process_stack_size__;
    . = ALIGN(8);
    __process_stack_end__ = .;
    __main_thread_stack_end__ = .;
  } >DTCM_RAM
  
  /* User_heap_stack section, used to check that there is enough RAM left */
  _heap_stack_end = ORIGIN(STACKRAM)+LENGTH(STACKRAM) - 8; /* 8 bytes to allow for alignment */
  _heap_stack_begin = _heap_stack_end - _Min_Stack_Size  - _Min_Heap_Size;
  . = _heap_stack_begin;
  ._user_heap_stack :
  {
    . = ALIGN(4);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(4);
  } >DTCM_RAM = 0xa5
    
  .busdev_registry :
  {
    PROVIDE_HIDDEN (__busdev_registry_start = .);
    KEEP (*(.busdev_registry))
    KEEP (*(SORT(.busdev_registry.*)))
    PROVIDE_HIDDEN (__busdev_registry_end = .);
  } >D2_RAM

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >D2_RAM
  
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >D2_RAM
  
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(.fini_array*))
    KEEP (*(SORT(.fini_array.*)))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >D2_RAM
  
  .pg_registry :
  {
    PROVIDE_HIDDEN (__pg_registry_start = .);
    KEEP (*(.pg_registry))
    KEEP (*(SORT(.pg_registry.*)))
    PROVIDE_HIDDEN (__pg_registry_end = .);
  } >D2_RAM
  
  .pg_resetdata :
  {
    PROVIDE_HIDDEN (__pg_resetdata_start = .);
    KEEP (*(.pg_resetdata))
    PROVIDE_HIDDEN (__pg_resetdata_end = .);
  } >D2_RAM
  slow_const = LOADADDR(.slow_const); 
  .slow_const :
  {
    . = ALIGN(4);
    slow_const_start = .; 
    *(.slow_const)
    *(.slow_const*)
    . = ALIGN(4);
    slow_const_end = .; 
  } > D2_RAM

  slow_code = LOADADDR(.slow_code); 
  .slow_code :
  {
    . = ALIGN(4);
    slow_code_start = .; 
    *(.slow_code)
    *(.slow_code*)
    . = ALIGN(4);
    slow_code_end = .; 
  } > D2_RAM
  
  images : ALIGN(4) SUBALIGN(4)
  {
    KEEP(*(.images_section))
  } > D2_RAM
    
  .SLOW_RAM (NOLOAD) :
  {
  	KEEP(*(.SLOW_RAM))
  } > SLOW_RAM
  
  video_ram (NOLOAD) :
  {
  	. = ALIGN(32);
  	PROVIDE(videoram_start = .);
    *(.video_ram)
    *(.video_ram.*)
    PROVIDE(videoram_end = .);
  } > D2_RAM
  
  .DMA_RAM (NOLOAD) :
  {
    . = ALIGN(32);
    PROVIDE(dmaram_start = .);
    _sdmaram = .;
    _dmaram_start__ = _sdmaram;
    KEEP(*(.DMA_RAM))
    PROVIDE(dmaram_end = .);
    _edmaram = .;
    _dmaram_end__ = _edmaram;
  } >D2_DMA_RAM

  .persistent_data (NOLOAD) :
  {
    __persistent_data_start__ = .;
    *(.persistent_data)
    . = ALIGN(4);
    __persistent_data_end__ = .;
  } >D2_RAM

  /* MEMORY_bank1 section, code must be located here explicitly            */
  /* Example: extern int foo(void) __attribute__ ((section (".mb1text"))); */
  .memory_b1_text :
  {
    *(.mb1text)        /* .mb1text sections (code) */
    *(.mb1text*)       /* .mb1text* sections (code)  */
    *(.mb1rodata)      /* read-only data (constants) */
    *(.mb1rodata*)
  } >MEMORY_B1

  /* Remove information from the standard libraries */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
