#include "PPB_Privacy_Amplification.h"


// Destructor to free allocated memory
PPB_Privacy_Amplification::~PPB_Privacy_Amplification() {
    if (toeplitzMatrix) {
        freeToeplitzMatrix(R);
    }
    if (key) {
        free(key);
    }
}

// Calculate the key rate based on key length, QBER, and leaked bits
int PPB_Privacy_Amplification::calcKeyRate(int keyLength, double qber, int leakedBits) {
    double entropyQber = -qber * (log(qber) / log(2)) - (1 - qber) * (log(1 - qber) / log(2));
    int result = static_cast<int>(floor(keyLength * (1 - 2 * entropyQber))) - leakedBits;

    // Ensure result is non-negative and divisible by 8
    if (result < 0) {
        return 0; // Return 0 if the result is negative
    }
    
    // Check if result is greater than or equal to 8 before adjusting
    if (result >= 8) {
        result &= ~0x7; // Clear the last three bits to make it divisible by 8
    }

    return result;
}

// Create a Toeplitz matrix for privacy amplification
void PPB_Privacy_Amplification::createToeplitzMatrix(int rows, int cols) {
    toeplitzMatrix = (uint8_t*) malloc((rows + cols)* sizeof(uint8_t));


    // Initialize the first column and first row with random bits
    for (int i = 0; i < rows + cols; i++) {
        toeplitzMatrix[i] = random(0, 2);
    }
       Serial.println("Bob : Toeplitz matrix .. ");
        for (int i = 0; i < rows + cols; i++) {
        Serial.print(toeplitzMatrix[i] );
    }
    // Perform amplification immediately after creating the matrix 
    performAmplification();
}

// Free the allocated memory for the Toeplitz matrix
void PPB_Privacy_Amplification::freeToeplitzMatrix(int rows) {
    delete[] toeplitzMatrix;
}

// Prepare data for privacy amplification
void PPB_Privacy_Amplification::prepareData(uint8_t* mkey, unsigned int length, unsigned int leakedBits, float qber) {
    this->mykey = mkey;
    this->BLOCK_SIZE = length;
    this->leakedBits = leakedBits;
    this->qber = qber;
    int keyLength = length * 8; // Convert length to bits
    this->R = calcKeyRate(keyLength, qber, leakedBits);

    if (R > 0) {
        randomSeedValue = random(0, 255); // Generate a random seed
Serial.println("Attempting to send random seed...");
if (sendRandomSeed()) {
    Serial.println("Random seed sent successfully.");
    randomSeed(randomSeedValue);
    Serial.println("Bob :Random seed initialized.");
    createToeplitzMatrix(R, keyLength);
    Serial.println("Bob: Toeplitz matrix created.");
} else {
    Serial.println("Failed to send random seed.");
}
    } else {
        Serial.println("Too much information leaked, can't proceed with encryption.");
    }
}

// Perform privacy amplification
bool PPB_Privacy_Amplification::performAmplification() {
    if (R <= 0) {
        return false;
    }

    generateFinalKey();
    return true;
}

// Generate the final key using the Toeplitz matrix
void PPB_Privacy_Amplification::generateFinalKey() {
    // Allocate memory for the key array
    key = (uint8_t*)malloc((R+7)/8* sizeof(uint8_t));
    memset(key, 0, (R+7)/8); // Initialize the key array to zero



    Serial.println("Bob : R ");
    Serial.println(R);
    // Compute the final key
    Serial.println("Bob : generating key");
    for (int i = 0; i < R; i++) {
        uint8_t sum = 0;
        for (int j = 0; j < BLOCK_SIZE * 8; j++) {
            int byteIndex = j / 8;
            int bitIndex = j % 8;
            uint8_t bit = (mykey[byteIndex] >> bitIndex) & 1;

            sum ^= (toeplitzMatrix[j + R - i] & bit);
            
        }
        Serial.print(sum);
        key[i / 8] |= (sum << (i % 8));
    }

    // Print the final key for debugging
    Serial.print("Bob :Final Key: ");
    for (int i = 0; i < (R + 7) / 8; i++) {
        Serial.print(key[i], BIN);
        Serial.print(" ");
    }
    Serial.println();
}

// Send the random seed to Alice
bool PPB_Privacy_Amplification::sendRandomSeed() { 
    char buffer[11] = {0}; 
    sprintf(buffer, "%u", randomSeedValue); 

    // Attempt to send the random seed
    if (Serial2.println(buffer) > 0) { // Check if data was sent successfully
        Serial2.flush(); // Ensure all data is sent
        delay(100); // Delay to allow for proper transmission
        Serial.print("Sending randomSeedValue: ");
        Serial.println(buffer);
        return true; // Indicate success
    } else {
        Serial.println("Failed to send randomSeedValue.");
        return false; // Indicate failure
    }
}

// Get the final key
uint8_t* PPB_Privacy_Amplification::getKey() {
    return key;
}

// Get the key rate
int PPB_Privacy_Amplification::getKeyRate() const {
    return R;
}