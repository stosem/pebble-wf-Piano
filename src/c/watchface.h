#pragma once

#include <pebble.h>

//#define DEBUG 1
//#define DEMO 1

// persistent storage keys
#define SETTINGS_KEY 1

#define OFF 0

#ifdef DEBUG
#define LOG(str...) app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__, str);
#else
#define LOG(str...)
#endif

#define t1(time) (time / 10)
#define t2(time) (time % 10)

///////////////////
// Clay settings //
///////////////////
typedef struct ClaySettings {
  bool VibrateOnBTLost;
  uint8_t BatteryWarning;
  uint16_t  VibrateInterval;
} ClaySettings; // Define our settings struct

//////////////////
// status flags
/////////////////
typedef struct Status {
    bool changed;
    bool is_charging;
    bool is_battery_discharged;
    bool is_bt_connected;
    bool is_quiet_time;
} Status;

static void config_clear();
static void config_load();
static void config_save();
static void status_clear();
static void update_proc_main(Layer *layer, GContext *ctx);
static void handler_tick(struct tm *tick_time, TimeUnits units_changed);
static void handler_battery(BatteryChargeState charge_state);
static void callback_bluetooth(bool connected);
static void inbox_received_callback(DictionaryIterator *iterator, void *context);
static void inbox_dropped_callback(AppMessageResult reason, void *context);
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context);
static void outbox_sent_callback(DictionaryIterator *iterator, void *context);
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void init();
static void deinit();
