#ifndef PPB_PRIVACY_AMPLIFICATION_H
#define PPB_PRIVACY_AMPLIFICATION_H

#include <Arduino.h>

class PPB_Privacy_Amplification {
public:
    ~PPB_Privacy_Amplification(); // Destructor to free allocated memory
    void prepareData(uint8_t* key,unsigned int length, unsigned int leakedBits, float qber);
    uint8_t* getKey(); // Method to access the final key
    int getKeyRate() const; // Method to access the key rate

private:
    int calcKeyRate(int keyLength, double qber, int leakedBits);
    void createToeplitzMatrix(int rows, int cols);
    bool performAmplification();
    void generateFinalKey();
    bool sendRandomSeed();
    void freeToeplitzMatrix(int rows);


    int BLOCK_SIZE;
    int R; // Key rate
    int leakedBits;
    double qber;
    uint8_t* mykey;
    uint8_t* toeplitzMatrix; // Pointer to a 2D array for the Toeplitz matrix
    uint8_t* key; // Array to store the final key
    unsigned int randomSeedValue;
};

#endif // PPB_PRIVACY_AMPLIFICATION_H