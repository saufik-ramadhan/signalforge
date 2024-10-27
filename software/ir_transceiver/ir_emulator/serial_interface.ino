void serialInterfaceListen() {
  if (Serial.available() > 0) { // Check if there's data available
    String command = Serial.readStringUntil('\n'); // Read the entire line until newline

    if (command == "/up") {
      Serial.println("Received up command");
      oledDisplayRender("/up");
      // Add your code here to control your device or system for up
    } else if (command == "/left") {
      Serial.println("Received left command");
      // Add your code here to control your device or system for left
    } else if (command == "/down") {
      Serial.println("Received down command");
      oledDisplayRender("/down");
      // Add your code here to control your device or system for down
    } else if (command == "/right") {
      Serial.println("Received right command");
      // Add your code here to control your device or system for right
    } else if (command == "/ok") {
      Serial.println("Received ok command");
      // Add your code here to control your device or system for ok
    } else if (command == "/reset") {
      Serial.println("Received reset command");
      // Add your code here to control your device or system for reset
    } else {
      Serial.println("Invalid command");
    }
  }
}

