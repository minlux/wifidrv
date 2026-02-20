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
#include "USB.h"
#include "USBMSC.h"

#define LED_DI_PIN     40
#define LED_CI_PIN     39


USBMSC MSC;
CRGB leds[1];

extern "C" uint32_t get_lba_slice(uint32_t lba, void * buffer, uint32_t bufsize);
extern "C" uint32_t set_lba_slice(uint32_t lba, const void * data, uint32_t len);


// offset ist immer 0 !?
// bufsize ist minimal 512, maxixmal 4096 und immer ein vielfaches von 512 !?
static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    // Serial.printf("onRead: lba=%lu, offset=%lu, buffer=%p, bufsize=%lu\n", lba, offset, buffer, bufsize);

    if (lba == 2152) // CREDS.JSN
    {
        leds[0] = CRGB::Yellow;
        FastLED.show();
    }
    else if (lba == 2156) // IMG1.JPG
    {
        leds[0] = CRGB::Green;
        FastLED.show();
    }
    else if (lba == 2412) // IMG2.JPG
    {
        leds[0] = CRGB::Blue;
        FastLED.show();
    }

    get_lba_slice(lba, buffer, bufsize);
    return bufsize;
}


static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
    // Serial.printf("onWrite: lba=%lu, offset=%lu, buffer=%p, bufsize=%lu\n", lba, offset, buffer, bufsize);
    // set_lba_slice(lba, buffer, bufsize);
    return bufsize;
}



static bool onStartStop(uint8_t power_condition, bool start, bool load_eject)
{
    Serial.printf("MSC START/STOP: power: %u, start: %u, eject: %u\n", power_condition, start, load_eject);
    leds[0] = CRGB::Red;
    FastLED.show();
    return true;
}

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == ARDUINO_USB_EVENTS) {
        arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
        switch (event_id) {
        case ARDUINO_USB_STARTED_EVENT: Serial.println("USB PLUGGED"); break;
        case ARDUINO_USB_STOPPED_EVENT: Serial.println("USB UNPLUGGED"); break;
        case ARDUINO_USB_SUSPEND_EVENT: Serial.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en); break;
        case ARDUINO_USB_RESUME_EVENT:  Serial.println("USB RESUMED"); break;
        default: break;
        }
    }
}

void setup()
{
    Serial.begin(115200);

    FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(leds, 1);  // BGR ordering is typical
    FastLED.setBrightness(25);

    //Note:  Arduino IDE -> Tools -> USB Mode -> Select "USB-OTG (TinyUSB)" mode before using this sketch
    leds[0] = CRGB::Red;
    FastLED.show();

    USB.onEvent(usbEventCallback);
    MSC.vendorID("ESP32");       //max 8 chars
    MSC.productID("WIFIDRV");    //max 16 chars
    MSC.productRevision("1.0");  //max 4 chars
    MSC.onStartStop(onStartStop);
    MSC.onRead(onRead);
    MSC.onWrite(onWrite);

    MSC.mediaPresent(true);
    MSC.isWritable(false);  // true if writable, false if read-only


    MSC.begin(64*1024, 512); // Identify as 32MB Stick
    USB.begin();
}

void loop()
{
    // put your main code here, to run repeatedly:
}


#endif /* ARDUINO_USB_MODE */
