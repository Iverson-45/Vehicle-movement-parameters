#include "i2c.h"

void I2C_Init(void)
{
    I2cPins(); // B6-SCL, B7-SDA

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    I2C1->CR1 &= ~I2C_CR1_PE;

    //PB1 50 MHz
    I2C1->CR2 = 50;

    //Standard Mode 100kHz
    // CCR = PCLK1 / (2 * 100kHz) = 50MHz / 200kHz = 250
    I2C1->CCR = 250;

    //TRISE
    // for 100kHz max 1000ns. TRISE = (1000ns / TPCLK1) + 1
    // TRISE = (1000 / 20ns) + 1 = 51
    I2C1->TRISE = 51;

    I2C1->CR1 |= I2C_CR1_PE;
}

void I2C_ReadRegister(uint8_t slave_addr, uint8_t reg_addr, uint8_t *buffer, uint8_t len)
{
    I2C1->CR1 |= I2C_CR1_START;                   // Start
    while (!(I2C1->SR1 & I2C_SR1_SB));            // wait for Start Bit

    I2C1->DR = (slave_addr << 1);                 // Address + Write
    while (!(I2C1->SR1 & I2C_SR1_ADDR));          // wait for sent address
    (void)I2C1->SR2;                              // Clear flag ADDR

    while (!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = reg_addr;                          // register number
    while (!(I2C1->SR1 & I2C_SR1_BTF));           // wait for end of transmit


    I2C1->CR1 |= I2C_CR1_START;                   // Repeated Start
    while (!(I2C1->SR1 & I2C_SR1_SB));

    I2C1->DR = (slave_addr << 1) | 1;             // Adres + Read
    while (!(I2C1->SR1 & I2C_SR1_ADDR));

    if (len == 1)
    {
        I2C1->CR1 &= ~I2C_CR1_ACK;                // No ACK for last byte
        (void)I2C1->SR2;                          // clear ADDR
        I2C1->CR1 |= I2C_CR1_STOP;                // generate STOP
    }
    else
    {
        (void)I2C1->SR2;                          // clear ADDR
        I2C1->CR1 |= I2C_CR1_ACK;
    }

    for (uint8_t i = 0; i < len; i++)
    {
        if (len > 1 && i == len - 1)
        {
            I2C1->CR1 &= ~I2C_CR1_ACK;            // NACK for last byte
            I2C1->CR1 |= I2C_CR1_STOP;
        }
        while (!(I2C1->SR1 & I2C_SR1_RXNE));      // wait for data
        buffer[i] = I2C1->DR;
    }
}

void I2C_WriteRegister(uint8_t addr, uint8_t reg_addr, const uint8_t* data, uint16_t len)
{
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));

    I2C1->DR = (addr << 1);
    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    (void)I2C1->SR2;

    while (!(I2C1->SR1 & I2C_SR1_TXE));
    I2C1->DR = reg_addr;

    for (uint16_t i = 0; i < len; i++)
    {
        while (!(I2C1->SR1 & I2C_SR1_TXE));
        I2C1->DR = data[i];
    }

    while (!(I2C1->SR1 & I2C_SR1_BTF));
    I2C1->CR1 |= I2C_CR1_STOP;
}














