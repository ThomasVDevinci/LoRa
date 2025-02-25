#define loraSerial Serial2

String inputMessage = "";
boolean messageReady = false;

void setup() {
  SerialUSB.begin(9600);
  while (!SerialUSB);
  
  loraSerial.begin(57600);
  while (!loraSerial);
  
  SerialUSB.println("Configuring RN2483 for P2P mode...");
  
  sendCommand("sys reset");
  sendCommand("mac pause");
  sendCommand("radio set mod lora");
  sendCommand("radio set freq 868100000");
  sendCommand("radio set pwr 14");
  sendCommand("radio set sf sf7");
  sendCommand("radio set bw 125");
  sendCommand("radio set cr 4/5");
  sendCommand("radio set wdt 0");
  
  SerialUSB.println("Configuration complete.");
  SerialUSB.println("Enter a message to send (end with newline):");
}

void loop() {
  // Check for user input
  while (SerialUSB.available()) {
    char c = SerialUSB.read();
    if (c == '\n') {
      messageReady = true;
    } else {
      inputMessage += c;
    }
  }

  // If a message is ready, send it
  if (messageReady) {
    sendLoRaMessage(inputMessage);
    inputMessage = "";
    messageReady = false;
    SerialUSB.println("Enter a new message to send:");
  }

  // Check for incoming LoRa messages
  receiveLoRaMessage();
}

void sendLoRaMessage(String message) {
  SerialUSB.println("Sending message: " + message);
  String hexMessage = stringToHex(message);
  SerialUSB.println("Hex message: " + hexMessage);
  sendCommand("radio tx " + hexMessage);
  
  String txResponse = loraSerial.readStringUntil('\n');
  SerialUSB.println("TX Response: " + txResponse);
}

void receiveLoRaMessage() {
  sendCommand("radio rx 0");
  
  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    if (loraSerial.available()) {
      String received = loraSerial.readStringUntil('\n');
      if (received.startsWith("radio_rx")) {
        String data = received.substring(10); // Remove "radio_rx "
        SerialUSB.println("Raw received data: " + data);
        String decodedMessage = hexToString(data);
        SerialUSB.println("Decoded message: " + decodedMessage);
        break;
      }
    }
  }
}

void sendCommand(String command) {
  SerialUSB.print("Sending command: ");
  SerialUSB.println(command);
  
  loraSerial.println(command);
  String response = loraSerial.readStringUntil('\n');
  
  SerialUSB.print("Response: ");
  SerialUSB.println(response);
  
  if (response != "ok" && !command.startsWith("radio rx")) {
    SerialUSB.println("Command failed: " + response);
  }
}

String stringToHex(String input) {
  String output = "";
  for (int i = 0; i < input.length(); i++) {
    if (input.charAt(i) < 0x10) output += "0";
    output += String(input.charAt(i), HEX);
  }
  return output;
}

String hexToString(String hex) {
  String output = "";
  for (int i = 0; i < hex.length(); i += 2) {
    if (i + 1 < hex.length()) {
      String part = hex.substring(i, i + 2);
      char ch = strtol(part.c_str(), NULL, 16);
      output += ch;
    }
  }
  return output;
}
