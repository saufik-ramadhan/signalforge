/**
 * Bluetooth Classic Example
 * Scan for devices - asyncronously, print device as soon as found
 * query devices for SPP - SDP profile
 * connect to first device offering a SPP connection
 * 
 * Example python server:
 * source: https://gist.github.com/ukBaz/217875c83c2535d22a16ba38fc8f2a91
 *
 * Tested with Raspberry Pi onboard Wifi/BT, USB BT 4.0 dongles, USB BT 1.1 dongles, 
 * 202202: does NOT work with USB BT 2.0 dongles when esp32 aduino lib is compiled with SSP support!
 *         see https://github.com/espressif/esp-idf/issues/8394
 *         
 * use ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE in connect() if remote side requests 'RequireAuthentication': dbus.Boolean(True),
 * use ESP_SPP_SEC_NONE or ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE in connect() if remote side has Authentication: False
 */

#include <map>
#include <BluetoothSerial.h>

#include <Update.h>
#include <FS.h>
#include <SD.h>

#define BUTTON_BACK_PIN 32 // pin for BACK button
int button_back_clicked = 0; // same as above

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif

BluetoothSerial SerialBT;


#define BT_DISCOVER_TIME  10000
esp_spp_sec_t sec_mask=ESP_SPP_SEC_NONE; // or ESP_SPP_SEC_ENCRYPT|ESP_SPP_SEC_AUTHENTICATE to request pincode confirmation
esp_spp_role_t role=ESP_SPP_ROLE_SLAVE; // or ESP_SPP_ROLE_MASTER

// std::map<BTAddress, BTAdvertisedDeviceSet> btDeviceList;

// perform the actual update from a given stream
void performUpdate(Stream &updateSource, size_t updateSize) {
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
void updateFromFS(fs::FS &fs) {
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


void setup() {
    uint8_t cardType;
    Serial.begin(115200);
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


  if(! SerialBT.begin("ESP32test", true) ) {
    Serial.println("========== serialBT failed!");
    abort();
  }
  // SerialBT.setPin("1234"); // doesn't seem to change anything
  // SerialBT.enableSSP(); // doesn't seem to change anything


  Serial.println("Starting discoverAsync...");
  BTScanResults* btDeviceList = SerialBT.getScanResults();  // maybe accessing from different threads!
  if (SerialBT.discoverAsync([](BTAdvertisedDevice* pDevice) {
      // BTAdvertisedDeviceSet*set = reinterpret_cast<BTAdvertisedDeviceSet*>(pDevice);
      // btDeviceList[pDevice->getAddress()] = * set;
      Serial.printf(">>>>>>>>>>>Found a new device asynchronously: %s\n", pDevice->toString().c_str());
    } )
    ) {
    delay(BT_DISCOVER_TIME);
    Serial.print("Stopping discoverAsync... ");
    SerialBT.discoverAsyncStop();
    Serial.println("discoverAsync stopped");
    delay(5000);
    if(btDeviceList->getCount() > 0) {
      BTAddress addr;
      int channel=0;
      Serial.println("Found devices:");
      for (int i=0; i < btDeviceList->getCount(); i++) {
        BTAdvertisedDevice *device=btDeviceList->getDevice(i);
        Serial.printf(" ----- %s  %s %d\n", device->getAddress().toString().c_str(), device->getName().c_str(), device->getRSSI());
        std::map<int,std::string> channels=SerialBT.getChannels(device->getAddress());
        Serial.printf("scanned for services, found %d\n", channels.size());
        for(auto const &entry : channels) {
          Serial.printf("     channel %d (%s)\n", entry.first, entry.second.c_str());
        }
        if(channels.size() > 0) {
          addr = device->getAddress();
          channel=channels.begin()->first;
        }
      }
      if(addr) {
        Serial.printf("connecting to %s - %d\n", addr.toString().c_str(), channel);
        SerialBT.connect(addr, channel, sec_mask, role);
      }
    } else {
      Serial.println("Didn't find any devices");
    }
  } else {
    Serial.println("Error on discoverAsync f.e. not workin after a \"connect\"");
  }
}


String sendData="Hi from esp32!\n";

void loop() {
  if(! SerialBT.isClosed() && SerialBT.connected()) {
    if( SerialBT.write((const uint8_t*) sendData.c_str(),sendData.length()) != sendData.length()) {
      Serial.println("tx: error");
    } else {
      Serial.printf("tx: %s",sendData.c_str());
    }
    if(SerialBT.available()) {
      Serial.print("rx: ");
      while(SerialBT.available()) {
        int c=SerialBT.read();
        if(c >= 0) {
          Serial.print((char) c);
        }
      }
      Serial.println();
    }
  } else {
    Serial.println("not connected");
  }
  delay(1000);
}

void TaskReadFromSerial(void *pvParameters) {
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
                    userInput[i+1] = 0;
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