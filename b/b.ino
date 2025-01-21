#include <Arduino.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>


#define MAX_BASES_LENGTH 128

uint8_t* subset_data;
uint8_t* bsubset;
uint8_t* aliceBases;
uint8_t* bobBases;
uint8_t* bobm;
uint8_t* matchingIndices;
uint8_t* bcascade;
uint8_t* errorcorrectb;
char* receivedData;
unsigned int msg_size = 0;
unsigned int key_size = 0;
uint8_t* sifted_key;
uint8_t* indsecretmsg;
unsigned long startTime = millis();
int totalBytesReceived = 0;
int subset_size = 0;
unsigned int siftedlen = 0;
unsigned int subsetlen = 0;
float errate = 0.0;
int ercount = 0;
float keyUsage = 0.4;
char error_rate[6];
char* mihex;
char* sihex;
int receivedBytes = 0;
int blocks[100][100];

void cleanData(char* data, int length);
void generateSubset(int siftedlen, int key_size, uint8_t* sifted_key, const uint8_t* indsecretmsg, uint8_t* subset_key);
void errorCorrectData(int siftedlen, int key_size, uint8_t* sifted_key, const uint8_t* indsecretmsg, uint8_t* subset_key);
void calcmi(int msg_size, const uint8_t* aliceBases, const uint8_t* bobBases, uint8_t* matchingIndices);
uint8_t* hexToBinary(const char* hex, int n);
int* convertCharArrayToIntArray(char* charArray, int size);
char* convertIntArrayToCharArray(int* intArray, int size);
char* extractData(const char* siftedKey, int sz, int* resultSize);
int* shuffle_key(int* key, int* indices, int size);
int calculate_block_size(int iteration, float error_rate);
int** divideintoblocks(int* key, int key_size, int block_size, int* num_blocks);
int getAliceParity(int blockIndex);
int* shuffleit(int* siintArray, int* bobSiftedKey, int size);

void setup() {
  Serial.begin(230400);
  Serial2.begin(230400);

  Serial.println("ready");

  receivedData = (char*)malloc(1024 * sizeof(char));
}

void loop() {
  if (Serial.available() > 0) {
    
    int receivedBytes = Serial.readBytesUntil('\0', (char*)receivedData, 1024);
    receivedData[receivedBytes] = '\0';  // Null-terminate the string

    unsigned long elapsedTime = millis() - startTime;

    Serial.print("\nBob: Received from PC: ");
    Serial.println(receivedData);

    sscanf(receivedData, "%u;", &msg_size);
    Serial.print("Message Size: ");
    Serial.println(msg_size);

    char* bmHex = (char*)malloc((msg_size) * sizeof(char));
    char* bBasesHex = (char*)malloc((msg_size) * sizeof(char));
    sscanf(receivedData, "%*u;%[^;];%s", bmHex, bBasesHex);
    msg_size /= 8;

    aliceBases = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));
    matchingIndices = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));
    bobBases = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));
    bobm = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));
    bobm = hexToBinary(bmHex, msg_size);
    bobBases = hexToBinary(bBasesHex, msg_size);

    // Debug: Print bobm and bobBases in binary
    Serial.println("Bob Message (Binary):");
    printUintArray(bobm, msg_size);
    Serial.println("Bob Bases (Binary):");
    printUintArray(bobBases, msg_size);

    Serial.print("\nBob: Time taken to receive from PC: ");
    Serial.print(elapsedTime);
    Serial.println(" ms");
    delay(100);

    // Handshake to receive alicebases
    Serial2.println("READY");
    Serial2.flush();
    Serial.print("Bob: Sent READY\n");
    delay(100);

    bool aliceBasesReceived = false;
    bool subsetReceived = false;

    while (!aliceBasesReceived || !subsetReceived) {
      if (Serial2.available() > 0) {
        if (!aliceBasesReceived) {
          unsigned long timeoutStart = millis();
          //totalBytesReceived = 0;
          /*while (totalBytesReceived < msg_size) {
            if (Serial2.available() > 0) {
              receivedBytes = Serial2.readBytes(aliceBases + totalBytesReceived, msg_size);
              totalBytesReceived += receivedBytes;
            }
            if (millis() - timeoutStart > 10000) {
              Serial.println("Bob: Timeout waiting for Alice's bases.");
              return;
            }
            delay(50);
          }*/

          //wait for available data
          readByteArray(msg_size, aliceBases);

          //cleanData((char*)aliceBases, totalBytesReceived);
          elapsedTime = millis() - startTime;

          // Debug: Print aliceBases in binary
          Serial.print("\nBob: Received Alice's Bases (bytes received: ");
          Serial.print(totalBytesReceived);
          Serial.println("): ");
          printUintArray(aliceBases, msg_size);
          Serial.print("\nBob: Time taken to receive Alice's bases: ");
          Serial.print(elapsedTime);
          Serial.println(" ms");

          //uint8_t* matchingIndices = (uint8_t*)malloc(msg_size * sizeof(char));
          calcmi(msg_size, aliceBases, bobBases, matchingIndices);

          // debug
          Serial.print("\nBob: Matching Indices: ");
          printUintArray(matchingIndices, msg_size);

          Serial.println();
          delay(200);
          Serial.println("Sending ReadyM");
          Serial2.print("READYM\n");
          Serial2.flush();
          bool ready = false;
          while (!ready) {
            String received = Serial2.readStringUntil('\n');
            Serial.println(received);
            if (received != "ACKM") {
              Serial.println("Bob: ACKM not received ");
            } else {

              writeByteArray(msg_size, matchingIndices);

              delay(100);
              Serial.print("Message Size: ");
              Serial.println(msg_size);
              ready = true;
            }
          }
          delay(200);
          elapsedTime = millis() - startTime;
          Serial.print("\nBob: Time taken to send matching indices: ");
          Serial.print(elapsedTime);
          Serial.println(" ms");
          
          delay(100);
          aliceBasesReceived = true;
        }
        int siftedlen = countones(matchingIndices, msg_size)/8; // subtract the modulo to make sure the size is multiple of eight
        Serial.println(siftedlen);
        int subsetlen = round(keyUsage * siftedlen);
        sifted_key =(uint8_t*)malloc(siftedlen * sizeof(uint8_t*));
        subset_data = (uint8_t*)malloc((subsetlen) * sizeof(uint8_t));
        bsubset = (uint8_t*)malloc((subsetlen) * sizeof(uint8_t));
        errorcorrectb = (uint8_t*)malloc((key_size) * sizeof(uint8_t));
        indsecretmsg = (uint8_t*)malloc((siftedlen) * sizeof(uint8_t));
        key_size = siftedlen - subsetlen;
        siftData(msg_size, siftedlen, bobm, matchingIndices,sifted_key);
        Serial.println(siftedlen);
        Serial.println(key_size);
        Serial.println(subsetlen);
        Serial.println("\nBob: Sifted data ");
        printUintArray(sifted_key, siftedlen);
        elapsedTime = millis() - startTime;
        Serial.print("\nBob: Time taken to sift: ");
        Serial.println(elapsedTime);
        if (aliceBasesReceived && !subsetReceived) {

          unsigned long timeoutStart = millis();
          elapsedTime = millis() - startTime;
          Serial.print("\nBob: Time taken to send READY_FOR_subset_data: ");
          Serial.print(elapsedTime);
          Serial.print(" ms\n");
              Serial2.print("READY_FOR_subset_data\n");
              Serial.println("Bob: Sent READY_FOR_subset_data");
              Serial.println(subsetlen);
              Serial2.flush();
          totalBytesReceived = 0;
          readByteArray(subsetlen, subset_data);

          Serial.println("Bob: Received subset_data");
          elapsedTime = millis() - startTime;
          Serial.print("Bob: Received subset_data Key from Alice: ");
          printUintArray(subset_data, subsetlen);
          Serial.print("Bob: Time taken to receive subset_data key: ");
          Serial.print(elapsedTime);
          Serial.print(" ms\n");





          timeoutStart = millis();
          elapsedTime = millis() - startTime;
          Serial.print("\nBob: Time taken to send READY_FOR_subset_ind: ");
          Serial.print(elapsedTime);
          Serial.print(" ms\n");
              Serial2.print("READY_FOR_subset_ind\n");
              Serial.println("Bob: Sent READY_FOR_subset_ind");
              Serial.println(siftedlen);
              Serial2.flush();
          readByteArray(siftedlen, indsecretmsg);

          Serial.println("Bob: Received subset_ind");
          elapsedTime = millis() - startTime;
          Serial.print("Bob: Received subsetindeces from Alice: ");
          printUintArray(indsecretmsg, siftedlen);

          Serial.print("Bob: Time taken to receive subset_ind: ");
          Serial.print(elapsedTime);
          Serial.print(" ms\n");
          subsetReceived = true;
        }
        Serial.println("Bob: Beginning parameter estimation: ");
        generateSubset(siftedlen, subsetlen, sifted_key, indsecretmsg, bsubset);
        errorCorrectData(siftedlen, key_size, sifted_key, indsecretmsg, errorcorrectb);
        // debug
            Serial.println("Bob : bobs subset Data:");
    if (bsubset) {
      printUintArray(bsubset, subsetlen);
      Serial.println(subsetlen);
    } else {
      Serial.println("Bob : Error: subset_data is NULL.");
    }

    Serial.println("Bob : Error Correct Data:");
    if (errorcorrectb) {
      printUintArray(errorcorrectb, key_size);
      Serial.println();
    } else {
      Serial.println("Bob : Error: errorcorrecta is NULL.");
    }

    errate = calculateErrorRate(bsubset, subset_data, subsetlen);
    Serial.print("Error Rate: ");
    Serial.println(errate);





      }
    }

    free(bobBases);
    free(aliceBases);
    free(subset_data);
    free(matchingIndices);
    free(bsubset);
    free(errorcorrectb);
    free(receivedData);
    Serial.flush();
    Serial2.flush();
  }
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


void calcmi(int msg_size, const uint8_t* aliceBases, const uint8_t* bobBases, uint8_t* mi) {
  //char* mi = (char*)malloc(msg_size * sizeof(char));
  for (int i = 0; i < msg_size; i++) {  
      mi[i] = ~ aliceBases[i] ^ bobBases[i];
  }
  //return mi;
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

int* convertCharArrayToIntArray(char* charArray, int size) {
  int* intArray = (int*)malloc(size * sizeof(int));
  for (int i = 0; i < size; i++) {
    if (charArray[i] == '0' || charArray[i] == '1') {
      intArray[i] = charArray[i] - '0';
    } else {
      intArray[i] = -1;
    }
  }
  return intArray;
}

char* convertIntArrayToCharArray(int* intArray, int size) {
  char* charArray = (char*)malloc((size + 1) * sizeof(char));
  for (int i = 0; i < size; i++) {
    if (intArray[i] == 0)
      charArray[i] = '0';
    else
      charArray[i] = intArray[i] + '0';
  }
  charArray[size] = '\0';
  return charArray;
}

char* extractData(const char* siftedKey, int sz, int* resultSize) {
  int count = 0;
  for (int i = 0; i < sz; i++) {
    if (siftedKey[i] == '0' || siftedKey[i] == '1') {
      count++;
    }
  }

  char* resultArray = (char*)malloc(count * sizeof(char));
  int index = 0;
  for (int i = 0; i < sz; i++) {
    if (siftedKey[i] == '0' || siftedKey[i] == '1') {
      resultArray[index++] = siftedKey[i];
    }
  }

  *resultSize = count;
  return resultArray;
}

int* shuffle_key(int* key, int* indices, int size) {
  for (int i = 0; i < size; i++) {
    indices[i] = i;
  }

  for (int i = size - 1; i > 0; i--) {
    int j = random(0, i + 1);

    int tempKey = key[i];
    key[i] = key[j];
    key[j] = tempKey;

    int tempIndex = indices[i];
    indices[i] = indices[j];
    indices[j] = tempIndex;
  }

  return key;
}

int calculate_block_size(int iteration, float error_rate) {
  int k = 0;
  if (iteration == 1) {
    k = max(1, (int)ceil(0.73 / error_rate));
  } else {
    k = (1 << (iteration - 1)) * max(1, (int)ceil(0.73 / error_rate));
  }
  return k;
}

int** divideintoblocks(int* key, int key_size, int block_size, int* num_blocks) {
  *num_blocks = (key_size + block_size - 1) / block_size;
  int** blocks = (int**)malloc(*num_blocks * sizeof(int*));
  for (int i = 0; i < *num_blocks; i++) {
    blocks[i] = (int*)malloc(block_size * sizeof(int));  // Allocate memory for each block
    for (int j = 0; j < block_size; j++) {
      int index = i * block_size + j;
      if (index < key_size) {
        blocks[i][j] = key[index];  // Assume key contains '0' and '1' as characters
      } else {
        blocks[i][j] = -1;  // Fill with -1 if out of bounds
      }
    }
  }
  return blocks;
}

int getAliceParity(int blockIndex) {
  Serial2.print(blockIndex);
  Serial2.flush();
  Serial2.println();

  while (true) {
    if (Serial2.available() > 0) {
      char aliceParityChar = Serial2.read();
      if (aliceParityChar == '0' || aliceParityChar == '1') {
        return aliceParityChar - '0';
      }
    }
    delay(1);
  }
}

int* shuffleit(int* siintArray, int* bobSiftedKey, int size) {
  static int shuffledData[100];  // Adjust size as needed
  for (int i = 0; i < size; i++) {
    shuffledData[i] = bobSiftedKey[siintArray[i]];  // Rearranging based on siintArray
  }
  return shuffledData;
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

void clearSerialBuffer(HardwareSerial &serialPort) {
  while (serialPort.available() > 0) {
    serialPort.read();  // Read and discard data until buffer is empty
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


float calculateErrorRate(uint8_t* bsubset, uint8_t* subset_data, int subsetlen) {
    if (!bsubset || !subset_data || subsetlen == 0) {
        Serial.println("Error: Invalid input arrays or length.");
        return -1.0;
    }

    int errorCount = 0;

    for (int i = 0; i < subsetlen; i++) {
      for (int j = 0; j < 8; j++) {
        if (bitRead(bsubset[i], j) != bitRead(subset_data[i], j)) {
            errorCount++;
        }
     }
    }

    float errate = static_cast<float>(errorCount) / static_cast<float>(subsetlen *8);

    Serial.print("error count:");
    Serial.println(errorCount);

    Serial.print("subset length:");
    Serial.println(subsetlen);

    return errate;
}

void writeByteArray(int len, uint8_t* buff)
{
  int index = 0;
  while(len - index > 256)
  {
    Serial2.write(buff + index, 256);
    Serial2.flush();

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
        Serial.print("received bytes");
        Serial.println(totalBytesReceived);
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