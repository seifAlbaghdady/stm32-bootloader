#include <stdint.h>
#include <string.h>

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
extern uint32_t _estack;

int main(void);
void Default_Handler(void);
void SysTick_Handler(void);

void Reset_Handler(void)
{

    uint32_t data_len = (uint32_t)&_edata - (uint32_t)&_sdata;
    memcpy(&_sdata, &_sidata, data_len);

    uint32_t bss_len = (uint32_t)&_ebss - (uint32_t)&_sbss;
    memset(&_sbss, 0, bss_len);

    main();

    for (;;)
    {
    }
}

void Default_Handler(void)
{
    for (;;)
    {
    }
}

__attribute__((section(".isr_vector"), used)) static void *const vector_table[] = {
    (void *)&_estack,
    Reset_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    0,
    0,
    0,
    0,
    Default_Handler,
    Default_Handler,
    0,
    Default_Handler,
    SysTick_Handler,
};
