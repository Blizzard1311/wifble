#pragma once

#ifdef __has_include
#if __has_include("heat_cloud_config.local.h")
#include "heat_cloud_config.local.h"
#endif
#endif

#ifndef HEAT_CLOUD_ENABLE
#define HEAT_CLOUD_ENABLE 0
#endif

#ifndef HEAT_CLOUD_WIFI_SSID
#define HEAT_CLOUD_WIFI_SSID ""
#endif

#ifndef HEAT_CLOUD_WIFI_PASSWORD
#define HEAT_CLOUD_WIFI_PASSWORD ""
#endif

#ifndef HEAT_CLOUD_EZDATA_TOKEN
#define HEAT_CLOUD_EZDATA_TOKEN ""
#endif

#ifndef HEAT_CLOUD_DEVICE_TYPE
#define HEAT_CLOUD_DEVICE_TYPE "basic"
#endif

#ifndef HEAT_CLOUD_TOPIC_PREFIX
#define HEAT_CLOUD_TOPIC_PREFIX "adv_heat"
#endif

#ifndef HEAT_CLOUD_WIFI_TIMEOUT_MS
#define HEAT_CLOUD_WIFI_TIMEOUT_MS 12000
#endif
