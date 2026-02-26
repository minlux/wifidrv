#include "usb_msc.h"
#include <Arduino.h>
#include <FastLED.h>
#include "USB.h"
#include "USBMSC.h"

USBMSC MSC;
extern CRGB leds[1];

extern "C" uint32_t get_lba_slice(uint32_t lba, void *buffer, uint32_t bufsize);
extern "C" uint32_t set_lba_slice(uint32_t lba, const void *data, uint32_t len);

extern volatile int http_fetch_trigger;


// offset ist immer 0 !?
// bufsize ist minimal 512, maxixmal 4096 und immer ein vielfaches von 512 !?
static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
    if (lba == 2152) // CREDS.JSN
    {
        leds[0] = CRGB::Yellow;
        FastLED.show();
    }
    else if (lba == 2156) // IMG1.JPG
    {
        http_fetch_trigger = millis(); //on access to first lba of IMG1
        leds[0] = CRGB::Green;
        FastLED.show();
    }
    else if (lba == 2412) // IMG2.JPG
    {
        http_fetch_trigger = millis(); //on access to first lba of IMG2
        leds[0] = CRGB::Blue;
        FastLED.show();
    }

    get_lba_slice(lba, buffer, bufsize);
    return bufsize;
}

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
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
    if (event_base == ARDUINO_USB_EVENTS) 
    {
        arduino_usb_event_data_t *data = (arduino_usb_event_data_t *)event_data;
        switch (event_id) 
        {
        case ARDUINO_USB_STARTED_EVENT:  Serial.println("USB PLUGGED"); break;
        case ARDUINO_USB_STOPPED_EVENT:  Serial.println("USB UNPLUGGED"); break;
        case ARDUINO_USB_SUSPEND_EVENT:  Serial.printf("USB SUSPENDED: remote_wakeup_en: %u\n", data->suspend.remote_wakeup_en); break;
        case ARDUINO_USB_RESUME_EVENT:   Serial.println("USB RESUMED"); break;
        default: break;
        }
    }
}

extern "C" void prepare_files(void);
void usb_msc_begin(void)
{
    prepare_files();
    USB.onEvent(usbEventCallback);

    MSC.vendorID("ESP32");       // max 8 chars
    MSC.productID("WIFIDRV");    // max 16 chars
    MSC.productRevision("1.0");  // max 4 chars
    MSC.onStartStop(onStartStop);
    MSC.onRead(onRead);
    MSC.onWrite(onWrite);
    MSC.mediaPresent(true);
    MSC.isWritable(false);
    MSC.begin(64*1024, 512);     // Identify as 32MB Stick
}
