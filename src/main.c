#include "uart.h"
#include "spi.h"
#include "sd.h"
#include "gpt.h"
#include "platform.h"

// print 4KB per 8MB address range, 16 bytes per row
#define PRINT_GRANULARITY 8*1024*1024
#define PRINT_SIZE 4096
#define PRINT_ROW_LENGTH 16

int main()
{
    init_uart(UART_FREQUENCY, UART_BAUD);
    print_uart("Hello World!\r\n");

#ifdef TEST_PRINT_BASE
    for(uint8_t* scan_base_ptr = (uint8_t*)TEST_PRINT_BASE,
               *scan_bound_ptr = (uint8_t*)TEST_PRINT_BASE + TEST_PRINT_LENGTH;
        scan_base_ptr < scan_bound_ptr; scan_base_ptr += PRINT_GRANULARITY){
        print_uart("Dumping 4K from address ");
        print_uart_addr((uint64_t)scan_base_ptr);
        print_uart(":\r\n");
	    for(uint8_t* srcPtr = scan_base_ptr,
                   * endPtr = scan_base_ptr + PRINT_SIZE;
            srcPtr < endPtr; srcPtr += PRINT_ROW_LENGTH){
            print_uart_addr((uint64_t)srcPtr);
            print_uart(":");
            for(unsigned i = 0; i < PRINT_ROW_LENGTH; ++i){
                uint8_t data = srcPtr[i];
                print_uart(" ");
                print_uart_byte(data);
            }
            print_uart("\r\n");
        }
    }
    print_uart("done\r\n");
    int res = 1;
#else /* TEST_PRINT_BASE */
#ifdef INITROM_BASE
    // copy INITROM_LENGTH from INITROM_BASE to DRAM_BASE
    uint64_t* srcPtr = (uint64_t*) INITROM_BASE;
    uint64_t* srcEndPtr = (uint64_t*) (INITROM_BASE+INITROM_LENGTH);
    print_uart("Copying from ");
    print_uart_addr(INITROM_BASE);
    print_uart(" with length ");
    print_uart_addr(INITROM_LENGTH);
    print_uart(" to main memory\r\n");
    uint64_t* destPtr = (uint64_t*)0x80000000UL;
    while(srcPtr < srcEndPtr){
        *destPtr++ = *srcPtr++;
        if(((uint64_t)srcPtr - INITROM_BASE) % (512*1024) == 0){
            print_uart(".");
        }
    }
    print_uart("done\r\n");
    int res = 0;
#else /* INITROM_BASE */
    int res = gpt_find_boot_partition((uint8_t *)0x80000000UL, 2 * 16384);
#endif /* INITROM_BASE */
#endif /* TEST_PRINT_BASE */
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
