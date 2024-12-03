//#define DEBUG
#include <Arduino.h>
// Function Declarations
void LoraP2P_Setup();
void LORA_Write(char* Data);
void waitTillMessageGone();
void StartLoraRead();
int LORA_Read(char* Data);
void FlushSerialBufferIn();

// LoRa Configuration Variables
int8_t trPower = 14;         // Transmission power (can be -3 to 15)
String SprFactor = "sf12";  // Spreading factor (can be sf7 to sf12)
uint8_t max_dataSize = 100; // Maximum charcount for messages
unsigned long readDelay = 60000; // Time to wait for messages in ms (max 4294967295 ms, 0 to disable)

const char CR = '\r';
const char LF = '\n';

void setup() {
  SerialUSB.begin(57600);  // Start Serial Monitor
  Serial2.begin(57600);    // Start Serial2 for LoRa communication

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  while (!SerialUSB && millis() < 1000); // Wait for USB serial to be ready

  LoraP2P_Setup();  // Initialize LoRa settings

  digitalWrite(LED_BUILTIN, HIGH);  // Indicate setup in progress
}

void loop() {
  digitalWrite(LED_GREEN, HIGH);  // Turn on green LED when listening for messages
  char Data[100] = "";  // Buffer to store received message

  if (LORA_Read(Data) == 1) {  // If message is received
    digitalWrite(LED_GREEN, LOW);  // Turn off green LED
    SerialUSB.println(Data);  // Print received message to Serial Monitor
  }

  delay(100);  // Small delay before checking for another message
}

// Function Definitions

// Set up LoRa with specific configurations
void LoraP2P_Setup() {
  Serial2.print("sys reset\r\n");
  delay(200);
  Serial2.print("radio set pwr ");
  Serial2.print(trPower);
  Serial2.print("\r\n");
  delay(100);
  Serial2.print("radio set sf ");
  Serial2.print(SprFactor);
  Serial2.print("\r\n");
  delay(100);
  Serial2.print("radio set wdt ");
  Serial2.print(readDelay);
  Serial2.print("\r\n");
  delay(100);
  Serial2.print("mac pause\r\n");
  delay(100);

  FlushSerialBufferIn();  // Flush any previous data in the serial buffer
}

// Send data over LoRa
void LORA_Write(char* Data) {
  Serial2.print("radio tx ");
  Serial2.print(Data);
  Serial2.print("\r\n");
  Serial2.flush();

  waitTillMessageGone();  // Wait until the message transmission is complete
}

// Wait until the message is transmitted
void waitTillMessageGone() {
  while (!Serial2.available());  // Wait until data is available
  delay(10);  // Small delay
  while (Serial2.available() > 0)
    Serial2.read();  // Read available data to clear the buffer

  while (!Serial2.available());  // Wait again for more data
  delay(10);  // Small delay
  while (Serial2.available() > 0) {
    Serial2.read();  // Discard any remaining data
  }
}

// Start LoRa receiver to listen for messages
void StartLoraRead() {
  Serial2.print("radio rx 0\r\n");
  delay(100);

  FlushSerialBufferIn();  // Flush the buffer before reading new data
}

// Read a message from the LoRa receiver
int LORA_Read(char* Data) {
  int messageFlag = 0;
  String dataStr = "radio_rx  ";
  String errorStr = "radio_err";
  String Buffer = "";

  StartLoraRead();  // Start receiving LoRa message

  while (messageFlag == 0) {  // Keep checking until a message is received
    while (!Serial2.available());  // Wait until data is available

    delay(50);  // Small delay to allow buffer to fill

    // Read the available data until a linefeed (LF) is encountered
    while (Serial2.available() > 0 && Serial2.peek() != LF) {
      Buffer += (char)Serial2.read();
    }

    // Check if the received data starts with "radio_rx"
    if (Buffer.startsWith(dataStr, 0)) {
      int i = 10;  // Data starts at the 11th character
      while (Buffer[i] != CR && i - 10 < max_dataSize) {
        Data[i - 10] = Buffer[i];  // Store received data
        i++;
      }
      messageFlag = 1;  // Message received successfully
    } else if (Buffer.startsWith(errorStr, 0)) {
      messageFlag = 2;  // Error in message or timeout
    }
  }

  return (messageFlag);  // Return message status
}

// Flush any available data in the serial buffer
void FlushSerialBufferIn() {
  while (Serial2.available() > 0) {
    Serial2.read();  // Clear out any available data
  }
}
