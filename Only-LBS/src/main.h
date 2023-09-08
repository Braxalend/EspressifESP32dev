#pragma once

#include <Arduino.h>

#define SIM868_BAUD_RATE 115200
#define SIM868_RX_PIN 17
#define SIM868_TX_PIN 16
#define SIM868_DTR_PIN 35
#define SIM868_EN 2             // HW вкл/выкл питания на SIM868 (ключ для VBAT)
#define SIM868_PWRKEY 21        // Пин PRWKEY на SIM868 (SW управление SIM868)
#define GNSS_BAUD_RATE 115200
#define GNSS_RX_PIN 33
#define GNSS_TX_PIN 32