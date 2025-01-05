#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include <Update.h>
#include <FS.h>
#include <SD.h>

#define BUTTON_BACK_PIN 32 // pin for BACK button
int button_back_clicked = 0; // same as above

#define WIFI_CHANNEL_SWITCH_INTERVAL  (500)
#define WIFI_CHANNEL_MAX               (13)


uint8_t level = 0, channel = 1;

static wifi_country_t wifi_country = {.cc="CN", .schan = 1, .nchan = 13}; //Most recent esp32 library struct

typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_init(void);
static void wifi_sniffer_set_channel(uint8_t channel);
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

esp_err_t event_handler(void *ctx, system_event_t *event)
{
  return ESP_OK;
}

void wifi_sniffer_init(void)
{
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
  ESP_ERROR_CHECK( esp_wifi_start() );
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_sniffer_set_channel(uint8_t channel)
{
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch(type) {
  case WIFI_PKT_MGMT: return "MGMT";
  case WIFI_PKT_DATA: return "DATA";
  default:  
  case WIFI_PKT_MISC: return "MISC";
  }
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  printf("PACKET TYPE=%s, CHAN=%02d, RSSI=%02d,"
    " ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
    " ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
    " ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
    wifi_sniffer_packet_type2str(type),
    ppkt->rx_ctrl.channel,
    ppkt->rx_ctrl.rssi,
    /* ADDR1 */
    hdr->addr1[0],hdr->addr1[1],hdr->addr1[2],
    hdr->addr1[3],hdr->addr1[4],hdr->addr1[5],
    /* ADDR2 */
    hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
    hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],
    /* ADDR3 */
    hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],
    hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]
  );
}

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

// the setup function runs once when you press reset or power the board
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

    // initialize digital pin 5 as an output.
    delay(10);
    wifi_sniffer_init();
}

// the loop function runs over and over again forever
void loop() {
    //Serial.print("inside loop");
    delay(1000); // wait for a second
    
    vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
    wifi_sniffer_set_channel(channel);
    channel = (channel % WIFI_CHANNEL_MAX) + 1;
}

void TaskReadFromSerial(void *pvParameters) {
    String input;
    String command;
    int overflow;
    char userInput[50];

    message_t message;
    message_t* pMessage = &message;

    printFormatted("!Starts Serial Tasks");
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
            if (input.startsWith("get")) {
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