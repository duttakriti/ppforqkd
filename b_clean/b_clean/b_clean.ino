#include <Arduino.h>
#include "src\PPB_Error_Correction.h"
#include "src\PPB_Privacy_Amplification.h"
uint8_t* subset_data;
uint8_t* bsubset;
uint8_t* aliceBases;
uint8_t* bobBases;
uint8_t* bobm;
uint8_t* matchingIndices;
uint8_t* errorcorrectb;
char* receivedData;
unsigned int msg_size = 0;
unsigned int key_size = 0;
uint8_t* sifted_key;
uint8_t* indsecretmsg;
int subset_size = 0;
unsigned int siftedlen = 0;
unsigned int subsetlen = 0;
unsigned int aleak  = 0;
unsigned int bleak  = 0;
float errate = 0.0;
float keyUsage = 0.4;
int receivedBytes = 0;
PPB_Error_Correction pp_ec;
PPB_Privacy_Amplification pp_pb;

void cleanData(char* data, int length);
void generateSubset(int siftedlen, int key_size, uint8_t* sifted_key, const uint8_t* indsecretmsg, uint8_t* subset_key);
void errorCorrectData(int siftedlen, int key_size, uint8_t* sifted_key, const uint8_t* indsecretmsg, uint8_t* subset_key);
void calcmi(int msg_size, const uint8_t* aliceBases, const uint8_t* bobBases, uint8_t* matchingIndices);
int getAliceParity(int blockIndex);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(230400);
  Serial2.begin(230400);

  receivedData = (char*)malloc(4096  * 8 * sizeof(char));
}

void loop() {
  // put your main code here, to run repeatedly:
   if (Serial.available() > 0) {
    
    

    int receivedBytes = Serial.readBytesUntil('\0', (char*)receivedData, 4096 * 8 );
    receivedData[receivedBytes] = '\0';  // Null-terminate the string


    sscanf(receivedData, "%u;", &msg_size);

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

    free(bmHex);
    free(bBasesHex);
    free(receivedData);

    while(!Serial2.available())  // this is to make sure that Alice is ready, when starting the timer
      continue;

    long time_start = millis(); 

    readByteArray(msg_size, aliceBases);

    
    calcmi(msg_size, aliceBases, bobBases, matchingIndices);
    writeByteArray(msg_size, matchingIndices);

    int siftedlen = countones(matchingIndices, msg_size)/8; // subtract the modulo to make sure the size is multiple of eight

    int subsetlen = round(keyUsage * siftedlen);
    sifted_key =(uint8_t*)malloc(siftedlen * sizeof(uint8_t*));
    subset_data = (uint8_t*)malloc((subsetlen) * sizeof(uint8_t));
    bsubset = (uint8_t*)malloc((subsetlen) * sizeof(uint8_t));
    errorcorrectb = (uint8_t*)malloc((key_size) * sizeof(uint8_t));
    indsecretmsg = (uint8_t*)malloc((siftedlen) * sizeof(uint8_t));
    key_size = siftedlen - subsetlen;

    siftData(msg_size, siftedlen, bobm, matchingIndices,sifted_key);
    readByteArray(subsetlen, subset_data);
    readByteArray(siftedlen, indsecretmsg);
    generateSubset(siftedlen, subsetlen, sifted_key, indsecretmsg, bsubset);
    errorCorrectData(siftedlen, key_size, sifted_key, indsecretmsg, errorcorrectb);
    errate = calculateErrorRate(bsubset, subset_data, subsetlen);

    Serial.println("Bob : error rate:");
    Serial.println(errate);
    sendFloat(errate);

    //Serial.println("Bob : before error correction");
    //printUintArray(errorcorrectb, key_size);

    free(bobm);

    //free(subset_data);
    free(bobBases);
    free(aliceBases);
    free(matchingIndices);
    //free(bsubset);
    //free(indsecretmsg);
    //free(sifted_key);
  

    pp_ec.prepare_Data(errorcorrectb, key_size*8);
    while(!Serial2.available())
     continue;   
    pp_ec.proceedErrorCorrection(aleak);
    Serial.println(aleak);
    bleak = 30 + aleak;
    Serial.println(bleak);    
    sendUnsignedInt(bleak);
    Serial.println("Bob : error correction finished");
    printUintArray(errorcorrectb, key_size);
    pp_pb.prepareData(errorcorrectb, key_size, bleak, errate);
    uint8_t* finalKey = pp_pb.getKey();
    int R = pp_pb.getKeyRate(); // Assume you have a way to get the key rate R
    int finalKeyLength = (R + 7) / 8; // Calculate the length in bytes
    Serial.print("Bob: Final Key:");
    printUintArray(finalKey, finalKeyLength);
    long time = millis() - time_start;
    Serial.print("time: ");
    Serial.println(time);
   }
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

    //Serial.print("error count:");
    //Serial.println(errorCount);

    //Serial.print("subset length:");
    //Serial.println(subsetlen);

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

void sendFloat(float value) {
    union {
        float floatValue;
        uint8_t bytes[4];
    } data;

    data.floatValue = value;

    // Send each byte of the float
    for (int i = 0; i < 4; i++) {
        Serial2.write(data.bytes[i]);
    }
    Serial2.flush();
}

void sendUnsignedInt(unsigned int value) {
char buffer[7] = {0}; 
    sprintf(buffer, "%u", value); 
   
    Serial2.println(buffer);
    Serial2.flush();
    Serial.print("Sending string: ");
    Serial.println(buffer);
}
