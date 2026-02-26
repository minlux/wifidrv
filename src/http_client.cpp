#include "http_client.h"
#include "credentials.h"
#include <Arduino.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

extern "C" {
    extern unsigned char FILE_IMG_JPG[];    // defined in storage.c
    extern unsigned int  FILE_IMG_JPG_len;  // actual fetched size, read by storage.c
}

volatile uint32_t http_fetch_trigger = 0;

void http_client_process(void)
{
    if ((http_fetch_trigger >= 10000) && ((millis() - http_fetch_trigger) > 2500)) //2.5 second after last access (not before 10s after startup)
    {
        http_fetch_trigger = 0;

        char url[256];
        creds_get_url(url, sizeof(url));
        if (url[0] == '\0')
        {
            Serial.println("HTTP: no URL configured");
            FILE_IMG_JPG_len = 0;
            return;
        }

        Serial.printf("HTTP: fetching %s\n", url);

        HTTPClient http;
        http.begin(url);
        int code = http.GET();

        if (code == HTTP_CODE_OK)
        {
            WiFiClient *stream = http.getStreamPtr();
            const size_t capacity = 128 * 1024; //sizeof(FILE_IMG_JPG)
            size_t written = 0;

            while ((http.connected() || stream->available()) && written < capacity)
            {
                size_t avail = stream->available();
                if (avail == 0) { delay(1); continue; }
                written += stream->readBytes(FILE_IMG_JPG + written, min(avail, capacity - written));
            }

            if (written < capacity)
            {
                memset(FILE_IMG_JPG + written, 0, capacity - written);
            }
            FILE_IMG_JPG_len = written;
            Serial.printf("HTTP: fetched %u bytes\n", written);
        }
        else
        {
            Serial.printf("HTTP: GET failed, code %d\n", code);
        }

        http.end();
    }
}
