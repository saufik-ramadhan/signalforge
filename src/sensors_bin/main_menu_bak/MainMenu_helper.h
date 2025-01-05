// Global Params
const int NUM_ITEMS = 6; // number of items in the list and also the number of screenshots and screenshots with QR codes (other screens)
const int NUM_CHILD_ITEMS = 3;
const int MAX_ITEM_LENGTH = 20; // maximum characters for the item name
int number_of_items = NUM_ITEMS;

// Keep Track of Menu, Screen, and Running Function
bool module_function_is_running = false;
int current_screen = 0;   // Menu definition is on Menu_List.h
int item_selected = 0;    // which item in the menu is selected
int parent_idx = -1;
int item_sel_previous;    // previous item - used in the menu screen to draw the item before the selected one
int item_sel_next;        // next item - used in the menu screen to draw next item after the selected one

char main_menu[NUM_ITEMS][MAX_ITEM_LENGTH] = {
  { "IR Module" },
  { "NFC" },
  { "WiFi Tools" },
  { "Micro SD" },
  { "Bluetooth" },
  { "LoRA" }
};

char child_menu[NUM_ITEMS][NUM_CHILD_ITEMS][MAX_ITEM_LENGTH] = {
  {"Read IR", "Send IR", "List IR Cmd"},        // IR Menus
  {"Read NFC", "Write NFC", "List NFC"},        // NFC Menus
  {"Scan AP", "Sniff Traffic", "Deauth WiFi"},  // WiFi Menus
  {"Read File", "Format Storage", "Info"},      // Storage Menus
  {"Receive", "Send", "Info"},                  // Bluetooth
  {"Receive", "Send", "Info"},                  // LoRA
};




// char menu_items [NUM_ITEMS][MAX_ITEM_LENGTH] = {  // array with item names
//   { "IR Module" }, 
//   { "NFC" }, 
//   { "WiFi Tools" }, 
//   { "Micro SD" }, 
//   { "Bluetooth" }
// };

// note - when changing the order of items above, make sure the other arrays referencing bitmaps
// also have the same order, for example array "bitmap_icons" for icons, and other arrays for screenshots and QR codes

void moduleDaemon(int parent, int child) {
  if (module_function_is_running == true) {
    if(parent == 0 && child == 0) { // read ir
      read_ir_signal();
    } else if (parent == 0 && child == 1) { // write ir
      send_ir_signal();
    } else if (parent == 0 && child == 2) { // list ir files
      list_ir_files(0);
    }
  } else {
    // do nothing
  }
}

void handleMenu() {
  if(current_screen == 0 || current_screen == 1) {
    // up and down buttons only work for the menu screen
    if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 0)) { // up button clicked - jump to previous menu item
      item_selected = item_selected - 1; // select previous item
      button_up_clicked = 1; // set button to clicked to only perform the action once
      if (item_selected < 0) { // if first item was selected, jump to last item
        item_selected = number_of_items-1;
      }
    }
    else if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 0)) { // down button clicked - jump to next menu item
      item_selected = item_selected + 1; // select next item
      button_down_clicked = 1; // set button to clicked to only perform the action once
      if (item_selected >= number_of_items) { // last item was selected, jump to first menu item
        item_selected = 0;
      }
    }
  }
 

  if(current_screen == 0) {
    if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0)) {
      number_of_items = NUM_CHILD_ITEMS;
      current_screen = current_screen + 1;
      parent_idx = item_selected;
      button_select_clicked = 1;
      item_selected = 0;
    } else if ((digitalRead(BUTTON_BACK_PIN) == LOW) && (button_back_clicked == 0)) {
      current_screen = 0; // you are already at main menu
      button_back_clicked = 1;
      item_selected = 0;
    }
  } else if(current_screen == 1) {
    if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 0)) {
      // Run Module
      module_function_is_running = true;
      current_screen = current_screen + 1;
      button_select_clicked = 1;
    } else if ((digitalRead(BUTTON_BACK_PIN) == LOW) && (button_back_clicked == 0)) {
      // back to main menu
      number_of_items = NUM_ITEMS;
      parent_idx = -1;
      item_selected = 0;
      current_screen = current_screen - 1;
      button_back_clicked = 1;
    }
  } else if(current_screen == 2) {
    if ((digitalRead(BUTTON_BACK_PIN) == LOW) && (button_back_clicked == 0)) {
      // back to child menu
      current_screen = current_screen - 1;
      module_function_is_running = false;
      button_back_clicked = 1;
    }
  }

  // debounce button
  if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 1)) { // unclick 
    button_up_clicked = 0;
  }
  if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 1)) { // unclick
    button_down_clicked = 0;
  }
  if ((digitalRead(BUTTON_SELECT_PIN) == HIGH) && (button_select_clicked == 1)) { // unclick
    button_select_clicked = 0;
  }
  if ((digitalRead(BUTTON_BACK_PIN) == HIGH) && (button_back_clicked == 1)) { // unclick
    button_back_clicked = 0;
  }

  // set correct values for the previous and next items
  item_sel_previous = item_selected - 1;
  if (item_sel_previous < 0) {
    item_sel_previous = number_of_items - 1;
  } // previous item would be below first = make it the last
  item_sel_next = item_selected + 1;  
  if (item_sel_next >= number_of_items) {
    item_sel_next = 0;
  } // next item would be after last = make it the first


  u8g.firstPage(); // required for page drawing mode for u8g library
  do {
    if (current_screen == 0) { // MENU SCREEN
      render_menu_list(number_of_items);
    } else if (current_screen == 1) {
      render_child_menu_list(parent_idx, number_of_items);
    } else if (current_screen == 2) {
      render_running_screen();
    }
  } while ( u8g.nextPage() ); // required for page drawing mode with u8g library



  // Daemon
  moduleDaemon(parent_idx, item_selected);
}

void render_menu_list(int num_items) {
    // selected item background
    u8g.setFlipMode(0);
    u8g.drawBitmap(0, 22, 128/8, 21, bitmap_item_sel_outline);

    // draw previous item as icon + label
    u8g.setFont(u8g_font_7x14);
    u8g.drawStr(25, 15, main_menu[item_sel_previous]); 
    u8g.drawBitmap( 4, 2, 16/8, 16, bitmap_icons[item_sel_previous]);          

    // draw selected item as icon + label in bold font
    u8g.setFont(u8g_font_7x14B);    
    u8g.drawStr(25, 15+20+2, main_menu[item_selected]);   
    u8g.drawBitmap( 4, 24, 16/8, 16, bitmap_icons[item_selected]);     

    // draw next item as icon + label
    u8g.setFont(u8g_font_7x14);     
    u8g.drawStr(25, 15+20+20+2+2, main_menu[item_sel_next]);   
    u8g.drawBitmap( 4, 46, 16/8, 16, bitmap_icons[item_sel_next]);        

    // draw scrollbar handle
    u8g.drawBox(125, 64/num_items * item_selected, 3, 64/num_items);   
}

void render_child_menu_list(int parent_idx, int num_items) {
    // selected item background
    u8g.drawBitmap(0, 22, 128/8, 21, bitmap_item_sel_outline);

    // draw previous item as icon + label
    u8g.setFont(u8g_font_7x14);
    u8g.drawStr(25, 15, child_menu[parent_idx][item_sel_previous]); 
    u8g.drawBitmap( 4, 2, 16/8, 16, bitmap_child_icons[parent_idx][item_sel_previous]);          

    // draw selected item as icon + label in bold font
    u8g.setFont(u8g_font_7x14B);    
    u8g.drawStr(25, 15+20+2, child_menu[parent_idx][item_selected]);   
    u8g.drawBitmap( 4, 24, 16/8, 16, bitmap_child_icons[parent_idx][item_selected]);     

    // draw next item as icon + label
    u8g.setFont(u8g_font_7x14);     
    u8g.drawStr(25, 15+20+20+2+2, child_menu[parent_idx][item_sel_next]);   
    u8g.drawBitmap( 4, 46, 16/8, 16, bitmap_child_icons[parent_idx][item_sel_next]);        

    // draw scrollbar handle
    u8g.drawBox(125, 64/num_items * item_selected, 3, 64/num_items);   
}

void render_running_screen() {
  if(parent_idx == 0 && item_selected == 2) {
    // List IR Command Screen
    render_list_ir_cmd();
    // u8g.clear();
  } else {
    u8g.setFont(u8g_font_7x14B);
    u8g.drawStr(0, 15, "running:");
  }
}