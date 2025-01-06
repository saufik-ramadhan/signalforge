int wifi_item_sel = 0;
int wifi_item_prev = 0;
int wifi_item_next = 0;
int wifi_num_items = 0;
bool wifiEnabled = true;
const int MAX_WIFI_ITEM_LENGTH = 10;
const int MAX_WIFI_NUM_ITEM = 100;
unsigned long prevEnableWifiTime = 0;

char wifi_buffer_list[MAX_IR_NUM_ITEM][MAX_IR_ITEM_LENGTH] = {};

void WiFiSTAInit() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("WiFi Setup done");
}

void refresh_wifi_menu() {
  // set correct values for the previous and next items
  wifi_item_prev = wifi_item_sel - 1;
  if (wifi_item_prev < 0) {
    wifi_item_prev = wifi_num_items - 1;
  } // previous item would be below first = make it the last
  wifi_item_next = wifi_item_sel + 1;  
  if (wifi_item_next >= wifi_num_items) {
    wifi_item_next = 0;
  } // next item would be after last = make it the first
}

void list_wifi_nav() {
  refresh_wifi_menu();
  // up and down buttons only work for the menu screen
  if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 0)) { // up button clicked - jump to previous menu item
    wifi_item_sel = wifi_item_sel - 1; // select previous item
    button_up_clicked = 1; // set button to clicked to only perform the action once
    if (wifi_item_sel < 0) { // if first item was selected, jump to last item
      wifi_item_sel = wifi_num_items-1;
    }
  }
  else if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 0)) { // down button clicked - jump to next menu item
    wifi_item_sel = wifi_item_sel + 1; // select next item
    button_down_clicked = 1; // set button to clicked to only perform the action once
    if (wifi_item_sel >= wifi_num_items) { // last item was selected, jump to first menu item
      wifi_item_sel = 0;
    }
  }
}

void render_wifi_list() {
  list_wifi_nav();


  // selected item background
  u8g.drawBitmap(0, 22, 128/8, 21, bitmap_item_sel_outline);    

  if(wifi_num_items > 2) {
    // draw previous item as icon + label
    u8g.setFont(u8g_font_7x14);
    if (wifi_buffer_list[wifi_item_prev] != nullptr && wifi_buffer_list[wifi_item_prev][0] != '\0') {
        u8g.drawStr(25, 15, wifi_buffer_list[wifi_item_prev]);
    } else {
        u8g.drawStr(25, 15, "N/A"); // Default text if undefined
    }
    u8g.drawBitmap( 4, 2, 16/8, 16, bitmap_icons[2]);
  }

  // draw selected item as icon + label in bold font
  u8g.setFont(u8g_font_7x14B);    
  if (wifi_buffer_list[wifi_item_sel] != nullptr && wifi_buffer_list[wifi_item_sel][0] != '\0') {
      u8g.drawStr(25, 15 + 20 + 2, wifi_buffer_list[wifi_item_sel]);
  } else {
      u8g.drawStr(25, 15 + 20 + 2, "N/A"); // Default text if undefined
  }
  u8g.drawBitmap( 4, 24, 16/8, 16, bitmap_icons[2]);     

  if(wifi_num_items > 1) {
    // draw next item as icon + label
    u8g.setFont(u8g_font_7x14B);    
    if (wifi_buffer_list[wifi_item_next] != nullptr && wifi_buffer_list[wifi_item_next][0] != '\0') {
        u8g.drawStr(25, 15 + 20 + 20 + 2 + 2, wifi_buffer_list[wifi_item_next]);
    } else {
        u8g.drawStr(25, 15 + 20 + 20 + 2 + 2, "N/A"); // Default text if undefined
    }
    u8g.drawBitmap( 4, 46, 16/8, 16, bitmap_icons[2]); 
  }    

  //draw scrollbar handle
  // u8g.drawBox(125, 64/num_items * item_sel, 3, 64/num_items);

}

void noNetworkFound() {
  u8g.setFont(u8g_font_7x14);
  u8g.drawStr(25, 15, "No Network..");
  u8g.drawBitmap( 4, 2, 16/8, 16, bitmap_icons[2]);
}

void wifi_scan_tool() {
  unsigned long currentTime = millis();
  prevEnableIRTime = currentTime;

  if((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0) && wifiEnabled == true) {
    wifiEnabled = false;
    wifi_num_items = 0;
    wifi_item_sel = 0;
    memset(wifi_buffer_list, 0, sizeof(wifi_buffer_list));
    int n = WiFi.scanNetworks();
    Serial.println("Scan done");

    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
      for (int i = 0; i < n; ++i) {
        wifi_num_items += 1;
        // Print SSID and RSSI for each network found
        Serial.printf("%2d",i + 1);
        Serial.print(" | ");
        Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
        strcpy(wifi_buffer_list[i], { WiFi.SSID(i).c_str() });
        Serial.print(" | ");
        Serial.printf("%4d", WiFi.RSSI(i));
        Serial.print(" | ");
        Serial.printf("%2d", WiFi.channel(i));
        Serial.print(" | ");
        switch (WiFi.encryptionType(i))
        {
        case WIFI_AUTH_OPEN:
            Serial.print("open");
            break;
        case WIFI_AUTH_WEP:
            Serial.print("WEP");
            break;
        case WIFI_AUTH_WPA_PSK:
            Serial.print("WPA");
            break;
        case WIFI_AUTH_WPA2_PSK:
            Serial.print("WPA2");
            break;
        case WIFI_AUTH_WPA_WPA2_PSK:
            Serial.print("WPA+WPA2");
            break;
        case WIFI_AUTH_WPA2_ENTERPRISE:
            Serial.print("WPA2-EAP");
            break;
        case WIFI_AUTH_WPA3_PSK:
            Serial.print("WPA3");
            break;
        case WIFI_AUTH_WPA2_WPA3_PSK:
            Serial.print("WPA2+WPA3");
            break;
        case WIFI_AUTH_WAPI_PSK:
            Serial.print("WAPI");
            break;
        default:
            Serial.print("unknown");
        }
        Serial.println();
        delay(10);
      }
    }
    Serial.println("");
    wifiEnabled = true;
  }
  
}