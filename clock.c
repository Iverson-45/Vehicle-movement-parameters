#include "clock.h"

void SystemClock_Config_100MHz(void)
{
	RCC->CR |= RCC_CR_HSEON;
	while (!(RCC->CR & RCC_CR_HSERDY));

	FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_LATENCY_3WS;

	RCC->CR &= ~RCC_CR_PLLON;
	while (RCC->CR & RCC_CR_PLLRDY);

	RCC->PLLCFGR = (4 << RCC_PLLCFGR_PLLQ_Pos) |        // PLLQ = 4
				   (0 << RCC_PLLCFGR_PLLP_Pos) |        // PLLP = 2 (00)
				   (192 << RCC_PLLCFGR_PLLN_Pos) |      // PLLN = 192
				   (25 << RCC_PLLCFGR_PLLM_Pos) |       // PLLM = 25
				   RCC_PLLCFGR_PLLSRC_HSE;              // source: HSE

	RCC->CR |= RCC_CR_PLLON;
	while (!(RCC->CR & RCC_CR_PLLRDY));

	RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
	RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
	RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR |= RCC_CFGR_SW_PLL;
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}




