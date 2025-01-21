#include "PPA_Privacy_Amplification.h"


// Destructor to free allocated memory
PPA_Privacy_Amplification::~PPA_Privacy_Amplification() {
    if (toeplitzMatrix) {
        freeToeplitzMatrix(R);
    }
    if (key) {
        free(key);
    }
}

// Calculate the key rate based on key length, QBER, and leaked bits
int PPA_Privacy_Amplification::calcKeyRate(int keyLength, double qber, int leakedBits) {
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
void PPA_Privacy_Amplification::createToeplitzMatrix(int rows, int cols) {
    toeplitzMatrix = (uint8_t*) malloc((rows + cols)* sizeof(uint8_t));


    // Initialize the first column and first row with random bits
    for (int i = 0; i < rows + cols; i++) {
        toeplitzMatrix[i] = random(0, 2);
    }
       Serial.println("Alice: Toeplitz matrix .. ");
        for (int i = 0; i < rows + cols; i++) {
        Serial.print(toeplitzMatrix[i] );
    }
    // Perform amplification immediately after creating the matrix 
    performAmplification();
}

// Free the allocated memory for the Toeplitz matrix
void PPA_Privacy_Amplification::freeToeplitzMatrix(int rows) {
    delete[] toeplitzMatrix;
}

// Prepare data for privacy amplification
void PPA_Privacy_Amplification::prepareData(uint8_t* mkey, unsigned int length, unsigned int leakedBits, float qber) {
    this->mykey = mkey;
    this->BLOCK_SIZE = length;
    this->leakedBits = leakedBits;
    this->qber = qber;
    int keyLength = length * 8; // Convert length to bits
    this->R = calcKeyRate(keyLength, qber, leakedBits);

    if (R > 0) {
        getRandomSeed(); // Get the random seed from Bob
        randomSeed(randomSeedValue);
        Serial.println("Alice :Random seed initialized.");
        createToeplitzMatrix(R, keyLength);
        Serial.println("Alice: Toeplitz matrix created.");
    } else {
        Serial.println("Too much information leaked, can't proceed with encryption.");
    }
}

// Perform privacy amplification
bool PPA_Privacy_Amplification::performAmplification() {
    if (R <= 0) {
        return false;
    }

    generateFinalKey();
    return true;
}

// Generate the final key using the Toeplitz matrix
void PPA_Privacy_Amplification::generateFinalKey() {
    // Allocate memory for the key array
    key = (uint8_t*)malloc((R+7)/8* sizeof(uint8_t));
    memset(key, 0, (R+7)/8); // Initialize the key array to zero

    Serial.println("Alice : R ");
    Serial.println(R);
    // Compute the final keyf
    Serial.println("Alice : generating key");
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
    Serial.print("Alice :Final Key: ");
    for (int i = 0; i < (R + 7) / 8; i++) {
        Serial.print(key[i], BIN);
        Serial.print(" ");
    }
    Serial.println();
}

// Get the random seed from Bob
void PPA_Privacy_Amplification::getRandomSeed() {
 
        while (!Serial2.available()) {           
        }
        String receivedStr = Serial2.readStringUntil('\n');

    randomSeedValue = extractUnsignedInt(receivedStr);

    Serial.print("Converted seed value: ");
    Serial.println(randomSeedValue);
 
}

// Get the final key
uint8_t* PPA_Privacy_Amplification::getKey() {
    return key;
}

// Get the key rate
int PPA_Privacy_Amplification::getKeyRate() const {
    return R;
}

unsigned int PPA_Privacy_Amplification::extractUnsignedInt(const String &str) {
    String numericStr = ""; 

    for (int i = 0; i < str.length(); i++) {
        char c = str.charAt(i);

        if (c >= '0' && c <= '9') {
            numericStr += c; // Append digit to numericStr
        }
    }

    return numericStr.length() > 0 ? numericStr.toInt() : 0; // Return 0 if no digits found
}