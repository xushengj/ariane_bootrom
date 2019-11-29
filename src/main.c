#include "uart.h"
#include "spi.h"
#include "sd.h"
#include "gpt.h"
#include "platform.h"

int main()
{
    init_uart(UART_FREQUENCY, UART_BAUD);
    print_uart("Hello World!\r\n");

#ifdef INITROM_BASE
    // copy INITROM_LENGTH from INITROM_BASE to DRAM_BASE
    uint64_t* srcPtr = (uint64_t*) INITROM_BASE;
    uint64_t* srcEndPtr = (uint64_t*) (INITROM_BASE+INITROM_LENGTH);
    uint64_t* destPtr = (uint64_t*)0x80000000UL;
    while(srcPtr < srcEndPtr){
        *destPtr++ = *srcPtr++;
    }
    int res = 0;
#else
    int res = gpt_find_boot_partition((uint8_t *)0x80000000UL, 2 * 16384);
#endif

    if (res == 0)
    {
        // jump to the address
        __asm__ volatile(
            "li s0, 0x80000000;"
            "la a1, _dtb;"
            "jr s0");
    }

    while (1)
    {
        // do nothing
    }
}

void handle_trap(void)
{
    // print_uart("trap\r\n");
}
