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
unsigned long readTimeout = 5000;  // Timeout for receiving messages in ms

const char CR = '\r';
const char LF = '\n';

// Button pin is defined as 'BUTTON' for Sodaq Explorer
const int buttonPin = BUTTON;  // Using the BUTTON constant for the pin

// Variable to track button press state
bool buttonPressed = false;

void setup() {
  SerialUSB.begin(57600);  // Start Serial Monitor
  Serial2.begin(57600);    // Start Serial2 for LoRa communication

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Set the button pin as input with pull-up resistor

  while (!SerialUSB && millis() < 1000); // Wait for USB serial to be ready

  // Seed the random number generator using an analog pin
  randomSeed(analogRead(A0));

  LoraP2P_Setup();  // Initialize LoRa settings

  digitalWrite(LED_BUILTIN, HIGH);  // Indicate setup complete
  SerialUSB.println("LoRa combined receiver and transmitter ready.");
}

void loop() {
  static unsigned long lastSendTime = 0;
  static unsigned long sendInterval = random(1000, 15001); // Random interval between 1 and 15 seconds
  unsigned long currentTime = millis();

  // First, check for incoming messages
  char receivedData[100] = ""; // Buffer for incoming data

  if (LORA_Read(receivedData) == 1) { // If a message is received
    digitalWrite(LED_GREEN, LOW); // Blink green LED to indicate a message
    delay(100);
    digitalWrite(LED_GREEN, HIGH);

    SerialUSB.print("Received: ");
    SerialUSB.println(receivedData);
  } else {
    StartLoraRead(); // Ensure the LoRa module is back in receive mode
  }

  // Check if the button is pressed (buttonPin reads LOW when pressed)
  if (digitalRead(buttonPin) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;  // Button is now pressed
      char sendData[100] = "48656c6c6f20576f726c6421"; // "Hello World!" in HEX
      LORA_Write(sendData);

      digitalWrite(LED_GREEN, LOW); // Blink green LED to indicate sending
      delay(100);
      digitalWrite(LED_GREEN, HIGH);

      SerialUSB.println("Data sent: Hello World!");

      StartLoraRead(); // Switch back to receive mode after sending
    }
  } else {
    buttonPressed = false; // Reset button state when it's released
  }

  delay(50); // Small delay to avoid rapid switching
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
  Serial2.print(readTimeout);
  Serial2.print("\r\n");
  delay(100);
  Serial2.print("mac pause\r\n");
  delay(100);

  FlushSerialBufferIn(); // Flush any previous data in the serial buffer
}

// Send data over LoRa
void LORA_Write(char* Data) {
  Serial2.print("radio tx ");
  Serial2.print(Data);
  Serial2.print("\r\n");
  Serial2.flush();

  waitTillMessageGone(); // Wait until the message transmission is complete
}

// Wait until the message is transmitted
void waitTillMessageGone() {
  while (!Serial2.available()); // Wait until data is available
  delay(10); // Small delay
  while (Serial2.available() > 0)
    Serial2.read(); // Read available data to clear the buffer
}

// Start LoRa receiver to listen for messages
void StartLoraRead() {
  Serial2.print("radio rx 0\r\n");
  delay(100);

  FlushSerialBufferIn(); // Flush the buffer before reading new data
}

// Read a message from the LoRa receiver
int LORA_Read(char* Data) {
  unsigned long startTime = millis();
  int messageFlag = 0;
  String dataStr = "radio_rx  ";
  String errorStr = "radio_err";
  String Buffer = "";

  StartLoraRead(); // Start receiving LoRa message

  while (millis() - startTime < readTimeout) { // Keep checking until timeout
    if (Serial2.available() > 0) {
      char c = Serial2.read();
      Buffer += c;

      // Check if a complete line is received
      if (c == LF) {
        // Check if the received data starts with "radio_rx"
        if (Buffer.startsWith(dataStr)) {
          int i = 10; // Data starts at the 11th character
          while (Buffer[i] != CR && i - 10 < max_dataSize) {
            Data[i - 10] = Buffer[i]; // Store received data
            i++;
          }
          messageFlag = 1; // Message received successfully
          break;
        } else if (Buffer.startsWith(errorStr)) {
          messageFlag = 2; // Error in message or timeout
          break;
        }
        Buffer = ""; // Reset buffer for the next line
      }
    }
  }

  return messageFlag; // Return message status
}

// Flush any available data in the serial buffer
void FlushSerialBufferIn() {
  while (Serial2.available() > 0) {
    Serial2.read(); // Clear out any available data
  }
}
