#include "uart.h"
#include "spi.h"
#include "sd.h"
#include "gpt.h"
#include "cdecode.h"
#include "platform.h"

#ifdef TEST_PRINT_BASE
static int memory_test();
#endif
#ifdef INITROM_BASE
static int copy_from_rom();
#endif
#ifdef UART_LOAD_IMAGE
static int load_image_from_uart();
#endif

int main()
{
    init_uart(UART_FREQUENCY, UART_BAUD);
    print_uart("Hello World!\r\n");

#ifdef ECHO_TEST
    print_uart("Echo test:\r\n");
    while(1){
        uint8_t ch = getchar_uart();
        print_uart("byte: ");
        print_uart_byte(ch);
        print_uart("\r\n");
        if(ch == 0x1b) // esc
            break;
    }
#endif

#if defined(UART_LOAD_IMAGE)
    int res = load_image_from_uart();
#elif defined(TEST_PRINT_BASE)
    int res = memory_test();
#elif defined(INITROM_BASE)
    int res = copy_from_rom();
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

#ifdef TEST_PRINT_BASE

// print 4KB per 8MB address range, 16 bytes per row
#define PRINT_GRANULARITY 8*1024*1024
#define PRINT_SIZE 4096
#define PRINT_ROW_LENGTH 16

static int memory_test()
{
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
    return 0;
}
#endif /* TEST_PRINT_BASE */

#ifdef INITROM_BASE
static int copy_from_rom()
{
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
    return 0;
}
#endif /* INITROM_BASE */

#ifdef UART_LOAD_IMAGE
#define DIGIT_CNT 16
static int load_image_from_uart()
{
    print_uart("image from UART:\r\n");
    base64_decodestate state;
    base64_init_decodestate(&state);
    uint64_t linesize = 0;
    char* destPtr = (char*)0x80000000UL;
    // progress indicator
    uint64_t printedSizeInKB = 0;
    uint8_t size_KB_digits[DIGIT_CNT] = {0};
    while(1){
        uint64_t currentLineSize = 0;
        while(1){
            uint8_t ch = getchar_uart();
            if (ch == '\r') continue;
            if (ch == '\n') {
                if (linesize == 0){
                    linesize = currentLineSize;
                }
                break;
            }else{
                currentLineSize += 1;
                destPtr += base64_decode_block((char*)&ch, 1, destPtr, &state);
            }
        }
        if (linesize > currentLineSize) break;
        uint64_t curSizeInKB = (destPtr - (char*)0x80000000UL) >> 10;
        if (curSizeInKB > printedSizeInKB){
            uint64_t increment = curSizeInKB - printedSizeInKB;
            printedSizeInKB = curSizeInKB;
            while(increment > 0){
                size_KB_digits[0] += 1;
                increment -= 1;
                for(int i = 0; i<(DIGIT_CNT-1) && size_KB_digits[i]>=10; ++i){
                    size_KB_digits[i] = 0;
                    size_KB_digits[i+1] += 1;
                }
            }
            int start = 0;
            for(int i = DIGIT_CNT-1; i > 0; --i){
                if(size_KB_digits[i] > 0){
                    start = i;
                    break;
                }
            }
            for(int i = start; i >= 0; --i){
                uint8_t digit = size_KB_digits[i];
                if (digit >= 10) {
                    putchar_uart('X');
                }else{
                    putchar_uart(digit + '0');
                }
            }
            print_uart(" KB\r");
        }
    }
    print_uart("\r\ndone\r\n");
    return 0;
}
#endif /* UART_LOAD_IMAGE */
