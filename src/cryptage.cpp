#include <Wire.h>
#include <RTClib.h>
#include <Crypto.h>
#include <AES.h>

// Déclaration des clés AES (10 clés de 16 octets pour AES-128)
const byte aes_keys[10][16] = {
  {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10},
  {0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20},
  {0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30},
  {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40},
  {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50},
  {0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60},
  {0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70},
  {0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80},
  {0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90},
  {0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0}
};

const byte* getEncryptionKey(int minute) {
  int keyIndex = minute % 10;
  return aes_keys[keyIndex];
}

void aes_encrypt(const byte* input, byte* output, const byte* key) {
  AES128 aes;
  aes.setKey(key, 16);
  aes.encryptBlock(output, input);
}

void aes_decrypt(const byte* input, byte* output, const byte* key) {
  AES128 aes;
  aes.setKey(key, 16);
  aes.decryptBlock(output, input);
}

void encryptMessage(const char* message, int hour, int minute, int second, byte* result) {
  char timeStr[9];
  snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", hour, minute, second);
  
  byte plainText[16];
  memcpy(plainText, message, 16);
  
  const byte* currentKey = getEncryptionKey(minute);
  aes_encrypt(plainText, result + 8, currentKey);
  
  memcpy(result, timeStr, 8);
}

void decryptMessage(const byte* encryptedMessage, char* decryptedText) {
  char timeStr[9];
  memcpy(timeStr, encryptedMessage, 8);
  timeStr[8] = '\0';
  
  int hour, minute, second;
  sscanf(timeStr, "%d:%d:%d", &hour, &minute, &second);
  
  const byte* currentKey = getEncryptionKey(minute);
  byte decryptedBytes[16];
  aes_decrypt(encryptedMessage + 8, decryptedBytes, currentKey);
  
  memcpy(decryptedText, decryptedBytes, 16);
  decryptedText[16] = '\0';
  
  char fullDecryptedText[26];
  snprintf(fullDecryptedText, sizeof(fullDecryptedText), "%s %s", timeStr, decryptedText);
  strcpy(decryptedText, fullDecryptedText);
}

void setup() {
  SerialUSB.begin(9600);
  while (!SerialUSB);
  SerialUSB.println("Sodaq Explorer prêt !");
}

void loop() {
  int hour = 12;
  int minute = 35;
  int second = 10;

  const char* message = "Hello, World!";
  SerialUSB.print("Message original : ");
  SerialUSB.println(message);

  byte encryptedMessage[24];
  encryptMessage(message, hour, minute, second, encryptedMessage);

  SerialUSB.print("Message chiffré : ");
  for (int i = 0; i < 24; i++) {
    SerialUSB.print(encryptedMessage[i], HEX);
    SerialUSB.print(" ");
  }
  SerialUSB.println();

  char decryptedText[26];
  decryptMessage(encryptedMessage, decryptedText);

  SerialUSB.print("Message déchiffré : ");
  SerialUSB.println(decryptedText);

  delay(5000);
}
