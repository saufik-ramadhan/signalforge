#include <Arduino.h>
// #include "PinDefinitionsAndMore.h"
#include <U8g2lib.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g(U8G2_R2, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 9, /* data=*/ 8);   // ESP32 Thing, HW I2C with pin remapping

#define BUTTON_UP_PIN 11 // pin for UP button 
#define BUTTON_SELECT_PIN 13 // pin for SELECT button
#define BUTTON_DOWN_PIN 10 // pin for DOWN button
#define DEMO_PIN 12 // pin for demo mode, use switch or wire to enable or disable demo mode, see more details below



/** Needed variable for <Main_Menu.ino> **/
int child_menu_size = 5;

int button_up_clicked = 0; // only perform action when button is clicked, and wait until another press
int button_select_clicked = 0; // same as above
int button_down_clicked = 0; // same as above

int item_selected = 0; // which item in the menu is selected
int item_selected_child = 0; // which item in the menu is selected
int item_selected_leaf = 0; // which item in the menu is selected

int item_sel_previous; // previous item - used in the menu screen to draw the item before the selected one
int item_sel_previous_child; // previous item - used in the menu screen to draw the item before the selected one
int item_sel_previous_leaf; // previous item - used in the menu screen to draw the item before the selected one

int item_sel_next; // next item - used in the menu screen to draw next item after the selected one
int item_sel_next_child; // next item - used in the menu screen to draw next item after the selected one
int item_sel_next_leaf; // next item - used in the menu screen to draw next item after the selected one

int current_screen = 0;   // 0 = menu, 1 = screenshot, 2 = qr

int demo_mode = 0; // when demo mode is set to 1, it automatically goes over all the screens, 0 = control menu with buttons
int demo_mode_state = 0; // demo mode state = which screen and menu item to display
int demo_mode_delay = 0; // demo mode delay = used to slow down the screen switching
/** --- **/



void setup() {
  Serial.begin(115200);

  // GFX
  u8g.begin();

  // Input Buttons
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP); // up button
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP); // select button
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP); // down button
  pinMode(DEMO_PIN, INPUT_PULLUP);

  // IRRemote
  // -- receiver
  // IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

}


void loop() {
  handleMenu();
}