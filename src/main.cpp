/**
 * @file      usb_mass_storage_led.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2025-12-15
 * @note      Arduino IDE -> Tools -> USB Mode -> Select "USB-OTG (TinyUSB)" mode before using this sketch
 */
#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else

#include "esp_arduino_version.h"
#if ESP_ARDUINO_VERSION < ESP_ARDUINO_VERSION_VAL(3, 3, 0)
#error This sketch requires ESP32 Arduino Core version 3.3.0 or later
#endif

#include <FastLED.h>
#include <WiFi.h>
#include "USB.h"
#include "usb_msc.h"
#include "cli.h"
#include "credentials.h"
#include "http_client.h"

#define LED_DI_PIN  40
#define LED_CI_PIN  39
CRGB leds[1];


void wifi_connect(void)
{
    char ssid[128], password[128];
    creds_get_ssid(ssid,         sizeof(ssid));
    creds_get_password(password, sizeof(password));

    if (ssid[0] == '\0' || password[0] == '\0')
    {
        Serial.println("WiFi: no credentials stored, skipping");
        return;
    }

    Serial.printf("WiFi: connecting to \"%s\" ...", ssid);
    WiFi.begin(ssid, password);
}

void wifi_status(char *buf, size_t len)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        snprintf(buf, len, "connected, IP %s", WiFi.localIP().toString().c_str());
    }
    else
    {
        snprintf(buf, len, "not connected (status %d)", (int)WiFi.status());
    }
}


void setup()
{
    Serial.begin(115200);

    FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(leds, 1);  // BGR ordering is typical
    FastLED.setBrightness(25);
    leds[0] = CRGB::Red;
    FastLED.show();

    usb_msc_begin();
    USB.begin();
    cli_begin();
    wifi_connect();
}

void loop()
{
    static bool wifi_connected;
    const bool connected = (WiFi.status() == WL_CONNECTED);
    if (connected != wifi_connected)
    {
        if (connected)
        {
            Serial.printf("WiFi: connected, IP %s\n", WiFi.localIP().toString().c_str());
        }
        else
        {
            Serial.println("WiFi: disconnected");
        }
    }
    wifi_connected = connected;

    cli_process();
    http_client_process();
}


#endif /* ARDUINO_USB_MODE */
