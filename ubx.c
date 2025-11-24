#include "ubx.h"

// ------------------- UBX Commands -------------------

// 1. Zmiana częstotliwości na 5Hz (200ms)
// Class: 0x06 (CFG), ID: 0x08 (RATE)
const uint8_t UBX_RATE_5HZ[] = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A
};

// 2. Tryb Automotive + Auto 2D/3D
// Class: 0x06 (CFG), ID: 0x24 (NAV5)
// DynModel: 04 (Automotive), FixMode: 03 (Auto 2D/3D)
const uint8_t UBX_NAV5_AUTOMOTIVE[] = {
    0xB5, 0x62, 0x06, 0x24, 0x24, 0x00,
    0xFF, 0xFF, 0x04, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00,
    0x64, 0x00, 0x2C, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x16, 0xDC
};

// 3. Konfiguracja Wiadomości (CFG-MSG) - Wyłączanie zbędnych, włączanie VTG
// Format: Class(1) ID(1) Rate(1) - ustawia rate na obecnym porcie (UART)

// Wyłącz NMEA GGA
const uint8_t UBX_MSG_DISABLE_GGA[] = {
    0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x00, 0x00, 0xFA, 0x0F
};

// Wyłącz NMEA GLL
const uint8_t UBX_MSG_DISABLE_GLL[] = {
    0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x01, 0x00, 0xFB, 0x11
};

// Wyłącz NMEA GSA
const uint8_t UBX_MSG_DISABLE_GSA[] = {
    0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x02, 0x00, 0xFC, 0x13
};

// Wyłącz NMEA GSV
const uint8_t UBX_MSG_DISABLE_GSV[] = {
    0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x03, 0x00, 0xFD, 0x15
};

// Wyłącz NMEA RMC
const uint8_t UBX_MSG_DISABLE_RMC[] = {
    0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x04, 0x00, 0xFE, 0x17
};

// Włącz NMEA VTG (Rate 1)
const uint8_t UBX_MSG_ENABLE_VTG[] = {
    0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x05, 0x01, 0xFF, 0x19
};


// ------------------- UART Send -------------------
void UBX_SendCommand(const uint8_t *data, uint16_t len)
{
    for(uint16_t i=0; i<len; i++)
    {
        while(!(USART1->ISR & USART_ISR_TXE)); // wait until TX ready
        USART1->TDR = data[i];
    }
}

// ------------------- GPS Configure -------------------
void UBX_Init(void)
{
    // Czekamy chwilę na rozruch GPS po zasilaniu
    delay_ms(500);

    // 1. Ustawiamy parametry nawigacyjne (Automotive)
    UBX_SendCommand(UBX_NAV5_AUTOMOTIVE, sizeof(UBX_NAV5_AUTOMOTIVE));
    delay_ms(100);

    // 2. Zmieniamy częstotliwość na 5Hz
    UBX_SendCommand(UBX_RATE_5HZ, sizeof(UBX_RATE_5HZ));
    delay_ms(100);

    // 3. Wyłączamy domyślne śmieci NMEA (po kolei)
    UBX_SendCommand(UBX_MSG_DISABLE_GGA, sizeof(UBX_MSG_DISABLE_GGA));
    delay_ms(50);
    UBX_SendCommand(UBX_MSG_DISABLE_GLL, sizeof(UBX_MSG_DISABLE_GLL));
    delay_ms(50);
    UBX_SendCommand(UBX_MSG_DISABLE_GSA, sizeof(UBX_MSG_DISABLE_GSA));
    delay_ms(50);
    UBX_SendCommand(UBX_MSG_DISABLE_GSV, sizeof(UBX_MSG_DISABLE_GSV));
    delay_ms(50);
    UBX_SendCommand(UBX_MSG_DISABLE_RMC, sizeof(UBX_MSG_DISABLE_RMC));
    delay_ms(50);

    // 4. Włączamy tylko VTG
    UBX_SendCommand(UBX_MSG_ENABLE_VTG, sizeof(UBX_MSG_ENABLE_VTG));
    delay_ms(100);

    UART2_SendString("GPS Configured: 5Hz, Automotive, VTG Only.\r\n");
}


