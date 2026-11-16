/* Linker script for the STM32F767ZI on a NUCLEO-F767ZI.
 *
 * Memory map, from the part's reference manual:
 *   FLASH: 0x08000000, 2 MiB   - the vector table sits at its start, which is
 *                                where the part boots from with BOOT0 low.
 *   DTCM:  0x20000000, 128 KiB - left unused. It is the fastest RAM on the
 *                                part and is kept free for whoever needs it.
 *   SRAM:  0x20020000, 384 KiB - SRAM1 and SRAM2, which are contiguous. Its
 *                                first 32 KiB is ETHDMA below; the rest holds
 *                                data, bss, the newlib heap and the stack.
 *
 * ETHDMA is what the Ethernet DMA reads and writes: its descriptors and the
 * frame buffers. Board.c gives the region to the MPU as memory the data cache
 * leaves alone, and an MPU region is a power of two long and starts on a
 * multiple of its length, which is why it is cut from the bottom of SRAM.
 * Board.c takes the region from the two symbols below, and the assertions
 * under them hold its length to what the MPU can take.
 *
 * The symbol names are the ones ST's startup_stm32f767xx.s reads.
 */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
    ETHDMA (rw) : ORIGIN = 0x20020000, LENGTH = 32K
    SRAM  (rwx) : ORIGIN = 0x20028000, LENGTH = 352K
}

_ethernet_dma_start = ORIGIN(ETHDMA);
_ethernet_dma_end = ORIGIN(ETHDMA) + LENGTH(ETHDMA);

ASSERT((LENGTH(ETHDMA) & (LENGTH(ETHDMA) - 1)) == 0, "ETHDMA has to be a power of two long: it is an MPU region")
ASSERT((ORIGIN(ETHDMA) % LENGTH(ETHDMA)) == 0, "ETHDMA has to start on a multiple of its length: it is an MPU region")

_estack = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        *(.glue_7) *(.glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        PROVIDE(__exidx_start = .);
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        PROVIDE(__exidx_end = .);
    } > FLASH

    .preinit_array :
    {
        PROVIDE(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE(__fini_array_end = .);
    } > FLASH

    /* Not loaded and not cleared at startup: the Ethernet driver writes every
     * descriptor before the DMA is given it, and a buffer is read only after
     * a frame has been written to it. The last three are the names
     * FreeRTOS-Plus-TCP's network interface for this MAC gives its descriptors
     * and its network buffers, every one of which the DMA may be handed. */
    .ethernet_dma (NOLOAD) :
    {
        *(.ethernet_dma*)
        *(.TxDescripSection)
        *(.RxDescripSection)
        *(.EthBuffersSection)
    } > ETHDMA

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    _sidata = LOADADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM

    /* newlib heap base. _sbrk grows from here toward the stack. */
    . = ALIGN(8);
    _end = .;
    PROVIDE(end = _end);
}
