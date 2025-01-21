#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "src\PPA_Error_Correction.h"

#define MAX_BASES_LENGTH 128

char* receivedData;
uint8_t* alicemessage;
uint8_t* aliceBases;
uint8_t* matchingIndices;
uint8_t* indsecretmsg;
unsigned int siftedlen = 0;
uint8_t* subset_data;
uint8_t* errorcorrecta;
uint8_t* sifted_key;
unsigned int key_size = 0;
unsigned int subsetlen = 0;
unsigned int msg_size = 0;
bool dataReady = false;
bool readym = false;
bool mircvd = false;
bool subsetack = false;
float keyUsage = 0.4;
unsigned long startTime = millis();

uint8_t* hexToBinary(const char* hex, int n);
void siftData(int msg_size, int siftedlen, uint8_t* message, uint8_t* indices);
void performErrorCorrection(uint8_t* errorcorrecta, int msg_size);
void shuffleData(uint8_t* data, int* indices, int size);
String uint8ArrayToHexString(uint8_t* array, size_t length);
void printUint8ArrayAsBinary(uint8_t* array, size_t length);

void setup() {
  Serial.begin(230400);
  Serial2.begin(230400);

  Serial.println("ready");

  receivedData = (char*)malloc(1024 * sizeof(char));
  if (receivedData == NULL) {
    Serial.println("Error: Memory allocation failed for receivedData.");
    return;
  }
}


void loop() {
  if (Serial.available() > 0 && !dataReady) {
    
    int receivedBytes = Serial.readBytesUntil('\0', receivedData, 1024);
    receivedData[receivedBytes] = '\0';
    
    unsigned long elapsedTime = millis() - startTime;
    sscanf(receivedData, "%u;", &msg_size);
    char* alicemHex = (char*)malloc((msg_size) * sizeof(char));
    char* aliceBasesHex = (char*)malloc((msg_size) * sizeof(char));
    sscanf(receivedData, "%*u;%[^;];%s", alicemHex, aliceBasesHex);
    msg_size /= 8;
    alicemessage = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));
    aliceBases = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));
    size_t alicemByteCount, aliceBasesByteCount;
    alicemessage = hexToBinary(alicemHex, msg_size);
    aliceBases = hexToBinary(aliceBasesHex, msg_size);
    free(alicemHex);
    free(aliceBasesHex);

    matchingIndices = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));
    errorcorrecta = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));

    dataReady = true;
  }

  if (dataReady) {

    unsigned long timeoutStart = millis();

    while (true) {
      if (Serial2.available() > 0) {
        String handshake = Serial2.readStringUntil('\n');
        delay(100);
        Serial.print("Handshake Completed: ");
        Serial.println(handshake);

        writeByteArray(msg_size, aliceBases);
        Serial2.flush();
        break;
      }
      if (millis() - timeoutStart > 10000) {
        Serial.println("Alice: Timeout waiting for Bob's handshake.");
        return;
      }
      delay(50);
    }

    timeoutStart = millis();
    while (!readym ) {
      String received = Serial2.readStringUntil('\n');
      if (received == "READYM") {
        Serial2.print("ACKM\n");
        Serial2.flush();
        Serial.println("Alice: Sent ACKM for matched indices");
        readym = true;
        break;
      } else {
        Serial.println(received);
        Serial.println("Alice: ReadyM not received");
      }
    }

    while (!mircvd ) {
      if (Serial2.available() > 0) {
        
      /*  while (totalReceivedBytes < msg_size) {
          int receivedBytes = Serial2.readBytes(matchingIndices + totalReceivedBytes, msg_size);
          totalReceivedBytes += receivedBytes;
          if (receivedBytes == 0) {
            // If no bytes are received, handle the timeout situation
            if (millis() - timeoutStart > 10000) {
              Serial.println("Alice: Timeout waiting for matching indices from Bob.");
              return;
            }
            delay(50);
          }
        }*/

        readByteArray(msg_size, matchingIndices);
        Serial.println("Alice : Matching Indices :");
        printUintArray(matchingIndices, msg_size);

        unsigned long elapsedTime = millis() - startTime;
        Serial.print("\nAlice: Time taken to receive MI: ");
        Serial.println(elapsedTime);
        mircvd = true;
        break;

        }

        if (millis() - timeoutStart > 10000) {
          Serial.println("Alice: Timeout waiting for matching indices from Bob.");
          return;
      }
      delay(50);
    }
    
    siftedlen = countones(matchingIndices, msg_size)/8;
    subsetlen = round(keyUsage * siftedlen);
    indsecretmsg =  (uint8_t*)malloc(siftedlen * sizeof(uint8_t*));
    //indsecretmsg = {0};
    for(int i = 0; i < siftedlen; i++)
      indsecretmsg[i] = 0;

    sifted_key =(uint8_t*)malloc(siftedlen * sizeof(uint8_t*));
    key_size = siftedlen - subsetlen;
    siftData(msg_size, siftedlen, alicemessage, matchingIndices, sifted_key);
    errorcorrecta =(uint8_t*)malloc(key_size * sizeof(uint8_t*));
    subset_data =(uint8_t*)malloc(subsetlen * sizeof(uint8_t*));

    // debug
    Serial.println("Alice : Matching Indices :");
    printUintArray(matchingIndices, msg_size);



    Serial.println("Alice : Sifted Key:");
    if (sifted_key) {
      printUintArray(sifted_key, siftedlen);
      Serial.println();
      Serial.println(siftedlen);
      Serial.println(key_size);
    } else {
      Serial.println("Alice : Error: sifted_key is NULL.");
    }

    generaterandomindeces(siftedlen, subsetlen,indsecretmsg);
    generateSubset(siftedlen, subsetlen, sifted_key, indsecretmsg, subset_data);
    errorCorrectData(siftedlen, key_size, sifted_key, indsecretmsg,errorcorrecta);
    Serial.println();

    Serial.println("Alice : Subset Data:");
    if (subset_data) {
      printUintArray(subset_data, subsetlen);
      Serial.println(subsetlen);
    } else {
      Serial.println("Alice : Error: subset_data is NULL.");
    }

    Serial.println("Alice : Error Corrected Data:");
    if (errorcorrecta) {
      printUintArray(errorcorrecta, key_size);
      Serial.println();
    } else {
      Serial.println("Alice : Error: errorcorrecta is NULL.");
    }

    unsigned long elapsedTime = millis() - startTime;
    Serial.print("\nAlice: Time taken to sift data and begin PE: ");
    Serial.println(elapsedTime);
    Serial.flush();
    delay(200);
    timeoutStart = millis();
    while (true) {
      if (Serial2.available() > 0) {
        String signal = Serial2.readStringUntil('\n');
        delay(50);
        Serial.println();
        Serial.print("Alice: Received signal: ");
        Serial.println(signal);
        Serial.println(signal.indexOf("READY_FOR_subset_data"));
        if (signal.indexOf("READY_FOR_subset_data") != -1) {
                unsigned long elapsedTime = millis() - startTime;
        Serial.print("\nAlice: Time reading subset: ");
        Serial.println(elapsedTime);
          writeByteArray(subsetlen, subset_data);
          delay(200);
          Serial.print("Alice: Sent subset data... ");
          Serial2.flush();
          break;
        }
        if (millis() - timeoutStart > 10000) {
        Serial.println("Alice: Timeout waiting for Bob's readiness.");
        return;
        }
        
      }

      
      delay(50);
    }

        timeoutStart = millis();
    while (true) {
      if (Serial2.available() > 0) {
    Serial.println("Alice : Subset indices:");
    //Serial.println((char*)indsecretmsg);
    printUintArray(indsecretmsg, siftedlen);
        String signal = Serial2.readStringUntil('\n');
        delay(50);
        Serial.println();
        Serial.print("Alice: Received signal: ");
        Serial.println(signal);
        Serial.println(signal.indexOf("READY_FOR_subset_ind"));
        if (signal.indexOf("READY_FOR_subset_ind") != -1) {
                unsigned long elapsedTime = millis() - startTime;
        Serial.print("\nAlice: Time writing subset indeces : ");
        Serial.println(elapsedTime);
          writeByteArray(siftedlen, indsecretmsg);
          delay(200);
          Serial.print("Alice: Sent subset index... ");
          Serial2.flush();
          break;
        }
        if (millis() - timeoutStart > 10000) {
        Serial.println("Alice: Timeout waiting for Bob's readiness.");
        return;
        }
        
      }

      
      delay(50);
    }

  }
}

uint8_t* hexToBinary(const char* hex, int n) {

  uint8_t* result = (uint8_t*)malloc(n * sizeof(uint8_t));

  for(int i = 0; i < n; i++)
  {
    result[i] = StrToHex(hex[i*2]);
    result[i] = result[i] << 4;
    result[i] = result[i] | StrToHex(hex[i*2+1]);
    
  }

   return result;
  }

 void generaterandomindeces(int siftedlen, int subsetlen, uint8_t* indsecretmsg) {

  // Seed the random number generator
  randomSeed(analogRead(0));
  int count = 0;
  // for ever byte pick two positions randomly to make 1
  while (count < subsetlen*8) { 
    for(int i =0; count < subsetlen*8 && i< siftedlen; i++) {      
      int index = random(0, 8);      
      if(bitRead(indsecretmsg[i], index)==0) {
        bitWrite(indsecretmsg[i],index,1);
        count++; 
      }             
    }   
  }
}


void siftData(int msg_size, int siftedlen, uint8_t* message, uint8_t* indices,uint8_t*  sifted_key) {
  int j = 0;
  for (int i = 0; i < msg_size && j < siftedlen*8; i++) {
    for (int k = 0; k < 8 ; k++) {
      if(bitRead(indices[i], k))
      {
        bitWrite(sifted_key[j/8],j%8,bitRead(message[i], k));
        j += 1;
      }
    }
  }
}


void generateSubset(int siftedlen, int key_size, uint8_t* sifted_key, const uint8_t* indsecretmsg, uint8_t* subset_key) {
  int j = 0;
  for (int i = 0; i < siftedlen && j < key_size*8; i++) {
    for (int k = 0; k < 8 ; k++) {
      if (bitRead(indsecretmsg[i], k)) {
        bitWrite(subset_key[j/8],j%8,bitRead(sifted_key[i], k));
        j += 1;
      }
    }
  }
}

void errorCorrectData(int siftedlen, int key_size, uint8_t* sifted_key, const uint8_t* indsecretmsg, uint8_t* subset_key ) {
  int j = 0;
  for (int i = 0; i < siftedlen && j < key_size*8; i++) {
    for (int k = 0; k < 8 ; k++) {
      if (!(bitRead(indsecretmsg[i], k))) {
        bitWrite(subset_key[j/8],j%8,bitRead(sifted_key[i], k));
        j += 1;
      }
    }
  }
}

void shuffleData(uint8_t* data, int* indices, int size) {
  for (int i = size - 1; i > 0; i--) {
    int j = random(0, i + 1);
    uint8_t tempData = data[i];
    data[i] = data[j];
    data[j] = tempData;

    int tempIndex = indices[i];
    indices[i] = indices[j];
    indices[j] = tempIndex;
  }
}

String uint8ArrayToHexString(uint8_t* array, size_t length) {
  String hexString = "";
  for (size_t i = 0; i < length; i++) {
    if (array[i] < 0x10) {
      hexString += "0";
    }
    hexString += String(array[i], HEX);
  }
  return hexString;
}

void printUint8ArrayAsBinary(uint8_t* array, size_t length) {
  for (size_t i = 0; i < length; i++) {
    // Print each byte as a binary string
    for (int bit = 7; bit >= 0; bit--) {
      Serial.print((array[i] >> bit) & 1);
    }
    Serial.print(" ");  // Add space for readability
  }
  Serial.println();  // New line after printing the array
}

void cleanData(char* data, int length) {
  int j = 0;
  for (int i = 0; i < length; i++) {
    if (data[i] != '\r' && data[i] != '\n') {
      data[j++] = data[i];
    }
  }
  data[j] = '\0';
}

int countones( uint8_t* matchingIndices, int msg_size) {
  int count = 0;
  for (int i = 0; i < msg_size; i++) {
    for (int j = 0; j < 8; j++) {
    if (bitRead(matchingIndices[i],j)) {
      count++;
    }
  }
  }
  return count;
}

void clearSerialBuffer(HardwareSerial& serialPort) {
  while (serialPort.available() > 0) {
    serialPort.read();  // Read and discard data until buffer is empty
  }
}

void writeByteArray(int len, uint8_t* buff)
{
  int index = 0;
  while(len - index > 256)
  {
    int i = Serial2.write(buff + index, 256);
    Serial2.flush();

    Serial.println(i);

    index += 256;

    while(!Serial2.available())
      continue;
    
    Serial2.read();
  }

  Serial2.write(buff + index, len-index);
}

void readByteArray(int len, uint8_t* buff)
{
    int totalReceivedBytes = 0;

    while(len - totalReceivedBytes > 256)
    {
        totalReceivedBytes += Serial2.readBytes(buff + totalReceivedBytes, 256);
        Serial2.print('n');
    }

    totalReceivedBytes += Serial2.readBytes(buff + totalReceivedBytes, len - totalReceivedBytes);
}

uint8_t StrToHex(char c)
{
  switch(c)
  {
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    case 'A':
      return 10;
    case 'B':
      return 11;
    case 'C':
      return 12;
    case 'D':
      return 13;
    case 'E':
      return 14;
    case 'F':
      return 15;
  }
}

void printUintArray(uint8_t* arr,int len)
{
    for(int i = 0; i < len; i++)
    {
      Serial.print(" ");
      Serial.print(arr[i]);
    }
    Serial.println();
}