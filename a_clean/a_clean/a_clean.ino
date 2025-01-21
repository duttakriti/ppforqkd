#include <Arduino.h>
#include "src\PPA_Error_Correction.h"
#include "src\PPA_Privacy_Amplification.h"

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
unsigned int aleak  = 0;
unsigned int bleak  = 0;
bool readym = false;
bool mircvd = false;
float keyUsage = 0.4;
PPA_Error_Correction pp_ec;
PPA_Privacy_Amplification pp_pa;

uint8_t* hexToBinary(const char* hex, int n);
void siftData(int msg_size, int siftedlen, uint8_t* message, uint8_t* indices);
void performErrorCorrection(uint8_t* errorcorrecta, int msg_size);
void shuffleData(uint8_t* data, int* indices, int size);




void setup() {
  // put your setup code here, to run once:
  Serial.begin(230400);
  Serial2.begin(230400);

  receivedData = (char*)malloc(4096 * 8 * sizeof(char));
  if (receivedData == NULL) {
    Serial.println("Error: Memory allocation failed for receivedData.");
    return;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) {
    
    int receivedBytes = Serial.readBytesUntil('\0', receivedData, 4096 * 8 );
    receivedData[receivedBytes] = '\0';
    
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
    free(receivedData);



    matchingIndices = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));
    errorcorrecta = (uint8_t*)malloc((msg_size) * sizeof(uint8_t));

    // write Alice Bases
    writeByteArray(msg_size, aliceBases);
    // read matched indices
    readByteArray(msg_size, matchingIndices);

    siftedlen = countones(matchingIndices, msg_size)/8;
    subsetlen = round(keyUsage * siftedlen);
    indsecretmsg =  (uint8_t*)malloc(siftedlen * sizeof(uint8_t*));

    for(int i = 0; i < siftedlen; i++)
      indsecretmsg[i] = 0;

    sifted_key =(uint8_t*)malloc(siftedlen * sizeof(uint8_t*));
    key_size = siftedlen - subsetlen;
    siftData(msg_size, siftedlen, alicemessage, matchingIndices, sifted_key);
    errorcorrecta =(uint8_t*)malloc(key_size * sizeof(uint8_t*));
    subset_data =(uint8_t*)malloc(subsetlen * sizeof(uint8_t*));

    generaterandomindeces(siftedlen, subsetlen,indsecretmsg);
    generateSubset(siftedlen, subsetlen, sifted_key, indsecretmsg, subset_data);
    errorCorrectData(siftedlen, key_size, sifted_key, indsecretmsg,errorcorrecta);

    writeByteArray(subsetlen, subset_data);

    writeByteArray(siftedlen, indsecretmsg);
    Serial.println("waiting or error rate");
    float errate = receiveFloat();
    Serial.print("Received error rate: ");
    Serial.println(errate);

    //Serial.println("Alice : before error correction");
    //printUintArray(errorcorrecta, key_size);

    free(sifted_key);
    free(subset_data);
    free(indsecretmsg);
    free(matchingIndices);
    free(aliceBases);
    free(alicemessage);

    pp_ec.prepare_Data(errorcorrecta, key_size*8);
    pp_ec.calc_permutations();

    while(pp_ec.proceedErrorCorrection())
      continue;

    Serial.println("Alice :error correction finished");
    printUintArray(errorcorrecta, key_size);
    bleak = receiveUnsignedInt();
    Serial.print("Received bleak: ");
    Serial.println(bleak);

    pp_pa.prepareData(errorcorrecta, key_size, bleak, errate);
    uint8_t* finalKey = pp_pa.getKey();
    int R = pp_pa.getKeyRate(); // Assume you have a way to get the key rate R
    int finalKeyLength = (R + 7) / 8; // Calculate the length in bytes
    Serial.print("Alice :Final Key:");
    printUintArray(finalKey, finalKeyLength);


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

void writeByteArray(int len, uint8_t* buff)
{
  int index = 0;
  while(len - index > 256)
  {
    int i = Serial2.write(buff + index, 256);
    Serial2.flush();

    //Serial.println(i);

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

float receiveFloat() {
    union {
        float floatValue;
        uint8_t bytes[4];
    } data;

    // Read each byte of the float
    for (int i = 0; i < 4; i++) {
        while (!Serial2.available()) {
            // Wait for the byte to be available
        }
        data.bytes[i] = Serial2.read();
    }

    return data.floatValue;
}

unsigned int receiveUnsignedInt() {

        while (!Serial2.available()) {           
        }
        String receivedStr = Serial2.readStringUntil('\n');

    unsigned int value = extractUnsignedInt(receivedStr);

    Serial.print("Converted value: ");
    Serial.println(value);

    return value;
}

unsigned int extractUnsignedInt(const String &str) {
    String numericStr = ""; 

    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);

        if (c >= '0' && c <= '9') {
            numericStr += c; // Append digit to numericStr
        }
    }

    return numericStr.length() > 0 ? numericStr.toInt() : 0; // Return 0 if no digits found
}
