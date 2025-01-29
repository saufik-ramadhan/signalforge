
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#include <Update.h>
#include <FS.h>
#include <SD.h>

#define BUTTON_BACK_PIN 32 // pin for BACK button
int button_back_clicked = 0; // same as above

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (14)
#define PN532_MOSI (13)
#define PN532_SS   (33)
#define PN532_MISO (12)

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
// #define PN532_IRQ   (13)
// #define PN532_RESET (22)  // Not connected by default on the NFC Shield

// Use this line for a breakout with a software SPI connection (recommended):
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

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

void setup(void) {
    Serial.begin(115200);
    while (!Serial) delay(10); // for Leonardo/Micro/Zero

    Serial.println("Hello!");

    uint8_t cardType;
    //   Serial.println("Welcome to the SD-Update example!");

    // You can uncomment this and build again
    // Serial.println("Update successful");

    //first init and check SD card
    if (!SD.begin()) {
        rebootEspWithReason("Card Mount Failed");
    }

    cardType = SD.cardType();

    if (cardType == CARD_NONE) {
        rebootEspWithReason("No SD_MMC card attached");
    }

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

    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.print("Didn't find PN53x board");
        while (1); // halt
    }
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

    Serial.println("Waiting for an ISO14443A Card ...");
}


void loop(void) {
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
    // 'uid' will be populated with the UID, and uidLength will indicate
    // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
        // Display some basic information about the card
        Serial.println("Found an ISO14443A card");
        Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);
        Serial.println("");

        if (uidLength == 4) {
            // We probably have a Mifare Classic card ...
            Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

            // Now we need to try to authenticate it for read/write access
            // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
            Serial.println("Trying to authenticate block 4 with default KEYA value");
            uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

            // Start with block 4 (the first block of sector 1) since sector 0
            // contains the manufacturer data and it's probably better just
            // to leave it alone unless you know what you're doing
            success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);

            if (success) {
                Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
                uint8_t data[16];

                // If you want to write something to block 4 to test with, uncomment
            // the following line and this text should be read back in a minute
                //memcpy(data, (const uint8_t[]){ 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0 }, sizeof data);
                // success = nfc.mifareclassic_WriteDataBlock (4, data);

                // Try to read the contents of block 4
                success = nfc.mifareclassic_ReadDataBlock(4, data);

                if (success) {
                    // Data seems to have been read ... spit it out
                    Serial.println("Reading Block 4:");
                    nfc.PrintHexChar(data, 16);
                    Serial.println("");

                    // Wait a bit before reading the card again
                    delay(1000);
                } else {
                    Serial.println("Ooops ... unable to read the requested block.  Try another key?");
                }
            } else {
                Serial.println("Ooops ... authentication failed: Try another key?");
            }
        }

        if (uidLength == 7) {
            // We probably have a Mifare Ultralight card ...
            Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");

            // Try to read the first general-purpose user page (#4)
            Serial.println("Reading page 4");
            uint8_t data[32];
            success = nfc.mifareultralight_ReadPage(4, data);
            if (success) {
                // Data seems to have been read ... spit it out
                nfc.PrintHexChar(data, 4);
                Serial.println("");

                // Wait a bit before reading the card again
                delay(1000);
            } else {
                Serial.println("Ooops ... unable to read the requested page!?");
            }
        }
    }
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