#include "credentials.h"
#include <Preferences.h>
#include <stdio.h>
#include <string.h>

extern "C" {
    extern unsigned char FILE_CREDS_JSN[2048];
}

static Preferences prefs;

static void refresh_json(void)
{
    char ssid[128], password[128], url[256];
    prefs.getString("ssid",     ssid,     sizeof(ssid));
    prefs.getString("password", password, sizeof(password));
    prefs.getString("url",      url,      sizeof(url));

    int n = snprintf((char *)FILE_CREDS_JSN, sizeof(FILE_CREDS_JSN),
                     "{\"ssid\":\"%s\",\"password\":\"%s\",\"url\":\"%s\"}\n",
                     ssid, password, url);
    if (n < (int)sizeof(FILE_CREDS_JSN))
        memset(FILE_CREDS_JSN + n, 0, sizeof(FILE_CREDS_JSN) - n);
}

void creds_begin(void)
{
    prefs.begin("wifidrv", false);
    refresh_json();
}

void creds_set(const char *key, const char *value)
{
    prefs.putString(key, value);
    refresh_json();
}

void creds_get(const char *key, char *buf, size_t len)
{
    prefs.getString(key, buf, len);
}
