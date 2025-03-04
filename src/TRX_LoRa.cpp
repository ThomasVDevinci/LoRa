#include <Wire.h>
#include <RTClib.h>
#include <Crypto.h>
#include <AES.h>

// Identifiant unique de la carte (à modifier pour chaque carte)
const String cardID = "CARD_01";

int8_t trPower = 1;         // Puissance du transmetteur (peut être de -3 à 15)
String SprFactor = "sf12";  // Facteur d'étalement (peut être sf7 à sf12)
uint8_t max_dataSize = 100; // Nombre maximum de caractères pour éviter d'écrire en dehors de la chaîne
unsigned long readDelay = 5000; // Temps de lecture des messages en ms (max 4294967295 ms, 0 pour désactiver)

const unsigned long frequencies[] = {
    868100000,  // 868.1 MHz
    868300000,  // 868.3 MHz
    868500000,  // 868.5 MHz
    867100000,  // 867.1 MHz
    867300000,  // 867.3 MHz
    867500000,  // 867.5 MHz
    867700000,  // 867.7 MHz
    867900000   // 867.9 MHz
};

const char CR = '\r';
const char LF = '\n';

const int NUM_FREQUENCIES = 8;
unsigned long lastFrequencyChange = 0;
const unsigned long FREQUENCY_CHANGE_INTERVAL = 60000; // 60 secondes en millisecondes
int currentFrequencyIndex = 0;

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

void setLoRaFrequency(unsigned long frequency) {
    Serial2.print("radio set freq ");
    Serial2.print(frequency / 1000000.0, 3); // Convertir en MHz avec 3 décimales
    Serial2.print("\r\n");
    delay(100);
    FlushSerialBufferIn();
}

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

    setLoRaFrequency(frequencies[currentFrequencyIndex]);

    Serial2.print("mac pause\r\n");
    delay(100);

    FlushSerialBufferIn();
}

void LORA_Write(const char* Data) {
    byte encryptedMessage[24];
    int hour = 12; // Remplacez par l'heure actuelle
    int minute = 35; // Remplacez par la minute actuelle
    int second = 10; // Remplacez par la seconde actuelle

    encryptMessage(Data, hour, minute, second, encryptedMessage);

    Serial2.print("radio tx ");
    for (int i = 0; i < 24; i++) {
        Serial2.print(encryptedMessage[i], HEX);
    }
    Serial2.print("\r\n");
    Serial2.flush();

    waitTillMessageGone();
}

void waitTillMessageGone() {
    while (!Serial2.available());
    delay(10);
    while (Serial2.available() > 0)
        Serial2.read();

    while (!Serial2.available());
    delay(10);
    while (Serial2.available() > 0) {
#ifdef DEBUG
        SerialUSB.write(Serial2.read());
#else
        Serial2.read();
#endif
    }
}

void StartLoraRead() {
    Serial2.print("radio rx 0\r\n");
    delay(100);

    FlushSerialBufferIn();
}

int LORA_Read(char* Data) {
    int messageFlag = 0;
    String dataStr = "radio_rx  ";
    String errorStr = "radio_err";
    String Buffer = "";

    StartLoraRead();

    while (messageFlag == 0) { // Tant qu'il n'y a pas de message
        while (!Serial2.available());

        delay(50);  // Un peu de temps pour que le tampon se remplisse

        // Lire le message du module LoRa RN2483
        while (Serial2.available() > 0 && Serial2.peek() != LF) {
            Buffer += (char)Serial2.read();
        }

        // S'il y a un message entrant
        if (Buffer.startsWith(dataStr, 0)) {
            byte encryptedMessage[24];
            for (int i = 0; i < 48; i += 2) {
                char hex[3] = {Buffer[i + 10], Buffer[i + 11], '\0'};
                encryptedMessage[i / 2] = (byte)strtol(hex, NULL, 16);
            }

            char decryptedText[26];
            decryptMessage(encryptedMessage, decryptedText);

            // Afficher le message déchiffré pour le débogage
            SerialUSB.print("Message déchiffré : ");
            SerialUSB.println(decryptedText);

            strncpy(Data, decryptedText, max_dataSize);
            Data[max_dataSize - 1] = '\0';
            messageFlag = 1; // Message reçu
        }
        else if (Buffer.startsWith(errorStr, 0)) {
            messageFlag = 2; // Erreur de lecture ou dépassement du Watchdog timer
        }
    }

#ifdef DEBUG
    SerialUSB.println(Buffer);
#endif

    return (messageFlag);
}

void FlushSerialBufferIn() {
    while (Serial2.available() > 0) {
#ifdef DEBUG
        SerialUSB.write(Serial2.read());
#else
        Serial2.read();
#endif
    }
}

void checkAndUpdateFrequency() {
    unsigned long currentTime = millis();
    if (currentTime - lastFrequencyChange >= FREQUENCY_CHANGE_INTERVAL) {
        currentFrequencyIndex = (currentFrequencyIndex + 1) % NUM_FREQUENCIES;
        setLoRaFrequency(frequencies[currentFrequencyIndex]);

        lastFrequencyChange = currentTime;

        #ifdef DEBUG
        SerialUSB.print("Changement de fréquence: ");
        SerialUSB.print(frequencies[currentFrequencyIndex] / 1000000.0, 3);
        SerialUSB.println(" MHz");
        #endif
    }
}

void setup() {
    SerialUSB.begin(57600);
    Serial2.begin(57600);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    while (!SerialUSB && millis() < 1000);
    LoraP2P_Setup();
    digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
    static char inputData[100];
    static int inputIndex = 0;

    checkAndUpdateFrequency();

    // Lecture des données depuis le moniteur série
    while (SerialUSB.available() > 0) {
        char inChar = SerialUSB.read();

        if (inChar != '\n' && inChar != '\r') {
            inputData[inputIndex] = inChar;
            inputIndex++;

            if (inputIndex >= 99) {
                inputIndex = 98;
            }
            inputData[inputIndex] = '\0';
        }
        else {
            if (inputIndex > 0) {
                // Ajouter l'identifiant de la carte et ": " au début des données
                String messageWithID = cardID + ": " + String(inputData);

                // Envoyer le message chiffré
                LORA_Write(messageWithID.c_str());

                digitalWrite(LED_GREEN, LOW);
                delay(100);
                digitalWrite(LED_GREEN, HIGH);

                SerialUSB.print("Message envoyé: ");
                SerialUSB.println(inputData);

                inputIndex = 0;
                inputData[0] = '\0';
            }
        }
    }

    // Lecture des données reçues
    char receiveData[100];
    if (LORA_Read(receiveData) == 1) {
        SerialUSB.print("Message reçu: ");
        SerialUSB.println(receiveData);
        digitalWrite(LED_GREEN, LOW);
        delay(50);
        digitalWrite(LED_GREEN, HIGH);
    }
}
