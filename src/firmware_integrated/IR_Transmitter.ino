#include <Arduino.h>
#include "IR_PinDefinitions.h"
#include <IRremote.hpp>

bool irEnabled = true;
uint16_t sAddress = 0x7;
uint8_t sCommand = 0x2;
uint16_t s16BitCommand = 0x5634;
uint8_t sRepeats = 0;
const int MAX_IR_ITEM_LENGTH = 10;
const int MAX_IR_NUM_ITEM = 100;
bool isRendering = false;
String ir_module_path = "/infrared";
int ir_module_screen = 0;

int item_sel = 0;
int item_prev = 0;
int item_next = 0;
int num_items = 0;

char ir_buffer_list[MAX_IR_NUM_ITEM][MAX_IR_ITEM_LENGTH] = {};

struct IRSignal {
  String name;
  String protocol;
  String address;
  String command;
};

IRSignal irSignals[10]; // Adjust size based on expected signals


unsigned long prevEnableIRTime = 0;

void refresh_ir_menu() {
  // set correct values for the previous and next items
  item_prev = item_sel - 1;
  if (item_prev < 0) {
    item_prev = num_items - 1;
  } // previous item would be below first = make it the last
  item_next = item_sel + 1;  
  if (item_next >= num_items) {
    item_next = 0;
  } // next item would be after last = make it the first
}

void list_ir_nav() {
  refresh_ir_menu();
  // up and down buttons only work for the menu screen
  if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 0)) { // up button clicked - jump to previous menu item
    item_sel = item_sel - 1; // select previous item
    button_up_clicked = 1; // set button to clicked to only perform the action once
    if (item_sel < 0) { // if first item was selected, jump to last item
      item_sel = num_items-1;
    }
  }
  else if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 0)) { // down button clicked - jump to next menu item
    item_sel = item_sel + 1; // select next item
    button_down_clicked = 1; // set button to clicked to only perform the action once
    if (item_sel >= num_items) { // last item was selected, jump to first menu item
      item_sel = 0;
    }
  }

  // if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0)) {
  //   number_of_items = NUM_CHILD_ITEMS;
  //   current_screen = current_screen + 1;
  //   parent_idx = item_selected;
  //   button_select_clicked = 1;
  //   item_selected = 0;
  // } else if ((digitalRead(BUTTON_BACK_PIN) == LOW) && (button_back_clicked == 0)) {
  //   current_screen = 0; // you are already at main menu
  //   button_back_clicked = 1;
  //   item_selected = 0;
  // }

  // debounce button
  // if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1)) { // unclick 
  //   button_up_clicked = 0;
  // }
  // if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1)) { // unclick
  //   button_down_clicked = 0;
  // }
  // if ((digitalRead(BUTTON_SELECT_PIN) == HIGH) && (button_select_clicked == 1)) { // unclick
  //   button_select_clicked = 0;
  // }
  // if ((digitalRead(BUTTON_BACK_PIN) == HIGH) && (button_back_clicked == 1)) { // unclick
  //   button_back_clicked = 0;re
  // }
}

void render_list_ir_cmd() {
    list_ir_nav();

    // selected item background
    u8g.drawBitmap(0, 22, 128/8, 21, bitmap_item_sel_outline);    

    // draw previous item as icon + label
    u8g.setFont(u8g_font_7x14);
    if (ir_buffer_list[item_prev] != nullptr && ir_buffer_list[item_prev][0] != '\0') {
        u8g.drawStr(25, 15, ir_buffer_list[item_prev]);
    } else {
        u8g.drawStr(25, 15, "N/A"); // Default text if undefined
    }
    u8g.drawBitmap( 4, 2, 16/8, 16, bitmap_icons[0]);

    // draw selected item as icon + label in bold font
    u8g.setFont(u8g_font_7x14B);    
    if (ir_buffer_list[item_sel] != nullptr && ir_buffer_list[item_sel][0] != '\0') {
        u8g.drawStr(25, 15 + 20 + 2, ir_buffer_list[item_sel]);
    } else {
        u8g.drawStr(25, 15 + 20 + 2, "N/A"); // Default text if undefined
    }
    u8g.drawBitmap( 4, 24, 16/8, 16, bitmap_icons[0]);     

    // draw next item as icon + label
    u8g.setFont(u8g_font_7x14B);    
    if (ir_buffer_list[item_next] != nullptr && ir_buffer_list[item_next][0] != '\0') {
        u8g.drawStr(25, 15 + 20 + 20 + 2 + 2, ir_buffer_list[item_next]);
    } else {
        u8g.drawStr(25, 15 + 20 + 20 + 2 + 2, "N/A"); // Default text if undefined
    }
    u8g.drawBitmap( 4, 46, 16/8, 16, bitmap_icons[0]);     

    //draw scrollbar handle
    // u8g.drawBox(125, 64/num_items * item_sel, 3, 64/num_items);
}

void list_ir_files(int numTabs) {
  if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (isRendering == false)) {
    isRendering = true;
    num_items = 0;

    String current_path = ir_module_path;
    if (ir_buffer_list[item_sel] != nullptr && ir_buffer_list[item_sel][0] != '\0') {
      current_path = current_path + "/" + ir_buffer_list[item_sel];
      memset(ir_buffer_list, 0, sizeof(ir_buffer_list));
    } else {
      current_path = current_path;
    }
    File dir = SD.open(current_path);
    
    // set correct values for the previous and next items
    item_prev = item_sel - 1;
    if (item_prev < 0) {
      item_prev = num_items - 1;
    } // previous item would be below first = make it the last
    item_next = item_sel + 1;  
    if (item_next >= num_items) {
      item_next = 0;
    } // next item would be after last = make it the first

    while (true) {
      File entry = dir.openNextFile();
      if (!entry) {
        // No more files
        isRendering = false;
        break;
      }
      
      strcpy(ir_buffer_list[num_items], { entry.name() });
      num_items += 1;

      Serial.print(entry.name());
      if (entry.isDirectory()) {
        Serial.println("/");
        // list_ir_files(entry, numTabs + 1); // Recursive call for subdirectories
      } else {
        // Files, print size
        Serial.print("\t");
        Serial.println(entry.size(), DEC);
      }
      entry.close();
    }
  }
}

void ir_module_init() {
#if defined(IR_SEND_PIN)
  IrSender.begin(); // Start with IR_SEND_PIN
  Serial.println(F("IR Sender <OK> at pin " STR(IR_SEND_PIN)));
#endif

#if defined(IR_RECEIVE_PIN)
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Serial.print(F("IR Receiver <OK>"));
  printActiveIRProtocols(&Serial);
  Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));
#endif

#if !defined(SEND_PWM_BY_TIMER)
  /*
  * Print internal software PWM signal generation info
  */
  IrSender.enableIROut(38); // Call it with 38 kHz just to initialize the values printed below
  Serial.print(F("Send signal mark duration is "));
  Serial.print(IrSender.periodOnTimeMicros);
  Serial.print(F(" us, pulse narrowing correction is "));
  Serial.print(IrSender.getPulseCorrectionNanos());
  Serial.print(F(" ns, total period is "));
  Serial.print(IrSender.periodTimeMicros);
  Serial.println(F(" us"));
#endif
}

void send_ir_signal() {
  unsigned long currentTime = millis();
  
  // Set target addr and cmd through serial input
  if (Serial.available() > 0) {
    // Read the input as a string
    String input = Serial.readString();
    input.trim(); // Remove any leading or trailing whitespace

    // Split the input by spaces (assuming space is the delimiter)
    char *addr, *cmd;
    int numValues = sscanf(input.c_str(), "%s %s", &addr, &cmd);
    uint16_t hexAddr = strtol(addr, NULL, 16); // Convert using base 16
    uint16_t hexCmd = strtol(cmd, NULL, 16); // Convert using base 16

    if (numValues == 2) {
      // Successfully read 3 integers
      Serial.print("Received values: ");
      Serial.print(addr);
      Serial.print(", ");
      Serial.print(cmd);

      
    } else {
      // Handle incorrect input
      Serial.println("Invalid input. Please enter three integer values.");
    }
  }

  if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (irEnabled == false)) { // select button clicked, jump between screens
    prevEnableIRTime = currentTime;
    irEnabled = true;
    Serial.println(F("Send Samsung 8 bit command"));
    Serial.flush();
    IrSender.sendSamsung(sAddress, sCommand, sRepeats);
  }
  if ((digitalRead(BUTTON_SELECT_PIN) == HIGH) && (irEnabled == true) && (currentTime - prevEnableIRTime >= 250)) { // unclick 
    irEnabled = false;
  }
}


char* ir_address;
char* ir_command;
char* ir_make;

void render_ir_info() {
  u8g.clear();
  u8g.setFont(u8g_font_7x14B);
  

  // if (ir_address_buffer != nullptr && ir_address_buffer[0] != '\0') {
  //     u8g.drawStr(25, 15, ir_address_buffer);
  // } else {
  //     u8g.drawStr(25, 15, "N/A"); // Default text if undefined
  // }
  if (ir_command != nullptr && ir_command[0] != '\0') {
      u8g.drawStr(25, 15 + 20 + 2, ir_command);
  } else {
      u8g.drawStr(25, 15 + 20 + 2, "N/A"); // Default text if undefined
  }
  // if (ir_make != nullptr && ir_make[0] != '\0') {
  //     u8g.drawStr(25, 15 + 20 + 20 + 2 + 2, ir_make);
  // } else {
  //     u8g.drawStr(25, 15 + 20 + 20 + 2 + 2, "N/A"); // Default text if undefined
  // }
}

void read_ir_signal() {
  if (IrReceiver.decode()) {
    /** Print a summary of received data */
    if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
      Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
      // We have an unknown protocol here, print extended info
      IrReceiver.printIRResultRawFormatted(&Serial, true);
      IrReceiver.resume(); // Do it here, to preserve raw data for printing with printIRResultRawFormatted()
    } else {
      IrReceiver.resume(); // Early enable receiving of the next IR frame
      IrReceiver.printIRResultShort(&Serial);
      IrReceiver.printIRSendUsage(&Serial);

      char ir_address_buffer[10];           // Create a character buffer
      sprintf(ir_address, "%d", IrReceiver.decodedIRData.address); // Convert integer to string
      char ir_command_buffer[10];           // Create a character buffer
      sprintf(ir_command, "%d", IrReceiver.decodedIRData.command); // Convert integer to string
      strcpy(ir_make, getProtocolString(IrReceiver.decodedIRData.protocol)); // Copy content
    }
    Serial.println();

    /** Finally, check the received data and perform actions according to the received command */
    if (IrReceiver.decodedIRData.command == 0x10) {
        // do something
        
    } else if (IrReceiver.decodedIRData.command == 0x11) {
        // do something else
    }
  }
}