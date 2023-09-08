#include "main.h"
#include <Arduino.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <cstring>
#include <sys/unistd.h>

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long u64;

HardwareSerial SIM868_MODULE(2);

void wait(u32 miliseconds);
void init_SIM868();
String sendATCommand(const String &cmd, u32 waiting = 5000);
void process_CREG();

void setup() {
    init_SIM868();
}

void loop() {
    sendATCommand("AT+CNETSCAN", 46000);
    wait(1000);
}

void wait(u32 miliseconds)
{
    vTaskDelay(miliseconds / portTICK_PERIOD_MS);
}

void init_SIM868() {
    SIM868_MODULE.begin(SIM868_BAUD_RATE, SERIAL_8N1, SIM868_RX_PIN, SIM868_TX_PIN);
    log_i("SIM868 module initing ...");
    pinMode(SIM868_EN, OUTPUT);         // SIM868_EN pin (VBAT switch)
    pinMode(SIM868_PWRKEY, OUTPUT);        // SIM868 PWRKEY pin
    digitalWrite(SIM868_EN, LOW);       // GPIO2 (SIM868_EN set VBAT OFF)
    digitalWrite(SIM868_PWRKEY, HIGH);  // PWRKEY pin SIM868 set to 0V level
    wait(1000);
    digitalWrite(SIM868_EN, HIGH);      // GPIO2 (SIM868_EN set VBAT ON)
    digitalWrite(SIM868_PWRKEY, LOW);   // PWRKEY pin SIM868 set to VBAT level
    wait(1500);
    digitalWrite(SIM868_PWRKEY, HIGH);  // PWRKEY pin SIM868 set to 0V level
    wait(2000);
    digitalWrite(SIM868_PWRKEY, LOW);   // PWRKEY pin SIM868 set to VBAT level
    wait(2000);
    log_i("SIM868 module inited!");

    sendATCommand("ATE0");              // E0 - ECHO off, E1 - ECHO on.
    sendATCommand("ATI");            // Display the product name and the product release information.
    sendATCommand("AT+GSV");         // Display product identification: the manufacturer, the product name, the product revision information.
    sendATCommand("AT+CMEE=1");         // Report Mobile Equipment Error
    // 0 = Disable +CME ERROR (<err> result code and use ERROR instead).
    // 1 = Enable +CME ERROR (<err> result code and use numeric <err>).
    // 2 = Enable +CME ERROR (<err> result code and use verbose <err>).
    sendATCommand("AT+CBC");            // Battery Charge current status and level in mV

    sendATCommand("AT+CNETSCAN=1");
    sendATCommand("AT+CNETSCAN?");    // Perform a Net Survey to Show All the Cells’ Information
    // sendATCommand("AT+CNETSCAN", 46000);

    process_CREG();
    // sendATCommand("AT+CIPCSGP=1,\"internet\",\"gdata\",\"gdata\""); // Set CSD or GPRS for Connection Mode 0 - CSD, 1 - GPRS
    sendATCommand("AT+CIPCSGP=1,\"internet.mts.ru\",\"mts\",\"mts\""); // Set CSD or GPRS for Connection Mode 0 - CSD, 1 - GPRS
    // sendATCommand("AT+CIPCSGP=1,\"internet.beeline.ru\",\"beeline\",\"beeline\""); // Set CSD or GPRS for Connection Mode 0 - CSD, 1 - GPRS
    // // +CIPCSGP:0-CSD,DIALNUMBER,USER NAME,PASSWORD,RATE(0-3) or +CIPCSGP: 1-GPRS,APN,USER NAME,PASSWORD
    sendATCommand("AT+CSTT");               // Start Task and Set APN, USER NAME, PASSWORD
}

String sendATCommand(const String &cmd, u32 waiting) {
    String _resp = "";                                  // Переменная для хранения результата
    if (cmd.length() > 128) {
        log_i("AT-command size greater than 128 bytes: %d bytes", cmd.length());   
    }
    while (cmd.length() > SIM868_MODULE.availableForWrite()) { // Ждём, когда команда уместится в буфере записи SIM868 (до 128 байт)
    };
    SIM868_MODULE.readString();                         // Обнуляем (вычитываем) буфер чтения SIM868
    SIM868_MODULE.println(cmd);                         // Отправляем команду модулю
    log_i("CMD: %s", cmd.c_str());                      // Дублируем команду в монитор порта
    u64 _timeout = millis() + waiting;                  // ... ждем, когда будет передан ответ, но не дольше чем waiting
    while (!SIM868_MODULE.available() && (millis() < _timeout)) {
        wait(10);
    };

    _timeout = waiting - (_timeout - millis());
    log_i("Completed for: %d ms", _timeout);
    // wait(10);
    if (SIM868_MODULE.available()) {
        _resp = SIM868_MODULE.readString();
        log_i("Answer: %s", _resp.c_str());
    } else {
        _resp = SIM868_MODULE.readString();
        log_e("Timeout...");                        // Оповещаем, если пришел таймаут
        return _resp;
    }
    // log_i("%s", _resp.c_str());                     // Оповещаем о результате
    return _resp;
}

void process_CREG() {
    u8 attempt_counter = 0;
    bool channelAttached = false;
    String answ;
    while (!channelAttached)
    {
        // log_i("COPS: %s", sendATCommand("AT+COPS?").c_str());
        answ = sendATCommand("AT+CREG?");
        if(answ.indexOf("+CREG: 0,1") > -1)
        {
            channelAttached = true;
            log_i("Network Registration Success!");
        } else {
            wait(5000);
        }
        attempt_counter ++;
        if(attempt_counter == 24)               // Повторяем попытки подключения к сети в течении 2-х минут
        {
            channelAttached = false;
            log_i("Network Registration Timeout...");
        }
    }
}
