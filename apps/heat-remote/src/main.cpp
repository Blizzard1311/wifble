#define DISABLE_CODE_FOR_RECEIVER
#define SEND_PWM_BY_TIMER

#include <algorithm>
#include <cctype>
#include <vector>

#include <ArduinoJson.h>
#define TINY_GSM_MODEM_ML307
#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <ArduinoHttpClient.h>
#include <HTTPClient.h>
#include <IRremote.hpp>
#include <M5Cardputer.h>
#include <M5GFX.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <SD.h>
#include <SPI.h>
#include <TinyGsmClient.h>
#include <WiFi.h>

#include "heat_cloud_config.h"

namespace {

enum class AppScreen {
  Home,
  HeatMenu,
  HeatSummary,
  HeatWiFi,
  HeatBle,
  HeatLog,
  HeatSession,
  HeatFiles,
  HeatFileSummary,
  Remote,
  Codex,
};

struct WiFiRecord {
  String ssid;
  int32_t rssi;
  uint8_t channel;
  wifi_auth_mode_t auth;
};

struct BLERecord {
  String name;
  String address;
  int32_t rssi;
};

struct ScanSnapshot {
  String stamp;
  int heat_score;
  int wifi_count;
  int ble_count;
};

struct SessionFileInfo {
  String name;
  String path;
};

struct SessionSummary {
  String name = "-";
  String start_stamp = "-";
  String last_stamp = "-";
  int rows = 0;
  int avg_heat = 0;
  int peak_heat = 0;
  bool valid = false;
};

enum class CloudState {
  Off,
  Idle,
  Connecting,
  Ready,
  Uploading,
  Ok,
  Error,
};

enum class RemoteProtocolKind {
  Nec,
  Onkyo,
  Samsung32,
  SamsungLG,
  SamsungMSB32,
  Raw38,
};

struct RemoteCommandDef {
  const char* label;
  RemoteProtocolKind protocol;
  uint16_t address;
  uint32_t command;
  const uint16_t* raw_data;
  uint16_t raw_length;
  uint8_t frequency_khz;
  uint8_t repeats;
};

struct RemoteProfileDef {
  const char* name;
  const RemoteCommandDef* commands;
  size_t command_count;
};

constexpr size_t kVisibleListRows = 4;
constexpr size_t kMaxHistoryEntries = 60;
constexpr size_t kMaxSessionFiles = 60;
constexpr uint32_t kAutoScanIntervalMs = 60000;
constexpr uint32_t kCat1AutoUploadIntervalMs = 15UL * 60UL * 1000UL;
constexpr int kCat1ImmediateUploadHeatDelta = 50;
constexpr uint32_t kBleScanSeconds = 4;
constexpr int32_t kWiFiNearRssi = -82;
constexpr int32_t kBleNearRssi = -88;
constexpr int kIrTxPin = 44;
constexpr int kSdSpiSckPin = 40;
constexpr int kSdSpiMisoPin = 39;
constexpr int kSdSpiMosiPin = 14;
constexpr int kSdSpiCsPin = 12;
constexpr int kCat1RxPin = 1;
constexpr int kCat1TxPin = 2;
constexpr uint32_t kCat1BaudRate = 115200;
constexpr uint32_t kCat1CommandTimeoutMs = 2500;
constexpr const char* kCat1Apn = "cmnet";
constexpr const char* kCat1TestHost = "124.223.4.88";
constexpr int kCat1TestPort = 80;
constexpr const char* kCat1TestPath = "/api/m5/cat1/test";
constexpr const char* kHeatLogDir = "/heatlogs";
constexpr const char* kHeatLogAllFileName = "heat_all.csv";
constexpr const char* kHeatLogAllPath = "/heatlogs/heat_all.csv";
constexpr uint32_t kCloudReconnectBackoffMs = 15000;
constexpr uint16_t kCloudMqttPort = 1883;
constexpr uint32_t kCloudReplyTimeoutMs = 3000;
constexpr const char* kCloudMqttHost = "uiflow2.m5stack.com";

const char* heat_level_name(int score);
const char* heat_trend_name(int previous, int current);
constexpr int kHeaderLineY = 18;
constexpr int kSubtitleY = 22;
constexpr int kStatusY = 36;
constexpr int kListTopY = 48;
constexpr int kRowHeight = 20;

constexpr RemoteCommandDef make_parsed_command(const char* label,
                                               RemoteProtocolKind protocol,
                                               uint16_t address,
                                               uint16_t command,
                                               uint8_t repeats = 0) {
  return {label, protocol, address, command, nullptr, 0, 38, repeats};
}

constexpr RemoteCommandDef make_samsung_msb_command(const char* label,
                                                    uint32_t command) {
  return {label, RemoteProtocolKind::SamsungMSB32, 0, command, nullptr, 0, 38, 0};
}

constexpr RemoteCommandDef make_raw38_command(const char* label,
                                              const uint16_t* raw_data,
                                              uint16_t raw_length) {
  return {label, RemoteProtocolKind::Raw38, 0, 0, raw_data, raw_length, 38, 0};
}

constexpr uint16_t kXiaomiTvOpenRaw[] = {
    985, 605, 587, 589, 587, 1470, 587, 1472, 586, 589, 587, 1471,
    586, 589, 586, 1470, 587, 589, 586, 1472, 586, 1472, 585, 10765,
};

constexpr uint16_t kXiaomiTvHomeRaw[] = {
    987, 605, 587, 1177, 587, 587, 588, 883, 587, 1177, 586, 589,
    587, 589, 587, 1470, 587, 883, 587, 589, 587, 1471, 586, 12529,
};

constexpr uint16_t kXiaomiTvOkRaw[] = {
    987, 606, 586, 1178, 586, 589, 586, 882, 587, 1176, 588, 589,
    586, 589, 587, 1472, 585, 883, 587, 589, 586, 1471, 586, 12529,
};

constexpr uint16_t kXiaomiTvBackRaw[] = {
    987, 606, 586, 1177, 587, 589, 586, 882, 587, 1176, 587, 588,
    588, 589, 587, 882, 587, 1471, 587, 1177, 586, 882, 587, 12528,
};

constexpr uint16_t kXiaomiTvVolUpRaw[] = {
    988, 605, 587, 1176, 588, 589, 586, 882, 587, 1178, 585, 588,
    588, 589, 587, 1470, 587, 1175, 589, 588, 587, 590, 586, 13118,
};

constexpr uint16_t kXiaomiTvVolDnRaw[] = {
    990, 603, 589, 1175, 589, 587, 588, 882, 587, 1175, 588, 588,
    588, 589, 586, 1470, 587, 1471, 587, 588, 588, 881, 588, 12528,
};

constexpr RemoteCommandDef kSamsungTvCommands[] = {
    make_parsed_command("POWER", RemoteProtocolKind::Samsung32, 0x0007, 0x0002, 1),
    make_parsed_command("SOURCE", RemoteProtocolKind::Samsung32, 0x0007, 0x0001, 1),
    make_parsed_command("HOME", RemoteProtocolKind::Samsung32, 0x0007, 0x0079, 1),
    make_parsed_command("VOL+", RemoteProtocolKind::Samsung32, 0x0007, 0x0007, 1),
    make_parsed_command("VOL-", RemoteProtocolKind::Samsung32, 0x0007, 0x000B, 1),
    make_parsed_command("MUTE", RemoteProtocolKind::Samsung32, 0x0007, 0x000F, 1),
    make_parsed_command("UP", RemoteProtocolKind::Samsung32, 0x0007, 0x0060, 1),
    make_parsed_command("DOWN", RemoteProtocolKind::Samsung32, 0x0007, 0x0061, 1),
    make_parsed_command("LEFT", RemoteProtocolKind::Samsung32, 0x0007, 0x0065, 1),
    make_parsed_command("RIGHT", RemoteProtocolKind::Samsung32, 0x0007, 0x0062, 1),
    make_parsed_command("OK", RemoteProtocolKind::Samsung32, 0x0007, 0x0068, 1),
    make_parsed_command("RETURN", RemoteProtocolKind::Samsung32, 0x0007, 0x0058, 1),
    make_parsed_command("MENU", RemoteProtocolKind::Samsung32, 0x0007, 0x001A, 1),
    make_parsed_command("TOOLS", RemoteProtocolKind::Samsung32, 0x0007, 0x004B, 1),
    make_parsed_command("EXIT", RemoteProtocolKind::Samsung32, 0x0007, 0x002D, 1),
};

constexpr RemoteCommandDef kSamsungAltCommands[] = {
    make_parsed_command("POWER", RemoteProtocolKind::Samsung32, 0x0007, 0x0002, 1),
    make_parsed_command("SOURCE", RemoteProtocolKind::Samsung32, 0x0007, 0x0001, 1),
    make_parsed_command("HOME", RemoteProtocolKind::Samsung32, 0x0007, 0x0079, 1),
    make_parsed_command("TOOLS", RemoteProtocolKind::Samsung32, 0x0007, 0x004B, 1),
    make_parsed_command("INFO", RemoteProtocolKind::Samsung32, 0x0007, 0x001F, 1),
    make_parsed_command("CHLIST", RemoteProtocolKind::Samsung32, 0x0007, 0x006B, 1),
    make_parsed_command("RETURN", RemoteProtocolKind::Samsung32, 0x0007, 0x0058, 1),
    make_parsed_command("EXIT", RemoteProtocolKind::Samsung32, 0x0007, 0x002D, 1),
};

constexpr RemoteCommandDef kSamsungMsbCommands[] = {
    make_samsung_msb_command("POWER", 0xE0E040BF),
    make_samsung_msb_command("VOL+", 0xE0E0E01F),
    make_samsung_msb_command("VOL-", 0xE0E0D02F),
    make_samsung_msb_command("MUTE", 0xE0E0F00F),
    make_samsung_msb_command("MENU", 0xE0E058A7),
    make_samsung_msb_command("RETURN", 0xE0E01AE5),
};

constexpr RemoteCommandDef kTclTvCommands[] = {
    make_parsed_command("POWER", RemoteProtocolKind::Onkyo, 0xEAC7, 0x17E8),
    make_parsed_command("VOL+", RemoteProtocolKind::Onkyo, 0xEAC7, 0x0FF0),
    make_parsed_command("VOL-", RemoteProtocolKind::Onkyo, 0xEAC7, 0x10EF),
    make_parsed_command("HOME", RemoteProtocolKind::Onkyo, 0xEAC7, 0x03FC),
    make_parsed_command("OK", RemoteProtocolKind::Onkyo, 0xEAC7, 0x2AD5),
    make_parsed_command("UP", RemoteProtocolKind::Onkyo, 0xEAC7, 0x19E6),
    make_parsed_command("DOWN", RemoteProtocolKind::Onkyo, 0xEAC7, 0x33CC),
    make_parsed_command("LEFT", RemoteProtocolKind::Onkyo, 0xEAC7, 0x1EE1),
    make_parsed_command("RIGHT", RemoteProtocolKind::Onkyo, 0xEAC7, 0x2DD2),
    make_parsed_command("EXIT", RemoteProtocolKind::Onkyo, 0xEAC7, 0x6699),
};

constexpr RemoteCommandDef kHisenseTvCommands[] = {
    make_parsed_command("POWER", RemoteProtocolKind::Nec, 0x0004, 0x0008),
    make_parsed_command("VOL+", RemoteProtocolKind::Nec, 0x0004, 0x0002),
    make_parsed_command("VOL-", RemoteProtocolKind::Nec, 0x0004, 0x0003),
    make_parsed_command("MUTE", RemoteProtocolKind::Nec, 0x0004, 0x0009),
    make_parsed_command("HOME", RemoteProtocolKind::Nec, 0x0004, 0x004A),
    make_parsed_command("MENU", RemoteProtocolKind::Nec, 0x0004, 0x0043),
    make_parsed_command("OK", RemoteProtocolKind::Nec, 0x0004, 0x005A),
    make_parsed_command("UP", RemoteProtocolKind::Nec, 0x0004, 0x0056),
    make_parsed_command("DOWN", RemoteProtocolKind::Nec, 0x0004, 0x0057),
    make_parsed_command("LEFT", RemoteProtocolKind::Nec, 0x0004, 0x0058),
    make_parsed_command("RIGHT", RemoteProtocolKind::Nec, 0x0004, 0x0059),
    make_parsed_command("BACK", RemoteProtocolKind::Nec, 0x0004, 0x0004),
    make_parsed_command("INPUT", RemoteProtocolKind::Nec, 0x0004, 0x000B),
};

constexpr RemoteCommandDef kHaierTvCommands[] = {
    make_parsed_command("POWER", RemoteProtocolKind::Nec, 0x0004, 0x0008),
    make_parsed_command("VOL+", RemoteProtocolKind::Nec, 0x0004, 0x0002),
    make_parsed_command("VOL-", RemoteProtocolKind::Nec, 0x0004, 0x0003),
    make_parsed_command("MUTE", RemoteProtocolKind::Nec, 0x0004, 0x0009),
    make_parsed_command("MENU", RemoteProtocolKind::Nec, 0x0004, 0x0043),
    make_parsed_command("OK", RemoteProtocolKind::Nec, 0x0004, 0x0044),
    make_parsed_command("INPUT", RemoteProtocolKind::Nec, 0x0004, 0x000B),
    make_parsed_command("EXIT", RemoteProtocolKind::Nec, 0x0004, 0x005B),
};

constexpr RemoteCommandDef kXiaomiTvCommands[] = {
    make_raw38_command("POWER", kXiaomiTvOpenRaw,
                       sizeof(kXiaomiTvOpenRaw) / sizeof(kXiaomiTvOpenRaw[0])),
    make_raw38_command("HOME", kXiaomiTvHomeRaw,
                       sizeof(kXiaomiTvHomeRaw) / sizeof(kXiaomiTvHomeRaw[0])),
    make_raw38_command("OK", kXiaomiTvOkRaw,
                       sizeof(kXiaomiTvOkRaw) / sizeof(kXiaomiTvOkRaw[0])),
    make_raw38_command("BACK", kXiaomiTvBackRaw,
                       sizeof(kXiaomiTvBackRaw) / sizeof(kXiaomiTvBackRaw[0])),
    make_raw38_command("VOL+", kXiaomiTvVolUpRaw,
                       sizeof(kXiaomiTvVolUpRaw) / sizeof(kXiaomiTvVolUpRaw[0])),
    make_raw38_command("VOL-", kXiaomiTvVolDnRaw,
                       sizeof(kXiaomiTvVolDnRaw) / sizeof(kXiaomiTvVolDnRaw[0])),
};

constexpr RemoteProfileDef kRemoteProfiles[] = {
    {"SAMSUNG", kSamsungTvCommands,
     sizeof(kSamsungTvCommands) / sizeof(kSamsungTvCommands[0])},
    {"SSG ALT", kSamsungAltCommands,
     sizeof(kSamsungAltCommands) / sizeof(kSamsungAltCommands[0])},
    {"SSG LEG", kSamsungMsbCommands,
     sizeof(kSamsungMsbCommands) / sizeof(kSamsungMsbCommands[0])},
    {"TCL", kTclTvCommands, sizeof(kTclTvCommands) / sizeof(kTclTvCommands[0])},
    {"HISENSE", kHisenseTvCommands,
     sizeof(kHisenseTvCommands) / sizeof(kHisenseTvCommands[0])},
    {"HAIER", kHaierTvCommands,
     sizeof(kHaierTvCommands) / sizeof(kHaierTvCommands[0])},
    {"XIAOMI", kXiaomiTvCommands,
     sizeof(kXiaomiTvCommands) / sizeof(kXiaomiTvCommands[0])},
};

constexpr size_t kRemoteProfileCount =
    sizeof(kRemoteProfiles) / sizeof(kRemoteProfiles[0]);

BLEScan* g_ble_scan = nullptr;
std::vector<WiFiRecord> g_wifi_results;
std::vector<BLERecord> g_ble_results;
std::vector<ScanSnapshot> g_history;
std::vector<SessionFileInfo> g_session_files;
HardwareSerial g_cat1_serial(1);
TinyGsm g_cat1_modem(g_cat1_serial);
TinyGsmClient g_cat1_tcp_client(g_cat1_modem);
HttpClient g_cat1_http(g_cat1_tcp_client, kCat1TestHost, kCat1TestPort);

AppScreen g_screen = AppScreen::Home;

int g_home_index = 0;
int g_heat_menu_index = 0;
int g_wifi_cursor = 0;
int g_ble_cursor = 0;
int g_history_cursor = 0;
int g_heat_files_cursor = 0;
int g_remote_profile_index = 0;
int g_remote_command_index = 0;

String g_status = "BOOT";
String g_last_scan_time = "--:--";
uint32_t g_last_scan_ms = 0;
int g_last_wifi_count = 0;
int g_last_ble_count = 0;
int g_last_wifi_total = 0;
int g_last_ble_total = 0;
int g_heat_score = 0;
int g_heat_score_raw = 0;
int g_heat_score_raw_uncapped = 0;
int g_heat_score_prev = 0;
String g_top_wifi_name = "-";
String g_top_ble_name = "-";
int g_top_wifi_rssi = -127;
int g_top_ble_rssi = -127;
bool g_sd_ready = false;
bool g_cloud_ready = false;
uint32_t g_cloud_last_connect_attempt_ms = 0;
uint32_t g_cloud_last_upload_ms = 0;
int g_cloud_sent = 0;
int g_cloud_fail = 0;
CloudState g_cloud_state = CloudState::Off;
WiFiClient g_cloud_wifi_client;
PubSubClient g_cloud_mqtt(g_cloud_wifi_client);
String g_cloud_device_token = "";
String g_cloud_mqtt_client_id = "";
String g_cloud_topic_up = "";
String g_cloud_topic_down = "";
String g_cloud_reply = "";
int g_cloud_reply_cmd = -1;
int g_cloud_reply_code = -1;
bool g_cloud_reply_ready = false;
bool g_session_active = false;
String g_session_name = "-";
String g_session_path = "";
String g_session_start_stamp = "-";
int g_session_row_count = 0;
int g_session_peak_heat = 0;
int g_session_heat_sum = 0;
SessionSummary g_selected_session_summary;
String g_remote_status = "READY";
String g_remote_last = "-";
String g_cloud_token_hint = "-";
String g_cloud_error_detail = "-";
Preferences g_prefs;
uint32_t g_session_sequence = 0;
String g_cat1_at_status = "WAIT";
String g_cat1_sim_status = "WAIT";
String g_cat1_net_status = "WAIT";
String g_cat1_csq_status = "WAIT";
String g_cat1_pdp_status = "WAIT";
String g_cat1_last_reply = "-";
String g_cat1_last_error = "-";
uint32_t g_cat1_last_probe_ms = 0;
bool g_cat1_probe_running = false;
String g_cat1_http_status = "IDLE";
int g_cat1_http_code = 0;
uint32_t g_cat1_last_upload_attempt_ms = 0;
uint32_t g_cat1_last_upload_success_ms = 0;
int g_cat1_last_uploaded_heat = -1;

bool cloud_feature_enabled() {
  return HEAT_CLOUD_ENABLE && strlen(HEAT_CLOUD_WIFI_SSID) > 0 &&
         strlen(HEAT_CLOUD_EZDATA_TOKEN) > 0;
}

const char* cloud_state_name(CloudState state) {
  switch (state) {
    case CloudState::Off:
      return "OFF";
    case CloudState::Idle:
      return "IDLE";
    case CloudState::Connecting:
      return "JOIN";
    case CloudState::Ready:
      return "READY";
    case CloudState::Uploading:
      return "SEND";
    case CloudState::Ok:
      return "OK";
    case CloudState::Error:
      return "ERR";
  }
  return "OFF";
}

String cloud_status_label() {
  if (g_cloud_state == CloudState::Error &&
      !g_cloud_error_detail.isEmpty() && g_cloud_error_detail != "-") {
    return g_cloud_error_detail;
  }
  return String(cloud_state_name(g_cloud_state));
}

const char* auth_name(wifi_auth_mode_t auth) {
  switch (auth) {
    case WIFI_AUTH_OPEN:
      return "OPEN";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA+2";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2-E";
    case WIFI_AUTH_WPA3_PSK:
      return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:
      return "WPA2+3";
    case WIFI_AUTH_WAPI_PSK:
      return "WAPI";
    default:
      return "OTHER";
  }
}

const char* home_name(int index) {
  switch (index) {
    case 0:
      return "HEAT";
    case 1:
      return "REMOTE";
    case 2:
      return "CAT1";
    default:
      return "UNK";
  }
}

constexpr const char* kHeatMenuItems[] = {
    "SUMMARY", "WIFI", "BLE", "LOG", "SESSION", "FILES",
};

constexpr size_t kHeatMenuItemCount =
    sizeof(kHeatMenuItems) / sizeof(kHeatMenuItems[0]);

const char* heat_menu_name(int index) {
  if (index < 0 || index >= static_cast<int>(kHeatMenuItemCount)) return "UNK";
  return kHeatMenuItems[index];
}

String screen_title(AppScreen screen) {
  switch (screen) {
    case AppScreen::Home:
      return "HOME";
    case AppScreen::HeatMenu:
    case AppScreen::HeatSummary:
    case AppScreen::HeatWiFi:
    case AppScreen::HeatBle:
    case AppScreen::HeatLog:
    case AppScreen::HeatSession:
    case AppScreen::HeatFiles:
    case AppScreen::HeatFileSummary:
      return "HEAT";
    case AppScreen::Remote:
      return "REMOTE";
    case AppScreen::Codex:
      return "CAT1";
  }
  return "UNK";
}

String screen_subtitle(AppScreen screen) {
  switch (screen) {
    case AppScreen::Home:
      return "MAIN MENU";
    case AppScreen::HeatMenu:
      return "HEAT MENU";
    case AppScreen::HeatSummary:
      return "SUMMARY";
    case AppScreen::HeatWiFi:
      return "WIFI LIST";
    case AppScreen::HeatBle:
      return "BLE LIST";
    case AppScreen::HeatLog:
      return "SCAN LOG";
    case AppScreen::HeatSession:
      return "SESSION";
    case AppScreen::HeatFiles:
      return "SESSIONS";
    case AppScreen::HeatFileSummary:
      return "SESSION INFO";
    case AppScreen::Remote:
      return "TV TEST PACK";
    case AppScreen::Codex:
      return "4G DEBUG";
  }
  return "UNK";
}

bool is_heat_screen(AppScreen screen) {
  return screen == AppScreen::HeatMenu || screen == AppScreen::HeatSummary ||
         screen == AppScreen::HeatWiFi || screen == AppScreen::HeatBle ||
         screen == AppScreen::HeatLog || screen == AppScreen::HeatSession ||
         screen == AppScreen::HeatFiles || screen == AppScreen::HeatFileSummary;
}

bool is_heat_live_screen(AppScreen screen) {
  return screen == AppScreen::HeatMenu || screen == AppScreen::HeatSummary ||
         screen == AppScreen::HeatWiFi || screen == AppScreen::HeatBle ||
         screen == AppScreen::HeatLog || screen == AppScreen::HeatSession ||
         screen == AppScreen::HeatFiles;
}

bool is_heat_detail_screen(AppScreen screen) {
  return screen == AppScreen::HeatSummary || screen == AppScreen::HeatWiFi ||
         screen == AppScreen::HeatBle || screen == AppScreen::HeatLog ||
         screen == AppScreen::HeatSession || screen == AppScreen::HeatFiles;
}

size_t utf8_char_size(uint8_t lead) {
  if ((lead & 0x80) == 0) return 1;
  if ((lead & 0xE0) == 0xC0) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  if ((lead & 0xF8) == 0xF0) return 4;
  return 1;
}

String trim_text(const String& input, size_t max_len) {
  const char* raw = input.c_str();
  const size_t bytes = input.length();
  size_t cursor = 0;
  size_t chars = 0;
  while (cursor < bytes && chars < max_len) {
    const size_t char_size = utf8_char_size(static_cast<uint8_t>(raw[cursor]));
    if (cursor + char_size > bytes) break;
    cursor += char_size;
    ++chars;
  }
  if (cursor >= bytes) return input;
  return input.substring(0, cursor) + "~";
}

void cat1_reset_status(const char* reason) {
  g_cat1_at_status = "WAIT";
  g_cat1_sim_status = "WAIT";
  g_cat1_net_status = "WAIT";
  g_cat1_csq_status = "WAIT";
  g_cat1_pdp_status = "WAIT";
  g_cat1_last_reply = "-";
  g_cat1_last_error = reason;
}

String cat1_device_id() {
  uint64_t mac = ESP.getEfuseMac();
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "adv-%04X%08lX",
           static_cast<unsigned int>((mac >> 32) & 0xFFFF),
           static_cast<unsigned long>(mac & 0xFFFFFFFF));
  return String(buffer);
}

void cat1_flush_input() {
  while (g_cat1_serial.available()) {
    g_cat1_serial.read();
  }
}

String cat1_send_command(const char* command, uint32_t timeout_ms) {
  cat1_flush_input();
  g_cat1_serial.println(command);

  String response;
  response.reserve(256);
  const uint32_t deadline = millis() + timeout_ms;
  while (millis() < deadline) {
    while (g_cat1_serial.available()) {
      const char ch = static_cast<char>(g_cat1_serial.read());
      response += ch;
      if (response.indexOf("\r\nOK\r\n") >= 0 || response.endsWith("OK\r\n") ||
          response.indexOf("\r\nERROR\r\n") >= 0 ||
          response.endsWith("ERROR\r\n")) {
        return response;
      }
    }
    delay(5);
  }
  return response;
}

bool cat1_response_ok(const String& response) {
  return response.indexOf("\r\nOK\r\n") >= 0 || response.endsWith("OK\r\n");
}

int cat1_parse_numeric_after(const String& response, const char* key) {
  const int key_pos = response.indexOf(key);
  if (key_pos < 0) return -1;
  int cursor = key_pos + strlen(key);
  while (cursor < response.length() &&
         (response[cursor] == ' ' || response[cursor] == '\t')) {
    ++cursor;
  }
  int value = 0;
  bool seen_digit = false;
  while (cursor < response.length() &&
         isdigit(static_cast<unsigned char>(response[cursor]))) {
    seen_digit = true;
    value = value * 10 + (response[cursor] - '0');
    ++cursor;
  }
  return seen_digit ? value : -1;
}

int cat1_parse_cereg_status(const String& response) {
  const int key_pos = response.indexOf("+CEREG:");
  if (key_pos < 0) return -1;
  int comma_pos = response.indexOf(',', key_pos);
  if (comma_pos < 0) return -1;
  ++comma_pos;
  while (comma_pos < response.length() &&
         (response[comma_pos] == ' ' || response[comma_pos] == '\t')) {
    ++comma_pos;
  }
  if (comma_pos >= response.length() ||
      !isdigit(static_cast<unsigned char>(response[comma_pos]))) {
    return -1;
  }
  return response[comma_pos] - '0';
}

String cat1_parse_cgpaddr(const String& response) {
  const int key_pos = response.indexOf("+CGPADDR:");
  if (key_pos < 0) return "";
  const int comma_pos = response.indexOf(',', key_pos);
  if (comma_pos < 0) return "";

  const int quote_start = response.indexOf('"', comma_pos);
  if (quote_start >= 0) {
    const int quote_end = response.indexOf('"', quote_start + 1);
    if (quote_end > quote_start) {
      return response.substring(quote_start + 1, quote_end);
    }
  }

  int start = comma_pos + 1;
  while (start < response.length() &&
         (response[start] == ' ' || response[start] == '\t')) {
    ++start;
  }
  int end = start;
  while (end < response.length() && response[end] != '\r' && response[end] != '\n') {
    ++end;
  }
  return response.substring(start, end);
}

void probe_cat1_status() {
  g_cat1_probe_running = true;
  g_cat1_last_probe_ms = millis();
  cat1_reset_status("-");

  String response = cat1_send_command("AT", kCat1CommandTimeoutMs);
  g_cat1_last_reply = trim_text(response, 24);
  if (!cat1_response_ok(response)) {
    g_cat1_at_status = "NO RESP";
    g_cat1_last_error = "AT";
    g_cat1_probe_running = false;
    return;
  }
  g_cat1_at_status = "OK";

  response = cat1_send_command("AT+CPIN?", kCat1CommandTimeoutMs);
  g_cat1_last_reply = trim_text(response, 24);
  if (response.indexOf("READY") >= 0) {
    g_cat1_sim_status = "READY";
  } else if (response.indexOf("SIM PIN") >= 0) {
    g_cat1_sim_status = "PIN";
  } else {
    g_cat1_sim_status = cat1_response_ok(response) ? "CHECK" : "FAIL";
    if (!cat1_response_ok(response)) g_cat1_last_error = "SIM";
  }

  response = cat1_send_command("AT+CSQ", kCat1CommandTimeoutMs);
  g_cat1_last_reply = trim_text(response, 24);
  const int csq = cat1_parse_numeric_after(response, "+CSQ:");
  if (csq >= 0 && csq <= 31) {
    const int dbm = -113 + csq * 2;
    g_cat1_csq_status = String(csq) + "/" + String(dbm);
  } else if (csq == 99) {
    g_cat1_csq_status = "99/UNK";
  } else {
    g_cat1_csq_status = cat1_response_ok(response) ? "UNK" : "FAIL";
    if (!cat1_response_ok(response)) g_cat1_last_error = "CSQ";
  }

  response = cat1_send_command("AT+CEREG?", kCat1CommandTimeoutMs);
  g_cat1_last_reply = trim_text(response, 24);
  const int cereg = cat1_parse_cereg_status(response);
  if (cereg == 1 || cereg == 5) {
    g_cat1_net_status = "REG";
  } else if (cereg >= 0) {
    g_cat1_net_status = "S" + String(cereg);
  } else {
    g_cat1_net_status = cat1_response_ok(response) ? "UNK" : "FAIL";
    if (!cat1_response_ok(response)) g_cat1_last_error = "NET";
  }

  response = cat1_send_command("AT+CGDCONT=1,\"IP\",\"cmnet\"", kCat1CommandTimeoutMs);
  g_cat1_last_reply = trim_text(response, 24);
  if (!cat1_response_ok(response)) {
    g_cat1_pdp_status = "APN ERR";
    g_cat1_last_error = "APN";
    g_cat1_probe_running = false;
    return;
  }

  response = cat1_send_command("AT+CGATT?", kCat1CommandTimeoutMs);
  g_cat1_last_reply = trim_text(response, 24);
  const int cgatt = cat1_parse_numeric_after(response, "+CGATT:");
  if (cgatt == 1) {
    g_cat1_pdp_status = "ATT";
    response = cat1_send_command("AT+CGPADDR=1", kCat1CommandTimeoutMs);
    g_cat1_last_reply = trim_text(response, 24);
    const String ip = cat1_parse_cgpaddr(response);
    if (!ip.isEmpty()) {
      g_cat1_pdp_status = trim_text(ip, 14);
    }
  } else if (cgatt == 0) {
    g_cat1_pdp_status = "DETACH";
  } else {
    g_cat1_pdp_status = cat1_response_ok(response) ? "UNK" : "FAIL";
    if (!cat1_response_ok(response)) g_cat1_last_error = "PDP";
  }

  if (g_cat1_last_error == "-") {
    g_cat1_last_error = "OK";
  }
  g_cat1_probe_running = false;
}

bool cat1_http_post_test() {
  g_cat1_http_status = "SEND";
  g_cat1_http_code = 0;
  g_cat1_last_error = "-";
  g_cat1_last_reply = "-";

  if (!g_cat1_modem.init()) {
    g_cat1_http_status = "MODEM";
    g_cat1_last_error = "MODEM";
    return false;
  }

  if (!g_cat1_modem.waitForNetwork()) {
    g_cat1_http_status = "NET TO";
    g_cat1_last_error = "NET";
    return false;
  }

  if (!g_cat1_modem.gprsConnect(kCat1Apn)) {
    g_cat1_http_status = "PDP ERR";
    g_cat1_last_error = "PDP";
    return false;
  }

  DynamicJsonDocument doc(512);
  doc["device_id"] = cat1_device_id();
  doc["session_name"] = g_session_name;
  doc["session_start"] = g_session_start_stamp;
  doc["time"] = g_last_scan_time;
  doc["heat"] = g_heat_score;
  doc["raw"] = g_heat_score_raw;
  doc["raw_uncapped"] = g_heat_score_raw_uncapped;
  doc["level"] = heat_level_name(g_heat_score);
  doc["trend"] = heat_trend_name(g_heat_score_prev, g_heat_score);
  doc["wifi_kept"] = g_last_wifi_count;
  doc["wifi_total"] = g_last_wifi_total;
  doc["ble_kept"] = g_last_ble_count;
  doc["ble_total"] = g_last_ble_total;

  String payload;
  serializeJson(doc, payload);

  g_cat1_http.connectionKeepAlive();
  g_cat1_http.beginRequest();
  g_cat1_http.post(kCat1TestPath);
  g_cat1_http.sendHeader("Content-Type", "application/json");
  g_cat1_http.sendHeader("Content-Length", payload.length());
  g_cat1_http.beginBody();
  g_cat1_http.print(payload);
  g_cat1_http.endRequest();

  const int status = g_cat1_http.responseStatusCode();
  g_cat1_http_code = status;
  String body = g_cat1_http.responseBody();
  g_cat1_http.stop();

  g_cat1_last_reply = trim_text(body, 18);
  if (status == 200 && body.indexOf("\"ok\": true") >= 0) {
    g_cat1_http_status = "OK";
    return true;
  }

  g_cat1_http_status = "HTTP " + String(status);
  g_cat1_last_error = "HTTP";
  return false;
}

void note_cat1_upload_result(bool success, uint32_t attempt_ms) {
  g_cat1_last_upload_attempt_ms = attempt_ms;
  if (!success) return;

  g_cat1_last_upload_success_ms = attempt_ms;
  g_cat1_last_uploaded_heat = g_heat_score;
}

bool trigger_cat1_heat_upload() {
  const uint32_t attempt_ms = millis();
  const bool success = cat1_http_post_test();
  note_cat1_upload_result(success, attempt_ms);
  return success;
}

bool should_auto_upload_cat1_heat() {
  const uint32_t now = millis();

  if (g_cat1_last_upload_success_ms == 0) {
    return now >= kCat1AutoUploadIntervalMs;
  }

  if (g_cat1_last_uploaded_heat >= 0) {
    int delta = g_heat_score - g_cat1_last_uploaded_heat;
    if (delta < 0) delta = -delta;
    if (delta > kCat1ImmediateUploadHeatDelta) {
      return true;
    }
  }

  return now - g_cat1_last_upload_success_ms >= kCat1AutoUploadIntervalMs;
}

void maybe_auto_upload_cat1_heat() {
  if (!should_auto_upload_cat1_heat()) return;
  trigger_cat1_heat_upload();
}

String format_time_label() {
  const uint32_t secs = millis() / 1000;
  const uint32_t minutes = secs / 60;
  const uint32_t remain = secs % 60;
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%02lu:%02lu",
           static_cast<unsigned long>(minutes),
           static_cast<unsigned long>(remain));
  return String(buffer);
}

String format_minutes_label(uint32_t ms) {
  const uint32_t total_minutes = ms / 60000;
  const uint32_t total_seconds = (ms / 1000) % 60;
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%lum%02lus",
           static_cast<unsigned long>(total_minutes),
           static_cast<unsigned long>(total_seconds));
  return String(buffer);
}

int clamp_int(int value, int low, int high) {
  return std::max(low, std::min(value, high));
}

int compute_heat_score_raw_uncapped(int wifi_count, int ble_count) {
  return wifi_count * 4 + ble_count * 3;
}

int compute_heat_score(int raw_uncapped) {
  return clamp_int(raw_uncapped, 0, 99);
}

int smooth_heat_score(int current, int next_value) {
  if (current <= 0) return next_value;
  return clamp_int((current * 3 + next_value * 2 + 2) / 5, 0, 99);
}

const char* heat_level_name(int score) {
  if (score >= 60) return "BUSY";
  if (score >= 35) return "HIGH";
  if (score >= 18) return "MID";
  return "LOW";
}

const char* heat_level_short_name(int score) {
  if (score >= 60) return "BZ";
  if (score >= 35) return "HI";
  if (score >= 18) return "MD";
  return "LO";
}

const char* heat_trend_name(int previous, int current) {
  const int delta = current - previous;
  if (delta >= 3) return "UP";
  if (delta <= -3) return "DOWN";
  return "HOLD";
}

const char* heat_trend_short_name(int previous, int current) {
  const int delta = current - previous;
  if (delta >= 3) return "UP";
  if (delta <= -3) return "DN";
  return "HD";
}

int heat_level_code(int score) {
  if (score >= 60) return 3;
  if (score >= 35) return 2;
  if (score >= 18) return 1;
  return 0;
}

int heat_trend_code(int previous, int current) {
  const int delta = current - previous;
  if (delta >= 3) return 1;
  if (delta <= -3) return -1;
  return 0;
}

String cloud_topic_name(const char* suffix) {
  return String(HEAT_CLOUD_TOPIC_PREFIX) + "_" + suffix;
}

void cloud_mqtt_callback(char* topic, byte* payload, unsigned int length) {
  (void)topic;
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; ++i) {
    message += static_cast<char>(payload[i]);
  }

  g_cloud_reply = message;
  g_cloud_reply_ready = true;
  g_cloud_reply_cmd = -1;
  g_cloud_reply_code = -1;

  DynamicJsonDocument doc(1024);
  if (deserializeJson(doc, message) == DeserializationError::Ok) {
    if (doc.containsKey("cmd")) g_cloud_reply_cmd = doc["cmd"].as<int>();
    if (doc.containsKey("code")) g_cloud_reply_code = doc["code"].as<int>();
  }
}

bool ensure_cloud_wifi();

String cloud_mac_compact() {
  uint64_t mac = ESP.getEfuseMac();
  char buffer[13];
  snprintf(buffer, sizeof(buffer), "%04X%08lX",
           static_cast<unsigned int>((mac >> 32) & 0xFFFF),
           static_cast<unsigned long>(mac & 0xFFFFFFFF));
  return String(buffer);
}

bool cloud_register_device_token(String& out_token) {
  if (!ensure_cloud_wifi()) return false;

  WiFiClientSecure secure_client;
  secure_client.setInsecure();
  HTTPClient http;
  if (!http.begin(secure_client,
                  "https://ezdata2.m5stack.com/api/v2/device/registerMac")) {
    g_cloud_error_detail = "REG";
    return false;
  }

  http.addHeader("Content-Type", "application/json");
  DynamicJsonDocument doc(256);
  doc["deviceType"] = HEAT_CLOUD_DEVICE_TYPE;
  doc["mac"] = cloud_mac_compact();

  String payload;
  serializeJson(doc, payload);
  const int http_code = http.POST(payload);
  if (http_code <= 0) {
    http.end();
    g_cloud_error_detail = "REG";
    return false;
  }

  DynamicJsonDocument resp(512);
  const DeserializationError err = deserializeJson(resp, http.getString());
  http.end();
  if (err) {
    g_cloud_error_detail = "REG";
    return false;
  }
  const int resp_code = resp["code"].as<int>();
  if (resp_code != 0 && resp_code != 200) {
    g_cloud_error_detail = "REG";
    return false;
  }

  out_token = resp["data"].as<String>();
  if (out_token.isEmpty()) {
    g_cloud_error_detail = "REG";
    return false;
  }
  g_cloud_error_detail = "-";
  return true;
}

void cloud_prepare_identity() {
  if (g_cloud_device_token.isEmpty()) {
    g_cloud_device_token = HEAT_CLOUD_EZDATA_TOKEN;
  }
  if (g_cloud_device_token.isEmpty()) {
    cloud_register_device_token(g_cloud_device_token);
  }
  if (g_cloud_device_token.isEmpty()) return;
  const String mac = cloud_mac_compact();
  g_cloud_mqtt_client_id = "ez" + mac + "ez";
  g_cloud_topic_up = "$ezdata/" + g_cloud_device_token + "/up";
  g_cloud_topic_down = "$ezdata/" + g_cloud_device_token + "/down";
  g_cloud_token_hint =
      g_cloud_device_token.length() >= 6
          ? g_cloud_device_token.substring(g_cloud_device_token.length() - 6)
          : g_cloud_device_token;
  g_cloud_error_detail = "-";
}

bool cloud_wait_for_reply(int expected_cmd) {
  const uint32_t deadline = millis() + kCloudReplyTimeoutMs;
  while (millis() < deadline) {
    g_cloud_mqtt.loop();
    if (g_cloud_reply_ready) {
      const int reply_cmd = g_cloud_reply_cmd;
      const int reply_code = g_cloud_reply_code;
      g_cloud_reply_ready = false;

      if (reply_cmd == expected_cmd && reply_code == 200) {
        return true;
      }
      if (reply_cmd == expected_cmd) {
        g_cloud_error_detail = "R" + String(reply_code);
        return false;
      }
      if (reply_cmd == 500) {
        g_cloud_error_detail = "R500";
        return false;
      }
      continue;
    }
    delay(10);
  }
  return false;
}

bool ensure_cloud_mqtt() {
  if (!cloud_feature_enabled()) {
    g_cloud_state = CloudState::Off;
    g_cloud_ready = false;
    g_cloud_error_detail = "-";
    return false;
  }
  if (!ensure_cloud_wifi()) return false;

  cloud_prepare_identity();
  if (g_cloud_device_token.isEmpty()) {
    g_cloud_state = CloudState::Error;
    g_cloud_ready = false;
    g_cloud_error_detail = "REG";
    return false;
  }
  g_cloud_mqtt.setServer(kCloudMqttHost, kCloudMqttPort);
  g_cloud_mqtt.setCallback(cloud_mqtt_callback);
  g_cloud_mqtt.setBufferSize(2048);

  if (g_cloud_mqtt.connected()) {
    g_cloud_state = CloudState::Ready;
    g_cloud_ready = true;
    g_cloud_error_detail = "-";
    return true;
  }

  g_cloud_state = CloudState::Connecting;
  const bool connected =
      g_cloud_mqtt.connect(g_cloud_mqtt_client_id.c_str(),
                           g_cloud_device_token.c_str(), nullptr);
  if (!connected) {
    g_cloud_state = CloudState::Error;
    g_cloud_ready = false;
    g_cloud_error_detail = "MQ" + String(g_cloud_mqtt.state());
    return false;
  }

  if (!g_cloud_mqtt.subscribe(g_cloud_topic_down.c_str())) {
    g_cloud_mqtt.disconnect();
    g_cloud_state = CloudState::Error;
    g_cloud_ready = false;
    g_cloud_error_detail = "SUB";
    return false;
  }

  g_cloud_state = CloudState::Ready;
  g_cloud_ready = true;
  g_cloud_error_detail = "-";
  return true;
}

bool cloud_send_field(const char* name, const String& value) {
  if (!ensure_cloud_mqtt()) return false;

  DynamicJsonDocument doc(256);
  doc["deviceToken"] = g_cloud_device_token;
  JsonObject body = doc.createNestedObject("body");
  body["name"] = name;
  body["value"] = value;
  body["requestType"] = 101;

  String payload;
  serializeJson(doc, payload);
  g_cloud_reply_ready = false;
  if (!g_cloud_mqtt.publish(g_cloud_topic_up.c_str(), payload.c_str())) {
    g_cloud_error_detail = "PUB";
    return false;
  }
  if (cloud_wait_for_reply(101)) {
    g_cloud_error_detail = "-";
    return true;
  }

  body["requestType"] = 100;
  payload = "";
  serializeJson(doc, payload);
  g_cloud_reply_ready = false;
  if (!g_cloud_mqtt.publish(g_cloud_topic_up.c_str(), payload.c_str())) {
    g_cloud_error_detail = "PUB";
    return false;
  }
  if (cloud_wait_for_reply(100)) {
    g_cloud_error_detail = "-";
    return true;
  }
  g_cloud_error_detail = "ACK";
  return false;
}

bool ensure_cloud_wifi() {
  if (!cloud_feature_enabled()) {
    g_cloud_state = CloudState::Off;
    g_cloud_ready = false;
    g_cloud_error_detail = "-";
    return false;
  }
  if (WiFi.status() == WL_CONNECTED) {
    g_cloud_state = CloudState::Ready;
    g_cloud_ready = true;
    g_cloud_error_detail = "-";
    return true;
  }
  const uint32_t now = millis();
  if (now - g_cloud_last_connect_attempt_ms < kCloudReconnectBackoffMs) {
    g_cloud_state = CloudState::Error;
    g_cloud_ready = false;
    g_cloud_error_detail = "WIFI";
    return false;
  }

  g_cloud_last_connect_attempt_ms = now;
  g_cloud_state = CloudState::Connecting;
  g_cloud_ready = false;
  WiFi.mode(WIFI_STA);
  WiFi.begin(HEAT_CLOUD_WIFI_SSID, HEAT_CLOUD_WIFI_PASSWORD);

  const uint32_t deadline = millis() + HEAT_CLOUD_WIFI_TIMEOUT_MS;
  while (WiFi.status() != WL_CONNECTED && millis() < deadline) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    g_cloud_state = CloudState::Ready;
    g_cloud_ready = true;
    g_cloud_error_detail = "-";
    return true;
  }

  g_cloud_state = CloudState::Error;
  g_cloud_ready = false;
  g_cloud_error_detail = "WIFI";
  return false;
}

bool upload_heat_summary_cloud() {
  if (!cloud_feature_enabled()) {
    g_cloud_state = CloudState::Off;
    return false;
  }
  if (!ensure_cloud_mqtt()) return false;

  g_cloud_state = CloudState::Uploading;
  const bool ok =
      cloud_send_field(cloud_topic_name("heat").c_str(), String(g_heat_score));

  g_cloud_last_upload_ms = millis();
  if (ok) {
    g_cloud_sent += 1;
    g_cloud_state = CloudState::Ok;
  } else {
    g_cloud_fail += 1;
    g_cloud_state = CloudState::Error;
  }
  return ok;
}

bool upload_session_summary_cloud(int rows, int avg_heat, int peak_heat) {
  (void)rows;
  (void)avg_heat;
  (void)peak_heat;
  if (!cloud_feature_enabled()) {
    g_cloud_state = CloudState::Off;
    return false;
  }
  return upload_heat_summary_cloud();
}

const char* remote_protocol_name(RemoteProtocolKind protocol) {
  switch (protocol) {
    case RemoteProtocolKind::Nec:
      return "NEC";
    case RemoteProtocolKind::Onkyo:
      return "NECX";
    case RemoteProtocolKind::Samsung32:
      return "SMSG";
    case RemoteProtocolKind::SamsungLG:
      return "SMLG";
    case RemoteProtocolKind::SamsungMSB32:
      return "SMSB";
    case RemoteProtocolKind::Raw38:
      return "RAW";
  }
  return "IR";
}

String remote_command_value(const RemoteCommandDef& command) {
  if (command.protocol == RemoteProtocolKind::Raw38) {
    return String(command.frequency_khz) + "K";
  }

  char buffer[20];
  if (command.protocol == RemoteProtocolKind::SamsungMSB32) {
    snprintf(buffer, sizeof(buffer), "%s %lX",
             remote_protocol_name(command.protocol),
             static_cast<unsigned long>(command.command));
  } else if (command.command <= 0xFF) {
    snprintf(buffer, sizeof(buffer), "%s %02X",
             remote_protocol_name(command.protocol), command.command);
  } else {
    snprintf(buffer, sizeof(buffer), "%s %04X",
             remote_protocol_name(command.protocol), command.command);
  }
  return String(buffer);
}

String next_session_name() {
  char buffer[40];
  snprintf(buffer, sizeof(buffer), "boot_%06lu",
           static_cast<unsigned long>(g_session_sequence));
  return String(buffer);
}

bool ensure_heat_log_dir();
bool ensure_total_heat_log_file();
bool begin_boot_session();
bool ensure_boot_session_ready();
bool prepare_session_sequence();

bool init_sd_card() {
  SPI.begin(kSdSpiSckPin, kSdSpiMisoPin, kSdSpiMosiPin, kSdSpiCsPin);
  g_sd_ready = SD.begin(kSdSpiCsPin, SPI, 25000000);
  if (!g_sd_ready) return false;
  return ensure_heat_log_dir();
}

bool ensure_heat_log_dir() {
  if (!g_sd_ready) return false;
  if (SD.exists(kHeatLogDir)) return true;
  return SD.mkdir(kHeatLogDir);
}

void refresh_session_files() {
  g_session_files.clear();
  if (!g_sd_ready && !init_sd_card()) {
    g_heat_files_cursor = 0;
    return;
  }
  if (!SD.exists(kHeatLogDir)) {
    g_heat_files_cursor = 0;
    return;
  }

  if (!SD.exists(kHeatLogAllPath)) {
    g_heat_files_cursor = 0;
    return;
  }

  g_session_files.push_back({String(kHeatLogAllFileName), String(kHeatLogAllPath)});
  if (g_session_files.empty()) {
    g_heat_files_cursor = 0;
  } else {
    g_heat_files_cursor = std::max(
        0, std::min(g_heat_files_cursor, static_cast<int>(g_session_files.size()) - 1));
  }
}

bool ensure_total_heat_log_file() {
  if (!g_sd_ready && !init_sd_card()) return false;
  if (!ensure_heat_log_dir()) return false;

  if (SD.exists(kHeatLogAllPath)) return true;

  File file = SD.open(kHeatLogAllPath, FILE_WRITE);
  if (!file) return false;
  file.println(
      "session_name,session_start,time,heat,raw,raw_uncapped,level,trend,wifi_kept,wifi_total,ble_kept,ble_total");
  file.close();
  return true;
}

bool prepare_session_sequence() {
  if (g_session_sequence > 0) return true;

  if (!g_prefs.begin("heatlog", false)) return false;
  const uint32_t next_value = g_prefs.getULong("boot_seq", 0) + 1;
  g_prefs.putULong("boot_seq", next_value);
  g_prefs.end();
  g_session_sequence = next_value;
  return true;
}

bool begin_boot_session() {
  if (!ensure_total_heat_log_file()) return false;
  if (!prepare_session_sequence()) return false;

  g_session_name = next_session_name();
  g_session_path = String(kHeatLogAllPath);
  g_session_start_stamp = format_time_label();
  g_session_row_count = 0;
  g_session_peak_heat = 0;
  g_session_heat_sum = 0;
  g_session_active = true;
  refresh_session_files();
  return true;
}

bool ensure_boot_session_ready() {
  if (!g_sd_ready && !init_sd_card()) return false;
  if (g_session_active && g_session_path == String(kHeatLogAllPath)) return true;
  return begin_boot_session();
}

void append_session_row() {
  if (!ensure_boot_session_ready()) return;
  File file = SD.open(g_session_path, FILE_APPEND);
  if (!file) return;
  file.printf("%s,%s,%s,%d,%d,%d,%s,%s,%d,%d,%d,%d\n",
              g_session_name.c_str(), g_session_start_stamp.c_str(),
              g_last_scan_time.c_str(), g_heat_score, g_heat_score_raw,
              g_heat_score_raw_uncapped,
              heat_level_name(g_heat_score),
              heat_trend_name(g_heat_score_prev, g_heat_score), g_last_wifi_count,
              g_last_wifi_total, g_last_ble_count, g_last_ble_total);
  file.close();

  g_session_row_count += 1;
  g_session_heat_sum += g_heat_score;
  g_session_peak_heat = std::max(g_session_peak_heat, g_heat_score);
}

bool load_session_summary(const String& path, SessionSummary& out) {
  out = SessionSummary();
  if (!g_sd_ready && !init_sd_card()) return false;
  File file = SD.open(path);
  if (!file) return false;

  int heat_sum = 0;
  String first_session = "-";
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) continue;
    if (line.startsWith("session_name,")) {
      continue;
    }
    int offsets[11];
    int found = 0;
    int search_from = 0;
    while (found < 11) {
      const int comma = line.indexOf(',', search_from);
      if (comma < 0) break;
      offsets[found++] = comma;
      search_from = comma + 1;
    }
    if (found < 11) continue;

    const String session_name = line.substring(0, offsets[0]);
    const String session_start = line.substring(offsets[0] + 1, offsets[1]);
    const String time = line.substring(offsets[1] + 1, offsets[2]);
    const int heat = line.substring(offsets[2] + 1, offsets[3]).toInt();

    if (first_session == "-") {
      first_session = session_name;
    }
    if (out.start_stamp == "-") {
      out.start_stamp = session_start;
    }
    out.last_stamp = time;
    out.rows += 1;
    heat_sum += heat;
    out.peak_heat = std::max(out.peak_heat, heat);
  }
  file.close();

  out.name = String(kHeatLogAllFileName);
  if (out.rows > 0) {
    out.avg_heat = heat_sum / out.rows;
    if (first_session != "-") {
      out.start_stamp = trim_text(first_session + " " + out.start_stamp, 18);
    }
    out.valid = true;
  }
  return out.valid;
}

const RemoteProfileDef& current_remote_profile() {
  return kRemoteProfiles[g_remote_profile_index];
}

const RemoteCommandDef& current_remote_command() {
  const auto& profile = current_remote_profile();
  return profile.commands[g_remote_command_index];
}

template <typename T>
void move_cursor(int delta, const std::vector<T>& items, int& cursor) {
  if (items.empty()) {
    cursor = 0;
    return;
  }
  const int last = static_cast<int>(items.size()) - 1;
  cursor = std::max(0, std::min(cursor + delta, last));
}

void move_history_cursor(int delta) {
  if (g_history.empty()) {
    g_history_cursor = 0;
    return;
  }
  const int last = static_cast<int>(g_history.size()) - 1;
  g_history_cursor = std::max(0, std::min(g_history_cursor + delta, last));
}

void move_session_file_cursor(int delta) {
  if (g_session_files.empty()) {
    g_heat_files_cursor = 0;
    return;
  }
  const int last = static_cast<int>(g_session_files.size()) - 1;
  g_heat_files_cursor = std::max(0, std::min(g_heat_files_cursor + delta, last));
}

void cycle_heat_detail(int delta) {
  constexpr AppScreen screens[] = {
      AppScreen::HeatSummary,
      AppScreen::HeatWiFi,
      AppScreen::HeatBle,
      AppScreen::HeatLog,
      AppScreen::HeatSession,
      AppScreen::HeatFiles,
  };
  int index = 0;
  for (int i = 0; i < 6; ++i) {
    if (screens[i] == g_screen) {
      index = i;
      break;
    }
  }
  index = (index + 6 + delta) % 6;
  g_screen = screens[index];
}

int ui_width() { return M5Cardputer.Display.width(); }

void use_ui_font(const lgfx::IFont* font = &fonts::Font2, float size = 1.0f) {
  M5Cardputer.Display.setFont(font);
  M5Cardputer.Display.setTextSize(size);
  M5Cardputer.Display.setTextDatum(textdatum_t::top_left);
}

void draw_text(int x, int y, const String& text, uint16_t fg,
               uint16_t bg = TFT_BLACK,
               const lgfx::IFont* font = &fonts::Font2,
               float size = 1.0f) {
  use_ui_font(font, size);
  M5Cardputer.Display.setTextColor(fg, bg);
  M5Cardputer.Display.drawString(text, x, y);
}

void draw_right_text(int x, int y, const String& text, uint16_t fg,
                     uint16_t bg = TFT_BLACK,
                     const lgfx::IFont* font = &fonts::Font2,
                     float size = 1.0f) {
  use_ui_font(font, size);
  M5Cardputer.Display.setTextColor(fg, bg);
  M5Cardputer.Display.drawRightString(text, x, y);
}

void draw_card(int x, int y, int w, int h, uint16_t fill) {
  M5Cardputer.Display.fillRoundRect(x, y, w, h, 8, fill);
  M5Cardputer.Display.drawRoundRect(x, y, w, h, 8, TFT_DARKGREY);
}

void draw_header(AppScreen screen) {
  const int w = ui_width();
  draw_text(4, 2, "ADV", TFT_CYAN, TFT_BLACK, &fonts::Font2, 1.0f);
  draw_right_text(w - 4, 2, screen_title(screen), TFT_WHITE, TFT_BLACK,
                  &fonts::Font2, 1.0f);
  M5Cardputer.Display.drawFastHLine(0, kHeaderLineY, w, TFT_DARKGREY);
  draw_text(4, kSubtitleY, screen_subtitle(screen), TFT_LIGHTGREY, TFT_BLACK,
            &fonts::Font0, 1.0f);
}

void draw_status_line(const String& text, uint16_t color = TFT_YELLOW) {
  draw_text(4, kStatusY, text, color, TFT_BLACK, &fonts::Font0, 1.0f);
}

void draw_list_item(int y, const String& label, const String& value,
                    bool selected) {
  const int w = ui_width();
  const uint16_t bg = selected ? 0x041F : 0x18A3;
  const uint16_t fg = selected ? TFT_WHITE : TFT_GREENYELLOW;
  draw_card(4, y, w - 8, 18, bg);
  const String shown_label = selected ? "> " + label : label;
  draw_text(10, y + 3, shown_label, fg, bg, &fonts::Font2, 1.0f);
  draw_right_text(w - 10, y + 3, value,
                  selected ? TFT_WHITE : TFT_LIGHTGREY, bg, &fonts::Font2, 1.0f);
}

void draw_home() {
  draw_header(AppScreen::Home);
  draw_status_line(String(heat_level_name(g_heat_score)) + " H" +
                   String(g_heat_score) + " " + g_last_scan_time);
  draw_list_item(48, "HEAT", String(g_heat_score),
                 g_home_index == 0);
  draw_list_item(68, "REMOTE", "PACK", g_home_index == 1);
  draw_list_item(88, "CAT1", "GROVE", g_home_index == 2);
}

void draw_heat_menu() {
  draw_header(AppScreen::HeatMenu);
  draw_status_line(String(heat_level_name(g_heat_score)) + " H" +
                   String(g_heat_score) + " " + g_status);
  const int start = std::max(
      0, std::min(g_heat_menu_index,
                  static_cast<int>(kHeatMenuItemCount) -
                      static_cast<int>(kVisibleListRows)));
  int y = kListTopY;
  for (size_t row = 0; row < kVisibleListRows; ++row) {
    const int idx = start + static_cast<int>(row);
    if (idx >= static_cast<int>(kHeatMenuItemCount)) break;
    String value = "-";
    switch (idx) {
      case 0:
        value = String(g_heat_score);
        break;
      case 1:
        value = String(g_last_wifi_count);
        break;
      case 2:
        value = String(g_last_ble_count);
        break;
      case 3:
        value = String(g_history.size());
        break;
      case 4:
        value = g_session_active ? "REC" : (g_sd_ready ? "IDLE" : "NO SD");
        break;
      case 5:
        value = String(g_session_files.size());
        break;
    }
    draw_list_item(y, heat_menu_name(idx), value, idx == g_heat_menu_index);
    y += kRowHeight;
  }
}

void draw_heat_summary() {
  draw_header(AppScreen::HeatSummary);
  String status = "TOP " + String(g_top_wifi_rssi) + "/" + String(g_top_ble_rssi) +
                  " C " + cloud_status_label();
  if (cloud_feature_enabled() && g_cloud_token_hint != "-") {
    status += " " + g_cloud_token_hint;
  }
  draw_status_line(status);
  draw_list_item(48, "HEAT",
                 String(g_heat_score) + "/" + String(g_heat_score_raw), true);
  draw_list_item(68, "TREND",
                 String(heat_trend_name(g_heat_score_prev, g_heat_score)) + " " +
                     heat_level_name(g_heat_score),
                 false);
  draw_list_item(88, "WIFI", String(g_last_wifi_count) + "/" +
                                String(g_last_wifi_total), false);
  draw_list_item(108, "BLE", String(g_last_ble_count) + "/" +
                                 String(g_last_ble_total), false);
}

void draw_wifi_records() {
  draw_header(AppScreen::HeatWiFi);
  draw_status_line("NEAR " + String(g_last_wifi_count) + "/" +
                   String(g_last_wifi_total) + " >-82");

  if (g_wifi_results.empty()) {
    draw_list_item(48, "NO WIFI", "-", false);
    return;
  }

  const int start = std::max(
      0, std::min(g_wifi_cursor,
                  static_cast<int>(g_wifi_results.size()) -
                      static_cast<int>(kVisibleListRows)));
  int y = kListTopY;
  for (size_t row = 0; row < kVisibleListRows; ++row) {
    const int idx = start + static_cast<int>(row);
    if (idx >= static_cast<int>(g_wifi_results.size())) break;
    const auto& item = g_wifi_results[idx];
    draw_list_item(y, String(idx + 1) + " " + trim_text(item.ssid, 8),
                   String(item.rssi) + " " + String(item.channel),
                   idx == g_wifi_cursor);
    y += kRowHeight;
  }
}

void draw_ble_records() {
  draw_header(AppScreen::HeatBle);
  draw_status_line("NEAR " + String(g_last_ble_count) + "/" +
                   String(g_last_ble_total) + " >-88");

  if (g_ble_results.empty()) {
    draw_list_item(48, "NO BLE", "-", false);
    return;
  }

  const int start = std::max(
      0, std::min(g_ble_cursor,
                  static_cast<int>(g_ble_results.size()) -
                      static_cast<int>(kVisibleListRows)));
  int y = kListTopY;
  for (size_t row = 0; row < kVisibleListRows; ++row) {
    const int idx = start + static_cast<int>(row);
    if (idx >= static_cast<int>(g_ble_results.size())) break;
    const auto& item = g_ble_results[idx];
    draw_list_item(y, String(idx + 1) + " " + trim_text(item.name, 10),
                   String(item.rssi), idx == g_ble_cursor);
    y += kRowHeight;
  }
}

void draw_history() {
  draw_header(AppScreen::HeatLog);
  draw_status_line("1M 60 LOGS " + g_last_scan_time);

  if (g_history.empty()) {
    draw_list_item(48, "NO LOG", "-", false);
    return;
  }

  const int start = std::max(
      0, std::min(g_history_cursor,
                  static_cast<int>(g_history.size()) -
                      static_cast<int>(kVisibleListRows)));
  int y = kListTopY;
  for (size_t row = 0; row < kVisibleListRows; ++row) {
    const int idx = start + static_cast<int>(row);
    if (idx >= static_cast<int>(g_history.size())) break;
    const auto& item = g_history[idx];
    int previous_score = item.heat_score;
    if (idx + 1 < static_cast<int>(g_history.size())) {
      previous_score = g_history[idx + 1].heat_score;
    }
    const String label =
        item.stamp + " " + String(heat_level_short_name(item.heat_score));
    const String value =
        "H" + String(item.heat_score) + " " +
        String(heat_trend_short_name(previous_score, item.heat_score));
    draw_list_item(y, label, value, idx == g_history_cursor);
    y += kRowHeight;
  }
}

void draw_heat_session() {
  draw_header(AppScreen::HeatSession);
  String status = g_sd_ready ? (g_session_active ? "AUTO LOG" : "WAIT SD")
                             : "NO SD CARD";
  status += " C ";
  status += cloud_status_label();
  if (cloud_feature_enabled() && g_cloud_token_hint != "-") {
    status += " " + g_cloud_token_hint;
  }
  draw_status_line(status);
  draw_list_item(48, "SESSION", trim_text(g_session_name, 12), true);
  draw_list_item(68, "ROWS", String(g_session_row_count), false);
  const int avg_heat = g_session_row_count > 0 ? g_session_heat_sum / g_session_row_count : 0;
  draw_list_item(88, "AVG/PEAK", String(avg_heat) + "/" + String(g_session_peak_heat),
                 false);
  draw_list_item(108, "FILE", String(kHeatLogAllFileName), false);
}

void draw_heat_files() {
  draw_header(AppScreen::HeatFiles);
  draw_status_line("TOTAL CSV");
  if (g_session_files.empty()) {
    draw_list_item(48, "NO FILE", g_sd_ready ? "WAIT INIT" : "NO SD", false);
    return;
  }
  draw_list_item(48, trim_text(g_session_files[0].name, 16), "ALL DATA", true);
  draw_list_item(68, "MODE", "AUTO SESSION", false);
  draw_list_item(88, "ROWS", String(g_session_row_count), false);
  draw_list_item(108, "CLOUD", String(g_cloud_sent) + "/" + String(g_cloud_fail),
                 false);
}

void draw_heat_file_summary() {
  draw_header(AppScreen::HeatFileSummary);
  draw_status_line(g_selected_session_summary.valid ? "TOTAL SUMMARY"
                                                    : "NO SUMMARY");
  draw_list_item(48, "FILE", trim_text(g_selected_session_summary.name, 12), true);
  draw_list_item(68, "ROWS", String(g_selected_session_summary.rows), false);
  draw_list_item(88, "AVG/PEAK",
                 String(g_selected_session_summary.avg_heat) + "/" +
                     String(g_selected_session_summary.peak_heat),
                 false);
  draw_list_item(108, "FIRST/LAST",
                 trim_text(g_selected_session_summary.start_stamp + ">" +
                               g_selected_session_summary.last_stamp,
                           14),
                 false);
}

void draw_remote() {
  draw_header(AppScreen::Remote);
  const auto& profile = current_remote_profile();
  String status = String(g_remote_profile_index + 1) + "/" +
                  String(kRemoteProfileCount) + " " + profile.name + " ";
  if (g_remote_last != "-") {
    status += g_remote_last;
  } else {
    status += g_remote_status;
  }
  draw_status_line(status);

  const int start = std::max(
      0, std::min(g_remote_command_index,
                  static_cast<int>(profile.command_count) -
                      static_cast<int>(kVisibleListRows)));
  int y = kListTopY;
  for (size_t row = 0; row < kVisibleListRows; ++row) {
    const int idx = start + static_cast<int>(row);
    if (idx >= static_cast<int>(profile.command_count)) break;
    const auto& command = profile.commands[idx];
    draw_list_item(y, command.label, remote_command_value(command),
                   idx == g_remote_command_index);
    y += kRowHeight;
  }
}

void draw_cat1() {
  draw_header(AppScreen::Codex);
  String status = "CMCC " + String(kCat1Apn);
  if (g_cat1_probe_running) {
    status += " PROBE";
  } else if (g_cat1_http_status == "SEND") {
    status += " SEND";
  } else if (g_cat1_http_status == "OK") {
    status += " POST200";
  } else if (g_cat1_http_code > 0) {
    status += " " + String(g_cat1_http_code);
  } else if (g_cat1_last_error != "-") {
    status += " " + g_cat1_last_error;
  }
  draw_status_line(status);
  draw_list_item(48, "AT", g_cat1_at_status, true);
  draw_list_item(68, "SIM", g_cat1_sim_status, false);
  draw_list_item(88, "NET/CSQ",
                 trim_text(g_cat1_net_status + " " + g_cat1_csq_status, 14),
                 false);
  draw_list_item(108, "PDP/HTTP",
                 trim_text(g_cat1_pdp_status + " " + g_cat1_http_status, 14),
                 false);
}

void redraw() {
  M5Cardputer.Display.clear(TFT_BLACK);
  switch (g_screen) {
    case AppScreen::Home:
      draw_home();
      break;
    case AppScreen::HeatMenu:
      draw_heat_menu();
      break;
    case AppScreen::HeatSummary:
      draw_heat_summary();
      break;
    case AppScreen::HeatWiFi:
      draw_wifi_records();
      break;
    case AppScreen::HeatBle:
      draw_ble_records();
      break;
    case AppScreen::HeatLog:
      draw_history();
      break;
    case AppScreen::HeatSession:
      draw_heat_session();
      break;
    case AppScreen::HeatFiles:
      draw_heat_files();
      break;
    case AppScreen::HeatFileSummary:
      draw_heat_file_summary();
      break;
    case AppScreen::Remote:
      draw_remote();
      break;
    case AppScreen::Codex:
      draw_cat1();
      break;
  }
}

void send_remote_command() {
  const auto& command = current_remote_command();
  switch (command.protocol) {
    case RemoteProtocolKind::Nec:
      IrSender.sendNEC(command.address, command.command, command.repeats);
      break;
    case RemoteProtocolKind::Onkyo:
      IrSender.sendOnkyo(command.address, command.command, command.repeats);
      break;
    case RemoteProtocolKind::Samsung32:
      IrSender.sendSamsung(command.address, command.command, command.repeats);
      break;
    case RemoteProtocolKind::SamsungLG:
      IrSender.sendSamsungLG(command.address, command.command, command.repeats);
      break;
    case RemoteProtocolKind::SamsungMSB32:
      IrSender.sendSamsungMSB(static_cast<unsigned long>(command.command), 32);
      break;
    case RemoteProtocolKind::Raw38:
      IrSender.sendRaw(command.raw_data, command.raw_length,
                       command.frequency_khz);
      break;
  }
  g_remote_status = "SENT";
  g_remote_last = command.label;
  Serial.printf("IR send: profile=%s protocol=%s address=0x%04X command=0x%08lX len=%u rep=%u\n",
                current_remote_profile().name,
                remote_protocol_name(command.protocol), command.address,
                static_cast<unsigned long>(command.command), command.raw_length,
                command.repeats);
  redraw();
}

void scan_wifi() {
  g_wifi_results.clear();
  g_top_wifi_name = "-";
  g_top_wifi_rssi = -127;
  WiFi.scanDelete();

  const int found = WiFi.scanNetworks();
  g_last_wifi_total = found > 0 ? found : 0;

  for (int i = 0; i < found; ++i) {
    WiFiRecord record;
    record.ssid = WiFi.SSID(i);
    if (record.ssid.isEmpty()) record.ssid = "<hidden>";
    record.rssi = WiFi.RSSI(i);
    record.channel = WiFi.channel(i);
    record.auth = WiFi.encryptionType(i);
    if (record.rssi < kWiFiNearRssi) {
      continue;
    }
    g_wifi_results.push_back(record);
  }

  g_last_wifi_count = static_cast<int>(g_wifi_results.size());

  std::sort(g_wifi_results.begin(), g_wifi_results.end(),
            [](const WiFiRecord& a, const WiFiRecord& b) { return a.rssi > b.rssi; });

  if (!g_wifi_results.empty()) {
    g_top_wifi_name = g_wifi_results.front().ssid;
    g_top_wifi_rssi = g_wifi_results.front().rssi;
  }

  WiFi.scanDelete();
  if (!g_wifi_results.empty()) {
    g_wifi_cursor = std::max(
        0, std::min(g_wifi_cursor, static_cast<int>(g_wifi_results.size()) - 1));
  } else {
    g_wifi_cursor = 0;
  }
}

void scan_ble() {
  g_ble_results.clear();
  g_top_ble_name = "-";
  g_top_ble_rssi = -127;

  BLEScanResults found = g_ble_scan->start(kBleScanSeconds, false);
  g_last_ble_total = found.getCount();

  for (int i = 0; i < found.getCount(); ++i) {
    BLEAdvertisedDevice device = found.getDevice(i);

    BLERecord record;
    std::string name = device.getName();
    record.name = name.empty() ? String("<unnamed>") : String(name.c_str());
    record.address = String(device.getAddress().toString().c_str());
    record.rssi = device.getRSSI();
    if (record.rssi < kBleNearRssi) {
      continue;
    }
    g_ble_results.push_back(record);
  }

  g_last_ble_count = static_cast<int>(g_ble_results.size());

  std::sort(g_ble_results.begin(), g_ble_results.end(),
            [](const BLERecord& a, const BLERecord& b) { return a.rssi > b.rssi; });

  if (!g_ble_results.empty()) {
    g_top_ble_name = g_ble_results.front().name;
    g_top_ble_rssi = g_ble_results.front().rssi;
  }

  g_ble_scan->clearResults();
  if (!g_ble_results.empty()) {
    g_ble_cursor = std::max(
        0, std::min(g_ble_cursor, static_cast<int>(g_ble_results.size()) - 1));
  } else {
    g_ble_cursor = 0;
  }
}

void push_history_snapshot() {
  ScanSnapshot snapshot;
  snapshot.stamp = format_time_label();
  snapshot.heat_score = g_heat_score;
  snapshot.wifi_count = g_last_wifi_count;
  snapshot.ble_count = g_last_ble_count;

  g_history.insert(g_history.begin(), snapshot);
  if (g_history.size() > kMaxHistoryEntries) g_history.pop_back();
  g_history_cursor = 0;
}

void run_scan_cycle() {
  g_status = "WIFI";
  redraw();
  scan_wifi();

  g_status = "BLE";
  redraw();
  scan_ble();

  g_last_scan_ms = millis();
  g_last_scan_time = format_time_label();
  g_heat_score_raw_uncapped =
      compute_heat_score_raw_uncapped(g_last_wifi_count, g_last_ble_count);
  g_heat_score_raw = compute_heat_score(g_heat_score_raw_uncapped);
  g_heat_score_prev = g_heat_score;
  g_heat_score = smooth_heat_score(g_heat_score, g_heat_score_raw);
  g_status = "DONE";
  push_history_snapshot();
  append_session_row();
  upload_heat_summary_cloud();
  maybe_auto_upload_cat1_heat();
  redraw();

  Serial.printf("Scan complete: WiFi=%d/%d BLE=%d/%d Heat=%d Raw=%d RawU=%d Level=%s\n",
                g_last_wifi_count, g_last_wifi_total, g_last_ble_count,
                g_last_ble_total, g_heat_score, g_heat_score_raw,
                g_heat_score_raw_uncapped,
                heat_level_name(g_heat_score));
}

void open_selected_home() {
  switch (g_home_index) {
    case 0:
      g_screen = AppScreen::HeatMenu;
      break;
    case 1:
      g_remote_profile_index = 0;
      g_remote_command_index = 0;
      g_remote_status = "READY";
      g_remote_last = "-";
      g_screen = AppScreen::Remote;
      break;
    case 2:
      g_screen = AppScreen::Codex;
      probe_cat1_status();
      break;
  }
  redraw();
}

void open_selected_heat_menu() {
  switch (g_heat_menu_index) {
    case 0:
      g_screen = AppScreen::HeatSummary;
      break;
    case 1:
      g_screen = AppScreen::HeatWiFi;
      break;
    case 2:
      g_screen = AppScreen::HeatBle;
      break;
    case 3:
      g_screen = AppScreen::HeatLog;
      break;
    case 4:
      g_screen = AppScreen::HeatSession;
      break;
    case 5:
      refresh_session_files();
      g_screen = AppScreen::HeatFiles;
      break;
  }
  redraw();
}

void handle_home_navigation(char key) {
  switch (key) {
    case 'W':
    case 'A':
    case 'K':
    case 'H':
      g_home_index = (g_home_index + 2) % 3;
      redraw();
      break;
    case 'S':
    case 'D':
    case 'J':
    case 'L':
      g_home_index = (g_home_index + 1) % 3;
      redraw();
      break;
    default:
      break;
  }
}

void handle_heat_menu_navigation(char key) {
  switch (key) {
    case 'W':
    case 'A':
    case 'K':
    case 'H':
      g_heat_menu_index =
          (g_heat_menu_index + static_cast<int>(kHeatMenuItemCount) - 1) %
          static_cast<int>(kHeatMenuItemCount);
      redraw();
      break;
    case 'S':
    case 'D':
    case 'J':
    case 'L':
      g_heat_menu_index =
          (g_heat_menu_index + 1) % static_cast<int>(kHeatMenuItemCount);
      redraw();
      break;
    default:
      break;
  }
}

void handle_heat_detail_navigation(char key) {
  switch (key) {
    case 'A':
    case 'H':
      cycle_heat_detail(-1);
      redraw();
      break;
    case 'D':
    case 'L':
      cycle_heat_detail(1);
      redraw();
      break;
    case 'W':
    case 'K':
      if (g_screen == AppScreen::HeatWiFi) {
        move_cursor(-1, g_wifi_results, g_wifi_cursor);
      } else if (g_screen == AppScreen::HeatBle) {
        move_cursor(-1, g_ble_results, g_ble_cursor);
      } else if (g_screen == AppScreen::HeatLog) {
        move_history_cursor(-1);
      } else if (g_screen == AppScreen::HeatFiles) {
        move_session_file_cursor(-1);
      }
      redraw();
      break;
    case 'S':
    case 'J':
      if (g_screen == AppScreen::HeatWiFi) {
        move_cursor(1, g_wifi_results, g_wifi_cursor);
      } else if (g_screen == AppScreen::HeatBle) {
        move_cursor(1, g_ble_results, g_ble_cursor);
      } else if (g_screen == AppScreen::HeatLog) {
        move_history_cursor(1);
      } else if (g_screen == AppScreen::HeatFiles) {
        move_session_file_cursor(1);
      }
      redraw();
      break;
    default:
      break;
  }
}

void handle_remote_navigation(char key) {
  const auto& profile = current_remote_profile();
  switch (key) {
    case 'A':
    case 'H':
      g_remote_profile_index =
          (g_remote_profile_index + static_cast<int>(kRemoteProfileCount) - 1) %
          static_cast<int>(kRemoteProfileCount);
      g_remote_command_index = 0;
      g_remote_status = "READY";
      g_remote_last = "-";
      redraw();
      break;
    case 'D':
    case 'L':
      g_remote_profile_index =
          (g_remote_profile_index + 1) % static_cast<int>(kRemoteProfileCount);
      g_remote_command_index = 0;
      g_remote_status = "READY";
      g_remote_last = "-";
      redraw();
      break;
    case 'W':
    case 'K':
      g_remote_command_index =
          (g_remote_command_index + static_cast<int>(profile.command_count) - 1) %
          static_cast<int>(profile.command_count);
      redraw();
      break;
    case 'S':
    case 'J':
      g_remote_command_index =
          (g_remote_command_index + 1) % static_cast<int>(profile.command_count);
      redraw();
      break;
    default:
      break;
  }
}

char map_fn_navigation_key(char key) {
  switch (key) {
    case ';':
      return 'K';  // UP
    case '/':
      return 'L';  // RIGHT
    case '.':
      return 'J';  // DOWN
    case ',':
      return 'H';  // LEFT
    default:
      return 0;
  }
}

void handle_key(char key) {
  const char upper = static_cast<char>(toupper(static_cast<unsigned char>(key)));
  if (g_screen == AppScreen::Home) {
    handle_home_navigation(upper);
    return;
  }
  if (g_screen == AppScreen::HeatMenu) {
    handle_heat_menu_navigation(upper);
    return;
  }
  if (is_heat_detail_screen(g_screen)) {
    handle_heat_detail_navigation(upper);
    return;
  }
  if (g_screen == AppScreen::Remote) {
    handle_remote_navigation(upper);
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg, true);

  Serial.begin(115200);
  delay(200);

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(1);

  if (cloud_feature_enabled()) {
    g_cloud_state = CloudState::Idle;
  }

  IrSender.begin(DISABLE_LED_FEEDBACK);
  IrSender.setSendPin(kIrTxPin);

  if (init_sd_card()) {
    refresh_session_files();
    begin_boot_session();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);

  BLEDevice::init("");
  g_ble_scan = BLEDevice::getScan();
  g_ble_scan->setActiveScan(true);
  g_ble_scan->setInterval(100);
  g_ble_scan->setWindow(99);
  g_cat1_serial.begin(kCat1BaudRate, SERIAL_8N1, kCat1RxPin, kCat1TxPin);
  cat1_reset_status("WAIT");

  redraw();
  run_scan_cycle();
}

void loop() {
  M5Cardputer.update();

  if (g_cloud_mqtt.connected()) {
    g_cloud_mqtt.loop();
  }

  if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
    auto status = M5Cardputer.Keyboard.keysState();

    for (char key : status.word) {
      if (status.fn) {
        const char fn_nav = map_fn_navigation_key(key);
        if (fn_nav != 0) {
          handle_key(fn_nav);
          continue;
        }
      }
      handle_key(key);
    }

    if (status.enter) {
      if (g_screen == AppScreen::Home) {
        open_selected_home();
      } else if (g_screen == AppScreen::HeatMenu) {
        open_selected_heat_menu();
      } else if (g_screen == AppScreen::HeatSession) {
        g_status = ensure_boot_session_ready() ? "AUTO LOG" : "NO SD";
        redraw();
      } else if (g_screen == AppScreen::HeatFiles) {
        if (!g_session_files.empty() &&
            load_session_summary(g_session_files[0].path,
                                 g_selected_session_summary)) {
          g_screen = AppScreen::HeatFileSummary;
          redraw();
        }
      } else if (is_heat_detail_screen(g_screen)) {
        run_scan_cycle();
      } else if (g_screen == AppScreen::Remote) {
        send_remote_command();
      } else if (g_screen == AppScreen::Codex) {
        trigger_cat1_heat_upload();
        redraw();
      }
    }

    if (status.tab && g_screen == AppScreen::HeatFiles) {
      refresh_session_files();
      redraw();
    } else if (status.tab && is_heat_live_screen(g_screen)) {
      run_scan_cycle();
    } else if (status.tab && g_screen == AppScreen::Remote) {
      send_remote_command();
    } else if (status.tab && g_screen == AppScreen::Codex) {
      probe_cat1_status();
      redraw();
    }

    if (status.del) {
      if (g_screen == AppScreen::HeatFileSummary) {
        g_screen = AppScreen::HeatFiles;
        redraw();
      } else
      if (is_heat_detail_screen(g_screen)) {
        g_screen = AppScreen::HeatMenu;
        redraw();
      } else if (g_screen != AppScreen::Home) {
        g_screen = AppScreen::Home;
        redraw();
      }
    }
  }

  if (millis() - g_last_scan_ms >= kAutoScanIntervalMs) {
    run_scan_cycle();
  }

  delay(10);
}
