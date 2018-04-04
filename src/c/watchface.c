// Piano 
// pebble watchface by stosem (github.com/stosem)
//
//

#include <pebble.h>
#include "watchface.h"
static Window *s_main_window, *s_sec_window;
static Layer *s_main_layer, *s_status_layer;
static BitmapLayer *s_bluetooth_bitmap_layer, *s_charging_bitmap_layer, *s_battery_bitmap_layer, *s_quiet_bitmap_layer;
static GBitmap *s_bluetooth_bitmap=NULL, *s_charging_bitmap=NULL, *s_battery_bitmap=NULL, *s_quiet_bitmap=NULL;
static ClaySettings settings; // An instance of the struct
static Status status;

// MAIN

///////////////////////////////
// set default Clay settings //
///////////////////////////////
static void config_clear() {
  settings.BatteryWarning = 30;
  settings.VibrateInterval = OFF;
  settings.VibrateOnBTLost = true;
};


/////////////////////////////////////
// load default settings from Clay //
/////////////////////////////////////
static void config_load() {
  // Read settings from persistent storage, if they exist
  persist_read_data( SETTINGS_KEY, &settings, sizeof(settings) );	
  LOG( "config loaded " );
};


/////////////////////////
// saves user settings //
/////////////////////////
static void config_save() {
  persist_write_data( SETTINGS_KEY, &settings, sizeof(settings) );
  LOG( "config_save" );
};


//////////////////////////////
// set defaults status
// //////////////////////////
static void status_clear() {
    status.changed=true;
    status.is_charging=false;
    status.is_battery_discharged=false;
    status.is_bt_connected=false;
    status.is_quiet_time=false;
};


// draw piano roll
static void draw_piano_roll( GContext *ctx, uint8_t y, uint8_t hl_num ) {
    // Draw Piano
	GRect rect_bounds;
    // draw big keys ( second digit )
    for ( uint8_t i = 0; i<9; i++ ) { 
        rect_bounds = GRect( PBL_IF_ROUND_ELSE(18, 1)+i*16, y, 14 , 58 );
	    graphics_fill_rect(ctx, rect_bounds, 2 , GCornersBottom );
        // colorize current key as second diget of highlight number
        if ( (1+i) == t2( hl_num ) ) {
            graphics_context_set_fill_color(ctx, GColorLightGray);
        } else {
            graphics_context_set_fill_color(ctx, GColorWhite);
        }
        graphics_fill_rect(ctx, rect_bounds, 2 , GCornersBottom );
    };
    // draw small keys ( first digit )
    for ( uint8_t i = 0; i<9; i++ ) { 
        if ( ( i == 2 ) || ( i == 6 ) ) continue ;
        rect_bounds = GRect( PBL_IF_ROUND_ELSE(28, 12)+i*16, y, 9 , 36 );
        graphics_context_set_fill_color(ctx, GColorBlack);
	    graphics_fill_rect(ctx, rect_bounds, 2 , GCornersBottom ); 
        // colorize current key ( skip 3'd key )
        if ( ( ( i<2 ) && ( t1( hl_num ) == (i+1) ) ) ||
             ( ( i>2 ) && ( t1( hl_num ) == i ) ) ) { 
            rect_bounds = GRect( PBL_IF_ROUND_ELSE(28, 12)+i*16+2, y, 5 , 34 );
            graphics_context_set_fill_color(ctx, GColorLightGray);
            graphics_fill_rect(ctx, rect_bounds, 1 , GCornersBottom );
        };
    };
};

/////////////////////////
// draw main window //
/////////////////////////
static void update_proc_main(Layer *layer, GContext *ctx) {
  LOG( "Redraw: main window" );
  time_t t = time( NULL );
  struct tm *tick = localtime( &t );
  LOG ( "Hour digits: %d and %d ", t1(tick->tm_hour), t2(tick->tm_hour) );
  draw_piano_roll( ctx, 28, tick->tm_hour ); 
  draw_piano_roll( ctx, 100, tick->tm_min ); 
  status.is_quiet_time = quiet_time_is_active();  
#ifdef DEMO
  status.is_quiet_time = true; 
#endif
  // update status icons
  if ( status.changed ) {
   layer_set_hidden( bitmap_layer_get_layer(s_bluetooth_bitmap_layer), status.is_bt_connected );
   layer_set_hidden( bitmap_layer_get_layer(s_charging_bitmap_layer), !status.is_charging );
   layer_set_hidden( bitmap_layer_get_layer(s_quiet_bitmap_layer), !status.is_quiet_time );
   layer_set_hidden( bitmap_layer_get_layer(s_battery_bitmap_layer), !status.is_battery_discharged );
  status.changed = false;
  };
};


//////////////////
// handle ticks //
//////////////////
static void handler_tick(struct tm *tick_time, TimeUnits units_changed) {
  LOG( "Run: handler tick" );
  // update quiet time status
  bool qt =  quiet_time_is_active();
  if ( status.is_quiet_time != qt ) {
      status.changed = true;
  }
  status.is_quiet_time = qt;
#ifdef DEMO
  status.is_quiet_time = true; 
#endif

 // vibrate 
  if (settings.VibrateInterval > 0) {
     if ( (tick_time->tm_min % settings.VibrateInterval == 0) || (
          (tick_time->tm_min == 0 ) && ( settings.VibrateInterval == 60) ) )  {
        if (!status.is_quiet_time) { vibes_short_pulse(); };
     };
  };

  // refresh status
  layer_mark_dirty( s_main_layer );
};


/////////////////////////////////////
// registers battery update events //
/////////////////////////////////////
static void handler_battery(BatteryChargeState charge_state) {
  LOG( "Run: handler battery. percent=%d, warning=%d, flag=%s", 
          charge_state.charge_percent, 
          settings.BatteryWarning,
          status.is_battery_discharged?"true":"false" );
  bool bd = ( charge_state.charge_percent <= settings.BatteryWarning );
  if ( status.is_battery_discharged != bd ) {
      status.is_battery_discharged = bd;
      status.changed = true;
  };
  bool ch=(charge_state.is_charging || charge_state.is_plugged );
  if ( status.is_charging != ch ) {
      status.is_charging = ch;
      status.changed = true;
      layer_mark_dirty( s_main_layer );
  };
#ifdef DEMO
  status.is_charging = true; 
  status.is_battery_discharged = true;
#endif
};


/////////////////////////////
// manage bluetooth status //
/////////////////////////////
static void callback_bluetooth(bool connected) {
  LOG( "Run: bluetooth_callback" );
#ifdef DEMO
  connected=false;
#endif
 // vibrate on disconnect not on quiet time 
  if(status.is_bt_connected && !connected && !status.is_quiet_time && settings.VibrateOnBTLost ) {
    vibes_double_pulse();
  }
 // check if status changed
  if(status.is_bt_connected != connected) {
      status.is_bt_connected=connected;
      status.changed = true;
      layer_mark_dirty( s_main_layer ); // force update on bluetooth
  }
  LOG( "Redraw: bluetooth, %d", status.is_bt_connected );
};


////////////////////////////
// Clay calls //
////////////////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  LOG( "inbox_received_callback" );
  // Read tuples for data
 
  // vibrate on bt lost
  Tuple *vibrate_on_bt_lost_t = dict_find(iterator, MESSAGE_KEY_KEY_VIBRATE_ON_BT_LOST);
  if(vibrate_on_bt_lost_t) { settings.VibrateOnBTLost = vibrate_on_bt_lost_t->value->int32 == 1; }

  // vibrate interval
  Tuple *vibrate_interval_t = dict_find(iterator, MESSAGE_KEY_KEY_VIBRATE_INTERVAL);
  if(vibrate_interval_t) { 
      settings.VibrateInterval = atoi(vibrate_interval_t->value->cstring);
  LOG( "set Vibrate interval=%d", settings.VibrateInterval );
  }

  // battery warinig level
  Tuple *battery_warning_t = dict_find(iterator, MESSAGE_KEY_KEY_BATTERY_WARNING);
  if(battery_warning_t) { settings.BatteryWarning = atoi(battery_warning_t->value->cstring); }
 
    status.changed = true;
    layer_mark_dirty( s_main_layer );

	config_save();
};


static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  LOG( "Message dropped!" );
};


static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  LOG( "Outbox send failed!" );
};


static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  LOG( "Outbox send success!" );
};


//////////////////////
// load main window //
//////////////////////
static void main_window_load(Window *window) {
  LOG( "load main window" );
  Layer *window_layer = window_get_root_layer( window );
  GRect bounds = layer_get_bounds( window_layer );
  
  // setup layers
  window_set_background_color( s_main_window, GColorBlack );

  // create main canvas layer 
  s_main_layer = layer_create( bounds );
  layer_set_update_proc( s_main_layer, update_proc_main );
  layer_add_child( window_layer, s_main_layer );  
 
  // create status icons
  // bluetooth
  s_bluetooth_bitmap=gbitmap_create_with_resource( RESOURCE_ID_ICON_BT );
  s_bluetooth_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(68, 0), PBL_IF_ROUND_ELSE(8, 4), 18, 18));
  bitmap_layer_set_compositing_mode( s_bluetooth_bitmap_layer, GCompOpSet );
  bitmap_layer_set_bitmap( s_bluetooth_bitmap_layer, s_bluetooth_bitmap ); 
  layer_add_child( s_main_layer, bitmap_layer_get_layer(s_bluetooth_bitmap_layer) );       
  // quiettime icon
  s_quiet_bitmap=gbitmap_create_with_resource( RESOURCE_ID_ICON_QUIET );
  s_quiet_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(80, 12), PBL_IF_ROUND_ELSE(4, 2), 18, 18));
  bitmap_layer_set_compositing_mode(s_quiet_bitmap_layer, GCompOpSet );
  bitmap_layer_set_bitmap( s_quiet_bitmap_layer, s_quiet_bitmap ); 
  layer_add_child(s_main_layer, bitmap_layer_get_layer(s_quiet_bitmap_layer));    
  // charging icon 
  s_charging_bitmap=gbitmap_create_with_resource( RESOURCE_ID_ICON_CHARGING );
  s_charging_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(80, 120), PBL_IF_ROUND_ELSE(160, 4), 18, 18));
  bitmap_layer_set_compositing_mode(s_charging_bitmap_layer, GCompOpSet );
  bitmap_layer_set_bitmap( s_charging_bitmap_layer, s_charging_bitmap ); 
  layer_add_child(s_main_layer, bitmap_layer_get_layer(s_charging_bitmap_layer));    
  // battery layer
  s_battery_bitmap=gbitmap_create_with_resource( RESOURCE_ID_ICON_BAT );
  s_battery_bitmap_layer = bitmap_layer_create(GRect(PBL_IF_ROUND_ELSE(66, 104), PBL_IF_ROUND_ELSE(160, 4), 18, 18));
  bitmap_layer_set_compositing_mode(s_battery_bitmap_layer, GCompOpSet );
  bitmap_layer_set_bitmap( s_battery_bitmap_layer, s_battery_bitmap ); 
  layer_add_child(s_main_layer, bitmap_layer_get_layer(s_battery_bitmap_layer));    
 
  status_clear();
  // detect quiet
  status.is_quiet_time = quiet_time_is_active(); 

  LOG( "main_window_load" );
};


///////////////////
// unload window //
///////////////////
static void main_window_unload(Window *window) {
  LOG( "unload window" );
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  // unload gbitmap
  if ( s_battery_bitmap ) gbitmap_destroy( s_battery_bitmap ); 
  if ( s_bluetooth_bitmap ) gbitmap_destroy( s_bluetooth_bitmap );
  if ( s_charging_bitmap ) gbitmap_destroy( s_charging_bitmap );
  if ( s_quiet_bitmap ) gbitmap_destroy( s_quiet_bitmap );
  // unload bitmap layer
  if( s_battery_bitmap_layer ) { bitmap_layer_destroy( s_battery_bitmap_layer ); s_battery_bitmap_layer=NULL; };
  if( s_bluetooth_bitmap_layer ) { bitmap_layer_destroy( s_bluetooth_bitmap_layer ); s_bluetooth_bitmap_layer=NULL; };
  if( s_charging_bitmap_layer ) { bitmap_layer_destroy( s_charging_bitmap_layer ); s_charging_bitmap_layer=NULL; };
  layer_destroy( s_main_layer ); 
  LOG( "end main window unload" );
};


////////////////////
// initialize app //
////////////////////
static void init() {
  config_clear();
  config_load();
  
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // show window on the watch with animated=true
  window_stack_push(s_main_window, true);

  // subscribe to time events
  tick_timer_service_subscribe(MINUTE_UNIT, handler_tick); 

  // register with Battery State Service
  battery_state_service_subscribe(handler_battery);
  // force initial update
  handler_battery(battery_state_service_peek());      
  
  // register with bluetooth state service
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = callback_bluetooth
  });
  // force bluetooth
  callback_bluetooth(connection_service_peek_pebble_app_connection());  
    
  // Register weather callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);  
  app_message_open(128, 128);  

  LOG( "init" );  
};


///////////////////////
// de-initialize app //
///////////////////////
static void deinit() {
  window_destroy(s_main_window);
};


/////////////
// run app //
/////////////
int main(void) {
  init();
  app_event_loop();
  deinit();
};
