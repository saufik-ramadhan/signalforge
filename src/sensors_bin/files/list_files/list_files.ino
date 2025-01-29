#include <Arduino.h>

#include <Update.h>
#include <FS.h>
#include <SD.h>


#include "SD_Card.h"


// BUTTON proto
#define BUTTON_BACK_PIN 32 // pin for BACK button

// BUTTONS State
int button_back_clicked = 0; // same as above


void setup() {
    uint8_t cardType;
    Serial.begin(115200);
    //   Serial.println("Welcome to the SD-Update example!");

    // You can uncomment this and build again
    // Serial.println("Update successful");

    // Input Buttons
    pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);

    // Create Tasks
    xTaskCreate(
        TaskReadFromSerial,
        "Task Read From Serial",
        2048,
        NULL,
        1,
        NULL // Task handle is not used here
    );

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
}

// perform the actual update from a given stream
void performUpdate(Stream& updateSource, size_t updateSize) {
    if (Update.begin(updateSize)) {
        size_t written = Update.writeStream(updateSource);
        if (written == updateSize) {
            Serial.println("Written : " + String(written) + " successfully");
        } else {
            Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
        }
        if (Update.end()) {
            Serial.println("OTA done!");
            if (Update.isFinished()) {
                Serial.println("Update successfully completed. Rebooting.");
            } else {
                Serial.println("Update not finished? Something went wrong!");
            }
        } else {
            Serial.println("Error Occurred. Error #: " + String(Update.getError()));
        }

    } else {
        Serial.println("Not enough space to begin OTA");
    }
}

// check given FS for valid update.bin and perform update if available
void updateFromFS(fs::FS& fs) {
    File updateBin = fs.open("/signalforge/main.bin");
    if (updateBin) {
        if (updateBin.isDirectory()) {
            Serial.println("Error, update.bin is not a file");
            updateBin.close();
            return;
        }

        size_t updateSize = updateBin.size();

        if (updateSize > 0) {
            Serial.println("Try to start update");
            performUpdate(updateBin, updateSize);
        } else {
            Serial.println("Error, file is empty");
        }

        updateBin.close();

        // when finished remove the binary from sd card to indicate end of the process
        fs.remove("/signalforge/update.bin");
    } else {
        Serial.println("Could not load update.bin from sd signalforge/ directory");
    }
}

void rebootEspWithReason(String reason) {
    Serial.println(reason);
    delay(1000);
    ESP.restart();
}


void loop() {
    delay(2000);
    listDir(SD, "/", 2);
}

void TaskReadFromSerial(void* pvParameters) {
    String input;
    String command;
    int overflow;
    char userInput[50];

    for (;;) {
        if (Serial.available() > 0) {
            overflow = 1;

            // Read Serial until termniation
            for (int i = 0; i < 50 - 1; i++) {
                while (Serial.available() == 0) {}
                userInput[i] = Serial.read();
                if (userInput[i] == '\n' || userInput[i] == '\0' || userInput[i] == '\r') {
                    userInput[i + 1] = 0;
                    overflow = 0;
                    break;
                }
            }

            if (overflow == 1) {
                break;
            }

            input = String(userInput);

            // Process command get
            if (input.startsWith("back")) {
                updateFromFS(SD);
            }

            Serial.flush();

        }

        if ((digitalRead(BUTTON_BACK_PIN) == LOW) && (button_back_clicked == 0)) {
            // back to main menu                
            updateFromFS(SD);

        }

        delay(1000); // wait for a second
    }
}