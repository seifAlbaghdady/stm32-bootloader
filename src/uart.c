#include "uart.h"

/*
 * USART2 — PA2 (TX), PA3 (RX)
 * APB1 clock assumed 16 MHz (HSI, no PLL)
 */

/* RCC registers */
#define RCC_BASE        0x40023800UL
#define RCC_AHB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB1ENR     (*(volatile uint32_t*)(RCC_BASE + 0x40))
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)
#define RCC_APB1ENR_USART2EN (1UL << 17)

/* GPIOA registers */
#define GPIOA_BASE      0x40020000UL
#define GPIOA_MODER     (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL      (*(volatile uint32_t*)(GPIOA_BASE + 0x20))
#define GPIOA_OSPEEDR   (*(volatile uint32_t*)(GPIOA_BASE + 0x08))

/* USART2 registers */
#define USART2_BASE     0x40004400UL
#define USART2_SR       (*(volatile uint32_t*)(USART2_BASE + 0x00))
#define USART2_DR       (*(volatile uint32_t*)(USART2_BASE + 0x04))
#define USART2_BRR      (*(volatile uint32_t*)(USART2_BASE + 0x08))
#define USART2_CR1      (*(volatile uint32_t*)(USART2_BASE + 0x0C))

#define USART_SR_RXNE   (1UL << 5)
#define USART_SR_TC     (1UL << 6)
#define USART_SR_TXE    (1UL << 7)
#define USART_CR1_UE    (1UL << 13)
#define USART_CR1_RE    (1UL << 2)
#define USART_CR1_TE    (1UL << 3)

/* SysTick for timeout (1 ms ticks, assumes 16 MHz HSI) */
#define SYSTICK_BASE    0xE000E010UL
#define SYST_RVR        (*(volatile uint32_t*)(SYSTICK_BASE + 0x04))
#define SYST_CVR        (*(volatile uint32_t*)(SYSTICK_BASE + 0x08))
#define SYST_CSR        (*(volatile uint32_t*)(SYSTICK_BASE + 0x00))

static volatile uint32_t systick_ms = 0;

void SysTick_Handler(void) { systick_ms++; }

static void systick_init(void) {
    SYST_RVR = 16000 - 1;   /* 16 MHz / 16000 = 1 kHz */
    SYST_CVR = 0;
    SYST_CSR = 0x07;         /* enable, tick interrupt, processor clock */
}

void uart_init(uint32_t baud) {
    systick_init();

    /* Enable clocks */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2, PA3 → Alternate Function 7 (USART2) */
    GPIOA_MODER  &= ~((3UL << 4) | (3UL << 6));
    GPIOA_MODER  |=  ((2UL << 4) | (2UL << 6));   /* AF mode */
    GPIOA_OSPEEDR|=  ((3UL << 4) | (3UL << 6));   /* high speed */
    GPIOA_AFRL   &= ~((0xFUL << 8) | (0xFUL << 12));
    GPIOA_AFRL   |=  ((7UL  << 8) | (7UL  << 12)); /* AF7 */

    /* Baud rate: BRR = F_CLK / baud (over-sampling x16) */
    USART2_BRR = 16000000UL / baud;

    /* Enable TX + RX + UART */
    USART2_CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void uart_send_byte(uint8_t b) {
    while (!(USART2_SR & USART_SR_TXE)) {}
    USART2_DR = b;
}

void uart_send(const uint8_t* buf, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) uart_send_byte(buf[i]);
    while (!(USART2_SR & USART_SR_TC)) {}  /* wait for last byte to leave shift register */
}

uint8_t uart_recv_byte(void) {
    while (!(USART2_SR & USART_SR_RXNE)) {}
    return (uint8_t)USART2_DR;
}

bool uart_recv_timeout(uint8_t* out, uint32_t timeout_ms) {
    uint32_t start = systick_ms;
    while (!(USART2_SR & USART_SR_RXNE)) {
        if ((systick_ms - start) >= timeout_ms) return false;
    }
    *out = (uint8_t)USART2_DR;
    return true;
}

void uart_send_str(const char* s) {
    while (*s) uart_send_byte((uint8_t)*s++);
}
