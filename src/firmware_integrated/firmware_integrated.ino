#include <Arduino.h>
#include <U8g2lib.h>
#include <Update.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "Menu_List.h"
#include "Bitmaps.h"
#include "WiFi.h"

/** ===============================
                MACROS 
    =============================== **/
// OLED
#define SDA_PIN 21
#define SCL_PIN 22
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g(U8G2_R2, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ SCL_PIN, /* data=*/ SDA_PIN);   // ESP32 Thing, HW I2C with pin remapping

// BUTTON
// #define BUTTON_UP_PIN 25 // pin for UP button 
// #define BUTTON_DOWN_PIN 26 // pin for DOWN button
// #define BUTTON_SELECT_PIN 27 // pin for SELECT button
// #define BUTTON_BACK_PIN 32 // pin for BACK button
// #define BUTTON_ACTION_PIN 33

// BUTTON proto
#define BUTTON_UP_PIN 25 // pin for UP button 
#define BUTTON_DOWN_PIN 26 // pin for DOWN button
#define BUTTON_SELECT_PIN 27 // pin for SELECT button
#define BUTTON_BACK_PIN 32 // pin for BACK button
#define BUTTON_ACTION_PIN 33

// BUTTONS State
int button_up_clicked = 0; // only perform action when button is clicked, and wait until another press
int button_down_clicked = 0; // same as above
int button_select_clicked = 0; // same as above
int button_back_clicked = 0; // same as above
int button_action_clicked = 0; // same as above


/** ===============================
            GLOBAL VARIABLES 
    =============================== **/

void setup() {
  uint8_t cardType;
  Serial.begin(115200);

  // GFX
  u8g.begin();

  // Input Buttons
  pinMode(BUTTON_UP_PIN, INPUT_PULLUP); // up button
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP); // down button
  pinMode(BUTTON_SELECT_PIN, INPUT_PULLUP); // select button
  pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ACTION_PIN, INPUT_PULLUP);

  // if (!SD.begin()) {
  //   rebootEspWithReason("Card Mount Failed");
  // }
  // cardType = SD.cardType();

  // // SDCard Firmware Update
  // if (cardType == CARD_NONE) {
  //   rebootEspWithReason("No SD_MMC card attached");
  // } else {
  //   fwSDupdateFromFS(SD);
  // }

  // SDCard
  sdCardInit();

  // IRRemote
  ir_module_init();

}


void loop() {
  handleMenu();
  // send_ir_signal();
}