#include "clock.h"

void SystemClock_Config_64MHz(void)
{
    // --- 1. Konfiguracja FLASH ---
    // Dla 64 MHz wymagane są 2 Wait States (LATENCY = 010)
    FLASH->ACR |= FLASH_ACR_PRFTBE;      // Włącz Prefetch Buffer
    FLASH->ACR &= ~FLASH_ACR_LATENCY;    // Wyzeruj bity opóźnienia
    FLASH->ACR |= FLASH_ACR_LATENCY_1;   // Ustaw 010 (2 Wait States)

    // --- 2. Konfiguracja HSI (High Speed Internal) ---
    // Upewnij się, że HSI jest włączone (to domyślny stan, ale dla pewności)
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0); // Czekaj na gotowość HSI

    // --- 3. Konfiguracja PLL ---
    // Najpierw wyłącz PLL, aby zmienić ustawienia
    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0); // Czekaj aż PLL zgaśnie

    // Wybór źródła i mnożnika
    // Źródło: HSI / 2 (Bit PLLSRC = 0). W F303 HSI zawsze wchodzi do PLL podzielone przez 2.
    // Wejście PLL = 8 MHz / 2 = 4 MHz.
    // Cel: 64 MHz. Mnożnik: x16.
    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL); // Reset
    RCC->CFGR |= RCC_CFGR_PLLMUL16; // Ustaw x16 (Bits: 1111)
    // Bit PLLSRC zostawiamy na 0 (HSI/2)

    // --- 4. Konfiguracja Preskalerów Magistral ---
    // HCLK (AHB) = SYSCLK / 1 = 64 MHz
    RCC->CFGR &= ~RCC_CFGR_HPRE;

    // PCLK2 (APB2) = HCLK / 1 = 64 MHz
    RCC->CFGR &= ~RCC_CFGR_PPRE2;

    // PCLK1 (APB1) = HCLK / 2 = 32 MHz
    // (APB1 max to 36 MHz, więc musimy dzielić przez 2)
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

    // --- 5. Włącz PLL ---
    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0); // Czekaj na stabilizację PLL

    // --- 6. Przełącz zegar systemowy na PLL ---
    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    // Czekaj aż sprzęt potwierdzi wybór PLL
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

void PeriphClock_Config(void)
{
    // --- 7. Zegary Peryferiów (RCC_CFGR3) ---

    // I2C1 -> SYSCLK (64 MHz)
    // Dzięki temu I2C może pracować bardzo szybko (np. Fast Mode Plus)
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW;

    // USART1 -> SYSCLK (64 MHz)
    // Pozwala na bardzo wysokie baudrate'y
    RCC->CFGR3 &= ~RCC_CFGR3_USART1SW;
    RCC->CFGR3 |= RCC_CFGR3_USART1SW_0; // 01: SYSCLK

    // USART2
    // Na STM32F303x8 (wersja K8) USART2 zazwyczaj nie ma opcji zmiany źródła w CFGR3.
    // Będzie pracował na zegarze magistrali APB1 (PCLK1).
    // PCLK1 = 32 MHz. To wciąż wystarcza na baudrate rzędu kilku Mbit/s.

    // --- 8. Włączanie zegarów (Clock Gating) ---
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // EXTI
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;   // I2C1
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN; // USART1
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // USART2
}


