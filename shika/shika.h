#ifndef _SHIKA_H
#define _SHIKA_H

#include <Arduino.h>
#define TIME_SYNC_ACCURACY 10 // 10 milliseconds
#define TIME_SYNC_INTERVAL 10 * TASK_SECOND  // Mesh time resync period, in us. 10 seconds
#include "painlessMesh.h"  //timesync branch with adjusted power
#define USE_GET_MILLISECOND_TIMER  // Define our own millis() source for FastLED beat functions: see get_millisecond_timer()
#include "FastLED.h"

#define MESH_PREFIX ""
#define MESH_PASSWORD ""
#define MESH_PORT 5555

#define GLOBAL_MODE_NONE 0
#define GLOBAL_MODE_OFF 0
#define GLOBAL_MODE_ALARM 1
#define GLOBAL_MODE_PARTY 2
#define GLOBAL_MODE_BUCK 3
#define GLOBAL_MODE_PALETTE 4

uint32_t get_millisecond_timer(void);

/*
 *  Available ESP32 RF power parameters:
    WIFI_POWER_19_5dBm    // 19.5dBm (For 19.5dBm of output, highest. Supply current ~150mA)
    WIFI_POWER_19dBm      // 19dBm
    WIFI_POWER_18_5dBm    // 18.5dBm
    WIFI_POWER_17dBm      // 17dBm
    WIFI_POWER_15dBm      // 15dBm
    WIFI_POWER_13dBm      // 13dBm
    WIFI_POWER_11dBm      // 11dBm
    WIFI_POWER_8_5dBm     //  8dBm
    WIFI_POWER_7dBm       //  7dBm
    WIFI_POWER_5dBm       //  5dBm
    WIFI_POWER_2dBm       //  2dBm
    WIFI_POWER_MINUS_1dBm // -1dBm( For -1dBm of output, lowest. Supply current ~120mA)  DOESNT WORK

    Available ESP8266 RF power parameters:
    0    (for lowest RF power output, supply current ~ 70mA
    20.5 (for highest RF power output, supply current ~ 80mA
*/

#endif