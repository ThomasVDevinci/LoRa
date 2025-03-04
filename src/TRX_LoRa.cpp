int8_t trPower = 1;         // Transreceiver power  (can be -3 to 15)
String SprFactor = "sf12";  // Spreadingsfactor (can be sf7 to sf12)
uint8_t max_dataSize = 100; // Maximum charcount to avoid writing outside of string
unsigned long readDelay = 5000; // Time to read for messages in ms (max 4294967295 ms, 0 to disable)

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

void LORA_Write(char* Data) {
    Serial2.print("radio tx ");
    Serial2.print(Data);
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

    while (messageFlag == 0) { // As long as there is no message
        while (!Serial2.available());
        
        delay(50);  // Some time for the buffer to fill

        // Read message from RN2483 LORA chip
        while (Serial2.available() > 0 && Serial2.peek() != LF) {
            Buffer += (char)Serial2.read();
        }

        // If there is an incoming message
        if (Buffer.startsWith(dataStr, 0)) {
            int i = 10;
            int j = 0;
            while (Buffer[i] != CR && j < max_dataSize - 1) {
                char hex[3] = {Buffer[i], Buffer[i+1], '\0'};
                Data[j] = (char)strtol(hex, NULL, 16);
                i += 2;
                j++;
            }
            Data[j] = '\0';
            messageFlag = 1; // Message received
        }
        else if (Buffer.startsWith(errorStr, 0)) {
            messageFlag = 2; // Read error or Watchdogtimer timeout
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
                // Convertir les données en hexadécimal
                char hexData[200];
                for (int i = 0; i < inputIndex; i++) {
                    sprintf(&hexData[i * 2], "%02X", inputData[i] & 0xFF);
                }
                hexData[inputIndex * 2] = '\0';  // Terminer la chaîne en hexadécimal
                
                // Envoyer le message en hexadécimal
                LORA_Write(hexData);
                
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
