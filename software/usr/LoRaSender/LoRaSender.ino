#include <SPI.h>
#include <LoRa.h>

int counter = 0;

#define ss 15
#define rst 4
#define dio0 -1

SPIClass *hspi = NULL;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  hspi = new SPIClass(HSPI);
  Serial.println("LoRa Sender");

  LoRa.setSPI(*hspi);
  LoRa.setPins(ss, rst, dio0);
  
  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);

  // send packet
  LoRa.beginPacket();
  LoRa.print("hello ");
  LoRa.print(counter);
  LoRa.endPacket();

  counter++;

  delay(5000);
}
