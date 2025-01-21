#ifndef PPB_Error_Correction_h
#define PPB_Error_Correction_h

#include "Arduino.h"

class PPB_Error_Correction{

    public:

    PPB_Error_Correction();
    void itterate_block(byte level);
    int binary_search(int sta, int sto);
    boolean getAlicePairity(int sta, int sto);
    boolean getBobPairity(int sta, int sto);
    bool proceedErrorCorrection(unsigned int& aleak);
    void printPairities();
    void prepare_Data(uint8_t* key, int length);
    void calc_permutations();
    void printBitArray(byte b[]);
    void prepare_blocks();
    void shuffle_array(int *array,int seed);

    private:
    void printIntArray(int b[], int len);

    int BLOCK_SIZE;

    int* block0; 
    int* block1;
    int* block2;
    int* block3;

    int number_blocks[4];
    int last_block_size[4];
    int block_sizes[4];

    boolean* pair_bob;
    boolean* pair_alice; 

    byte state = 0;

   
    long time_stemp;

    uint8_t* mykey;
 
    int* current_block_indices;
};

#endif
