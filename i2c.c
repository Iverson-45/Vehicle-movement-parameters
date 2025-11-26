#include "i2c.h"

void I2C_Init(void)
{
    I2cPins();

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // Reset I2C
    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    I2C1->CR1 &= ~I2C_CR1_PE;

    I2C1->TIMINGR = (0xF  << I2C_TIMINGR_PRESC_Pos) |
                    (0x13 << I2C_TIMINGR_SCLL_Pos)  |
                    (0xF  << I2C_TIMINGR_SCLH_Pos)  |
                    (0x2  << I2C_TIMINGR_SDADEL_Pos) |
                    (0x4  << I2C_TIMINGR_SCLDEL_Pos);

    I2C1->CR1 |= I2C_CR1_PE; // Włącz I2C
}

void I2C_ReadRegister(uint8_t slave_addr, uint8_t reg_addr, uint8_t *buffer, uint8_t len)
{
	I2C1->CR2 = (slave_addr << 1) | (1 << I2C_CR2_NBYTES_Pos);
	I2C1->CR2 &= ~I2C_CR2_RD_WRN;
	I2C1->CR2 |= I2C_CR2_START;

	while (!(I2C1->ISR & I2C_ISR_TXIS));
	I2C1->TXDR = reg_addr;
	while (!(I2C1->ISR & I2C_ISR_TC));

	I2C1->CR2 = (slave_addr << 1)
	            | (len << I2C_CR2_NBYTES_Pos)
				| I2C_CR2_RD_WRN
				| I2C_CR2_START
				| I2C_CR2_AUTOEND;

	for (uint8_t i = 0; i < len; i++)
	{
		while (!(I2C1->ISR & I2C_ISR_RXNE));
		buffer[i] = I2C1->RXDR;
	}

    while (!(I2C1->ISR & I2C_ISR_STOPF)) {}
    I2C1->ICR |= I2C_ICR_STOPCF;
}


void I2C_WriteRegister(uint8_t addr, uint8_t control, const uint8_t* data, uint16_t len)
{
    volatile uint32_t timeout;
    const uint32_t TIMEOUT_MAX = 100000;

    I2C1->CR2 = (addr << 1);
    I2C1->CR2 |= ((len + 1) << I2C_CR2_NBYTES_Pos);
    I2C1->CR2 &= ~I2C_CR2_RD_WRN;
    I2C1->CR2 |= I2C_CR2_START;

    timeout = TIMEOUT_MAX;
    while (!(I2C1->ISR & I2C_ISR_TXIS))
    {
        if (I2C1->ISR & I2C_ISR_NACKF)
        {
            I2C1->ICR |= I2C_ICR_NACKCF;
            return;
        }
        if (--timeout == 0) return;
    }
    I2C1->TXDR = control;

    for (uint16_t i = 0; i < len; i++)
    {
        timeout = TIMEOUT_MAX;
        while (!(I2C1->ISR & I2C_ISR_TXIS))
        {
            if (I2C1->ISR & I2C_ISR_NACKF)
            {
                I2C1->ICR |= I2C_ICR_NACKCF;
                return;
            }
            if (--timeout == 0) return;
        }
        I2C1->TXDR = data[i];
    }

    timeout = TIMEOUT_MAX;
    while (!(I2C1->ISR & I2C_ISR_TC))
    {
        if (I2C1->ISR & I2C_ISR_NACKF)
        {
             I2C1->ICR |= I2C_ICR_NACKCF;
             return;
        }
        if (--timeout == 0) return;
    }

    I2C1->CR2 |= I2C_CR2_STOP;

    timeout = TIMEOUT_MAX;
    while (!(I2C1->ISR & I2C_ISR_STOPF))
    {
         if (--timeout == 0) return;
    }

    I2C1->ICR |= I2C_ICR_STOPCF;
}














