//#define DEBUG
#include <Arduino.h>
// Function Declarations
void LoraP2P_Setup();
void LORA_Write(char* Data);
void waitTillMessageGone();
void FlushSerialBufferIn();

// LoRa Configuration Variables
int8_t trPower = 1;         // Transmission power (can be -3 to 15)
String SprFactor = "sf12";  // Spreading factor (can be sf7 to sf12)
unsigned long readDelay = 60000; // Time to read for messages in ms (max 4294967295 ms, 0 to disable)

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
  char Data[100] = "48656c6c6f20576f726c6421";  // Some data to send (in HEX)

  LORA_Write(Data);  // Send the data
  digitalWrite(LED_GREEN, LOW);  // Turn off green LED to indicate data sent
  delay(100);  // Wait for a brief moment
  digitalWrite(LED_GREEN, HIGH);  // Turn on green LED again to indicate ready for next data

  delay(2000);  // Delay between transmissions
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

// Flush any available data in the serial buffer
void FlushSerialBufferIn() {
  while (Serial2.available() > 0) {
    Serial2.read();  // Clear out any available data
  }
}
