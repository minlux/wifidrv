#include "cli.h"
#include "credentials.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

extern void wifi_status(char *buf, size_t len);

static char line_buf[256];
static int  line_len = 0;

static void dispatch(const char *line)
{
    // set <key> <value>
    if (strncmp(line, "set ", 4) == 0)
    {
        const char *rest = line + 4;
        const char *sp = strchr(rest, ' ');
        if (!sp)
        {
            Serial.print("ERR: usage: set <ssid|password|url> <value>\r\n");
            return;
        }

        char key[32];
        int klen = sp - rest;
        if (klen <= 0 || klen >= (int)sizeof(key))
        {
            Serial.print("ERR: key too long\r\n");
            return;
        }

        strncpy(key, rest, klen);
        key[klen] = '\0';
        const char *value = sp + 1;

        if (strcmp(key, "ssid") == 0)
        {
            creds_set_ssid(value);
            return;
        }
        if (strcmp(key, "password") == 0)
        {
            creds_set_password(value);
            return;
        }
        if (strcmp(key, "url") == 0)
        {
            creds_set_url(value);
            return;
        }
        // Otherwise
        Serial.printf("ERR: unknown key \"%s\"\r\n", key);
        return;
    }

    // get <key>
    if (strncmp(line, "get ", 4) == 0)
    {
        const char *key = line + 4;

        if (strcmp(key, "wifi") == 0)
        {
            char buf[64];
            wifi_status(buf, sizeof(buf));
            Serial.printf("WiFi: %s\r\n", buf);
            return;
        }

        char value[256];
        if (strcmp(key, "ssid") == 0)
        {
            creds_get_ssid(value, sizeof(value));
            Serial.printf("%s = \"%s\"\r\n", key, value);
            return;
        }
        if (strcmp(key, "password") == 0)
        {
            creds_get_password(value, sizeof(value));
            Serial.printf("%s = \"%s\"\r\n", key, value);
            return;
        }
        if (strcmp(key, "url") == 0)
        {
            creds_get_url(value, sizeof(value));
            Serial.printf("%s = \"%s\"\r\n", key, value);
            return;
        }
        // Otherwise
        Serial.printf("ERR: unknown key \"%s\"\r\n", key);
        return;
    }

    Serial.print("ERR: unknown command.\r\nCommands:\r\n - set <ssid|password|url> <value>\r\n - get <ssid|password|url|wifi>\r\n");
}

void cli_begin(void)
{
    creds_begin();
    Serial.print("CLI ready.\r\nCommands:\r\n - set <ssid|password|url> <value>\r\n - get <ssid|password|url|wifi>\r\n");
}

void cli_process(void)
{
    while (Serial.available())
    {
        char c = (char)Serial.read();
        if (c == '\r' || c == '\n')
        {
            Serial.print("\r\n");
            if (line_len > 0)
            {
                line_buf[line_len] = '\0';
                dispatch(line_buf);
                line_len = 0;
            }
        }
        else
        {
            if (line_len >= (int)sizeof(line_buf) - 1)
            {
                Serial.print("ERR: line too long, discarding\r\n");
                line_len = 0;
            }
            else
            {
                line_buf[line_len++] = c;
                Serial.print(c);  // echo
            }
        }
    }
}
