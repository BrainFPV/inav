/*
*****************************************************************************
**
**  File        : stm32_flash_f411.ld
**
**  Abstract    : Linker script for STM32F11 Device with
**                512KByte FLASH, 128KByte RAM
**
*****************************************************************************
*/

/* Entry Point */
ENTRY(Reset_Handler)

/*
0x08000000 to 0x0807FFFF  512K full flash,
0x08000000 to 0x08003FFF   16K isr vector, startup code,
0x08004000 to 0x08007FFF   16K config,                  // FLASH_Sector_1
0x08008000 to 0x0807FFFF  480K firmware,
*/

/* Specify the memory areas */
MEMORY
{
    FLASH (rx)        : ORIGIN = 0x08000000, LENGTH = 16K
    FLASH_CONFIG (r)  : ORIGIN = 0x08004000, LENGTH = 16K
    FLASH1 (rx)       : ORIGIN = 0x08008000, LENGTH = 480K
    RAM (rwx)         : ORIGIN = 0x20000000, LENGTH = 128K
    MEMORY_B1 (rx)    : ORIGIN = 0x60000000, LENGTH = 0K
}

REGION_ALIAS("STACKRAM", RAM)
REGION_ALIAS("FASTRAM", RAM)

/* Stack & Heap sizes */
_Min_Heap_Size = 0;
_Min_Stack_Size = 0x800;

__process_stack_size__  = 0x400;


/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(STACKRAM) + LENGTH(STACKRAM);    /* end of RAM */

/* Base address where the config is stored. */
__config_start = ORIGIN(FLASH_CONFIG);
__config_end = ORIGIN(FLASH_CONFIG) + LENGTH(FLASH_CONFIG);

/* Define output sections */
SECTIONS
{
  /* The startup code goes first into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    PROVIDE (isr_vector_table_base = .);
    KEEP(*(.isr_vector)) /* Startup code */
    . = ALIGN(4);
  } >FLASH

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
  } >FLASH1

   .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
    .ARM : {
    __exidx_start = .;
      *(.ARM.exidx*)
      __exidx_end = .;
    } >FLASH

  .busdev_registry :
  {
    PROVIDE_HIDDEN (__busdev_registry_start = .);
    KEEP (*(.busdev_registry))
    KEEP (*(SORT(.busdev_registry.*)))
    PROVIDE_HIDDEN (__busdev_registry_end = .);
  } >FLASH

  images : ALIGN(16) SUBALIGN(16)
  {
    KEEP(*(.images_section))
  } > FLASH

  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH1
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH1
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(.fini_array*))
    KEEP (*(SORT(.fini_array.*)))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH1
  .pg_registry :
  {
    PROVIDE_HIDDEN (__pg_registry_start = .);
    KEEP (*(.pg_registry))
    KEEP (*(SORT(.pg_registry.*)))
    PROVIDE_HIDDEN (__pg_registry_end = .);
  } >FLASH1
  .pg_resetdata :
  {
    PROVIDE_HIDDEN (__pg_resetdata_start = .);
    KEEP (*(.pg_resetdata))
    PROVIDE_HIDDEN (__pg_resetdata_end = .);
  } >FLASH1

  fonts : ALIGN(4)
  {
    KEEP(*(.font_section))
  } > FLASH1

  /* used by the startup to initialize data */
  _sidata = .;

  /* Initialized data sections goes into RAM, load LMA copy after code */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        /* create a global symbol at data start */
    *(.data)           /* .data sections */
    *(.data*)          /* .data* sections */

    . = ALIGN(4);
    _edata = .;        /* define a global symbol at data end */
  } >RAM AT> FLASH1

  /* Uninitialized data section */
  . = ALIGN(4);
  .bss :
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
  } >RAM

  .fastram_bss (NOLOAD) :
  {
    __fastram_bss_start__ = .;
    *(.fastram_bss)
    *(SORT_BY_ALIGNMENT(.fastram_bss*))
    . = ALIGN(4);
    __fastram_bss_end__ = .;
  } >FASTRAM

  video_ram (NOLOAD) : ALIGN(4)
  {
    *(.video_ram)
    *(.video_ram.*)
  } > RAM

  /* Special section for process stack.*/
  .pstack :
  {
	__process_stack_base__ = .;
    __main_thread_stack_base__ = .;
    . += __process_stack_size__;
    . = ALIGN(8);
    __process_stack_end__ = .;
    __main_thread_stack_end__ = .;
  } >STACKRAM

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
  } >STACKRAM = 0xa5

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
