#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstring>
#include <sys/unistd.h>
#include <SPI.h>
#include "RadioLib.h"

// --------------------------------------------------------------------------------------------------------
// ******** ESP32 pinout I2C V2 (пины платы V2)
// --------------------------------------------------------------------------------------------------------
// static const uint8_t SDA = 5;
// static const uint8_t SCL = 18;

// --------------------------------------------------------------------------------------------------------
// ******** ESP32 pinout SX1278 V2 (пины платы V2)
// --------------------------------------------------------------------------------------------------------
// NSS pin:   27
// DIO0 pin:  33
// RESET pin: 26
// DIO1 pin:  23  для работы в режиме LoRa на приёмнике надо напаять перемычку
// пины заданны в C:\.platformio\packages\framework-arduinoespressif32\variants\esp32\pins_arduino.h


static const uint8_t ESP_NSS   = 27;
static const uint8_t ESP_MOSI  = 15;
static const uint8_t ESP_MISO  = 12;
static const uint8_t ESP_SCK   = 13;
static const uint8_t ESP_RESET = 26;
static const uint8_t ESP_DIO0  = 33;
static const uint8_t ESP_DIO1  = 23;
static const uint8_t ESP_CHRG_LVL  = 36;
static const uint8_t ESP_CHRG_STAT = 39;
static const uint8_t ESP_LDO = 22;

SX1278 radio = nullptr;

int state_setup;
int state_transmit;
int state_receive;
char char_transmit[63];
char char_receive[63];
String str_transmit;
String str_receive;
String str_transmit_temp;
static uint32_t start_time = millis();
uint32_t current_time;
int radiorssi;
typedef unsigned int u32;

void wait(u32 miliseconds)
{
    vTaskDelay(miliseconds / portTICK_PERIOD_MS);
}

void check_transmit(void) {
    if (state_transmit == RADIOLIB_ERR_NONE) {
        log_printf("[SX1278] Packet transmitted successfully! \n");
    } else if (state_transmit == RADIOLIB_ERR_PACKET_TOO_LONG) {
        log_printf("[SX1278] Packet too long!\n");
    } else if (state_transmit == RADIOLIB_ERR_TX_TIMEOUT) {
        log_printf("[SX1278] Timed out while transmitting!\n");
    } else if (state_transmit == RADIOLIB_ERR_CHIP_NOT_FOUND) {
        log_printf("[SX1278] Radio chip not found!\n");
    } else {
        log_printf("[SX1278] Failed to transmit packet, code state = %s\n", state_transmit);
    }
}

void check_receive(void) {
    if (state_receive == RADIOLIB_ERR_NONE) {
        radiorssi = floor(radio.getRSSI(true, true));
        log_printf("String - %s; RSSI = %d\n", str_receive.c_str(), radiorssi);
    } else if (state_receive == RADIOLIB_ERR_RX_TIMEOUT) {
        log_printf("[SX1278] Timed out while waiting for packet!\n");
    } else if (state_receive == RADIOLIB_ERR_CHIP_NOT_FOUND) {
        log_printf("[SX1278] Radio chip not found!\n");
    } else {
        log_printf("[SX1278] Failed to receive packet, code state = %d\n", state_receive);
    }
}

typedef union {
    uint16_t count;
    uint8_t s_buf[2];
} TR_data_union_t;

TR_data_union_t td;
TR_data_union_t rd;

void init_transmit_task(void *Params) {
    while (true) {
        if (td.count >= 32000) {
            td.count = 0;
        }
        str_transmit = "Test string!!! - ";
        str_transmit_temp = String(td.count, 10);
        str_transmit = str_transmit + str_transmit_temp;
        state_transmit = radio.transmit(str_transmit);
        check_transmit();
        td.count++;
        wait(500);
    }
}

void init_recieve_task(void *Params) {
    while (true) {
        state_receive = radio.receive(str_receive);
        check_receive();
        wait(500);
    }
}

void setup() {
    pinMode(ESP_CHRG_LVL, INPUT);
    pinMode(ESP_CHRG_STAT, INPUT);
    pinMode(ESP_LDO, OUTPUT); // Пин ESP_LDO (управление LDO на SX1278 и MPU-9250)
    digitalWrite(ESP_LDO, LOW);
    wait(500);
    digitalWrite(ESP_LDO, HIGH);
    log_printf("[SX1278] Radio Initializing ...\n");
    SPI.begin(ESP_SCK, ESP_MISO, ESP_MOSI, ESP_NSS);
    radio = new Module(ESP_NSS, ESP_DIO0, ESP_RESET, ESP_DIO1, SPI, RADIOLIB_DEFAULT_SPI_SETTINGS);
    state_setup = radio.begin();
    log_printf("[SX1278] LoRa mode selected!\n");
    if (state_setup == RADIOLIB_ERR_NONE) {
        log_printf("[SX1278] Radio init success!\n");
    } else if (state_setup == RADIOLIB_ERR_CHIP_NOT_FOUND) {
        log_printf("[SX1278] Radio chip not found!\n");
        while (true);
    } else {
        log_printf("[SX1278] failed, code state = %d\n", state_setup);
        while (true);
    }
    state_setup = radio.setFrequency(444);
    state_setup = radio.setOutputPower(20);
    state_setup = radio.setCurrentLimit(120);
    state_setup = radio.setPreambleLength(16);
    state_setup = radio.setCRC(true,true);
    // xTaskCreatePinnedToCore(&init_transmit_task, "init_transmit_task", 4096, NULL, 0, NULL, 0);
    xTaskCreatePinnedToCore(&init_recieve_task, "init_recieve_task", 4096, NULL, 0, NULL, 0);
}

void loop() {
    wait(1000);
}