#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Global variables
String inputString = "";         // Holds incoming serial data
bool inputStringComplete = false;  // Flag for completed input
enum MenuState { MAIN_MENU, WIFI_SCAN, BT_SCAN, WIFI_SELECT, WIFI_CONNECT };
MenuState menuState = MAIN_MENU;
String selectedSSID = "";
BLEScan* pBLEScan;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  BLEDevice::init("");

  Serial.println("ESP32 Menu Program");
  showMenu();
}

void loop() {
  // Read serial input
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      inputStringComplete = true;
      break;
    } else {
      inputString += inChar;
    }
  }

  // Process input when complete
  if (inputStringComplete) {
    inputString.trim();  // Remove any leading/trailing whitespace
    switch (menuState) {
      case MAIN_MENU:
        handleMainMenuInput(inputString);
        break;
      case WIFI_SELECT:
        handleWiFiSelectInput(inputString);
        break;
      case WIFI_CONNECT:
        handleWiFiConnectInput(inputString);
        break;
      default:
        break;
    }
    inputString = "";
    inputStringComplete = false;
  }
}

void showMenu() {
  Serial.println("\n--- Main Menu ---");
  Serial.println("1. Scan and connect to WiFi network");
  Serial.println("2. Scan Bluetooth devices");
  Serial.print("Enter your choice: ");
}

void handleMainMenuInput(String input) {
  if (input == "1") {
    menuState = WIFI_SCAN;
    scanWiFiNetworks();
  } else if (input == "2") {
    menuState = BT_SCAN;
    scanBluetoothDevices();
  } else {
    Serial.println("Invalid choice.");
    showMenu();
  }
}

void scanWiFiNetworks() {
  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  Serial.println("Scan complete.");
  if (n == 0) {
    Serial.println("No networks found.");
    menuState = MAIN_MENU;
    showMenu();
  } else {
    Serial.println(String(n) + " networks found:");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.println(String(i) + ": " + WiFi.SSID(i) + " (Signal: " + WiFi.RSSI(i) + "dBm)");
    }
    Serial.print("Enter the number of the network you wish to connect to: ");
    menuState = WIFI_SELECT;
  }
}

void handleWiFiSelectInput(String input) {
  int choice = input.toInt();
  int n = WiFi.scanNetworks();
  if (choice >= 0 && choice < n) {
    selectedSSID = WiFi.SSID(choice);
    Serial.println("You have selected: " + selectedSSID);
    Serial.print("Enter the password: ");
    menuState = WIFI_CONNECT;
  } else {
    Serial.println("Invalid selection.");
    menuState = MAIN_MENU;
    showMenu();
  }
}

void handleWiFiConnectInput(String input) {
  String password = input;
  Serial.println("Connecting to " + selectedSSID + "...");
  WiFi.begin(selectedSSID.c_str(), password.c_str());
  int timeout = 10;  // 10 seconds timeout
  while (WiFi.status() != WL_CONNECTED && timeout > 0) {
    delay(1000);
    Serial.print(".");
    timeout--;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to " + selectedSSID);
    Serial.println("IP address: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nFailed to connect.");
  }
  menuState = MAIN_MENU;
  showMenu();
}

void scanBluetoothDevices() {
  Serial.println("Scanning for Bluetooth devices...");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);  // Active scan for more detailed results
  BLEScanResults foundDevices = pBLEScan->start(5);  // Scan for 5 seconds
  Serial.println("Scan complete.");
  int count = foundDevices.getCount();
  if (count == 0) {
    Serial.println("No Bluetooth devices found.");
  } else {
    Serial.println(String(count) + " devices found:");
    for (int i = 0; i < count; ++i) {
      BLEAdvertisedDevice device = foundDevices.getDevice(i);
      String deviceName = device.getName().c_str();
      if (deviceName.length() == 0) {
        deviceName = "Unknown";
      }
      Serial.println(String(i) + ": " + deviceName + " [" + device.getAddress().toString().c_str() + "]");
    }
  }
  menuState = MAIN_MENU;
  showMenu();
}
