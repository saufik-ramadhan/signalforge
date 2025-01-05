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

void get_ir_module_db() {
  File file = SD.open("/infrared/");
}

void render_list_ir_cmd() {
  // selected item background
    u8g.setFlipMode(0);
    u8g.drawBitmap(0, 22, 128/8, 21, bitmap_item_sel_outline);

    // draw previous item as icon + label
    u8g.setFont(u8g_font_7x14);
    u8g.drawStr(25, 15, "huyhuiy"); 
    u8g.drawBitmap( 4, 2, 16/8, 16, bitmap_icons[0]);          

    // draw selected item as icon + label in bold font
    u8g.setFont(u8g_font_7x14B);    
    u8g.drawStr(25, 15+20+2, ir_buffer_list[1]);   
    u8g.drawBitmap( 4, 24, 16/8, 16, bitmap_icons[0]);     

    // draw next item as icon + label
    u8g.setFont(u8g_font_7x14);     
    u8g.drawStr(25, 15+20+20+2+2, ir_buffer_list[0]);   
    u8g.drawBitmap( 4, 46, 16/8, 16, bitmap_icons[0]);        

    // draw scrollbar handle
    u8g.drawBox(125, 64/num_items * item_sel, 3, 64/num_items);
}

void list_ir_files(File dir, int numTabs) {
  if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (isRendering == false)) {
    isRendering = true;
    while (true) {
      File entry = dir.openNextFile();
      if (!entry) {
        // No more files
        isRendering = false;
        render_list_ir_cmd();
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