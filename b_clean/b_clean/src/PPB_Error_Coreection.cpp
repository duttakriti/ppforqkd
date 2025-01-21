#include "PPB_Error_Correction.h"

int aliceParityCallCount = 0;

// ------- variables for error correction --------
PPB_Error_Correction::PPB_Error_Correction()
{

}

/* function that executes one level of the cascade algorithm */
void PPB_Error_Correction::itterate_block(byte level)
{
  Serial.print("start itteration");
  Serial.println(level);

  current_block_indices = (int*)malloc((BLOCK_SIZE) * sizeof(int));
  boolean flipped = false;
  calc_permutations();
  //Serial.println("itterate block");
  for(int n = 0; n < number_blocks[level]; n++)
  {
    //Serial.print("block number");
    //Serial.println(n);
    if(!pair_bob[level + n*4] == pair_alice[level + n*4])
    {
      flipped = true;

      int block_size;

      if(n == number_blocks[level]-1)
        block_size = last_block_size[level];
      else
         block_size = block_sizes[level];


      switch(level)
      {
        case 0:
          for(int i = 0; i < block_size; i++)
          {
            current_block_indices[i] = block0[n  * block_sizes[level] + i];
          }
          break;
        case 1:
          for(int i = 0; i < block_size; i++)
          {
            current_block_indices[i] = block1[n  * block_sizes[level] + i];
          }
          break;
        case 2:
          for(int i = 0; i < block_size; i++)
          {
            current_block_indices[i] = block2[n  * block_sizes[level] + i];
          }
          break;
        case 3:
          for(int i = 0; i < block_size; i++)
          {
            current_block_indices[i] = block3[n  * block_sizes[level] + i];
          }
          break;
      }
      int index = binary_search(0,block_size);
      bitWrite(mykey[index/8],index%8,!bitRead(mykey[index/8],index%8));
    }
  }
  if(flipped && level > 0)
    itterate_block(level -1);
  else if(level == 3)
    return;
  else
    itterate_block(level +1);
}

/* Do the binary search to find one wrong bit */
int PPB_Error_Correction::binary_search(int sta, int sto)
{
  //Serial.println("start binary search");
  //Serial.println("binary_search");
  int len = sto - sta;
  if(len <= 1)
    return current_block_indices[sta];
  
  if(getAlicePairity(sta,sta + len/2) == getBobPairity(sta,sta + len/2))
    return binary_search(sta + len/2,sto);  
  else
    return binary_search(sta,sta + len/2);
}

/* requests and returns a pairity from Alice */
boolean PPB_Error_Correction::getAlicePairity(int sta, int sto)
{
  aliceParityCallCount++; 
  int len =  sto - sta;
  Serial2.write((byte*)&len,4);
  Serial2.write((byte*)&current_block_indices[sta], (sto - sta) * 4);

  while(!Serial2.available()){}
  

  return (Serial2.read() == (byte) 1);
}

/* returns parity value of key */
boolean PPB_Error_Correction::getBobPairity(int sta, int sto)
{
  boolean p = 0;
  for(int j = sta; j < sto; j++)
  {
    if(p)
      p = !bitRead(mykey[current_block_indices[j]/8],current_block_indices[j]%8);
    else
      p = bitRead(mykey[current_block_indices[j]/8],current_block_indices[j]%8);
  }
  return p;
}

/* function that proceeds the error correction */
bool PPB_Error_Correction::proceedErrorCorrection(unsigned int& aleak)
{
  Serial.println("Bob: start proceed error correction");
  aliceParityCallCount = 0;
  int i = 0;
  // read the incoming byte:
      // retrieve pairities from Alice

      do
      {
          if(Serial2.available() > 0)
          {
          pair_alice[i*4] = Serial2.read() == (byte) 1;
          i ++;
          }
      }
      while(i < number_blocks[0]);
      i = 0;
      do
      {
          if(Serial2.available() > 0)
          {
          pair_alice[1+4*i] = Serial2.read() == (byte) 1;
          i ++;
          }
      }
      while(i < number_blocks[1]);
      i = 0;
      do
      {
          if(Serial2.available() > 0)
          {
          pair_alice[2+4*i] = Serial2.read() == (byte) 1;
          i ++;
          }
      }
      while(i < number_blocks[2]);
      i = 0;
      do
      {
          if(Serial2.available() > 0)
          {
          pair_alice[3+4*i] = Serial2.read() == (byte) 1;
          i ++;
          }
      }
      while(i < number_blocks[3]);      

      Serial.println("Bob: Alice Block Parities received");
      itterate_block(0);

      int len =  0;   // send a zero to Alice to signal finished error correcion
      Serial2.write((byte*)&len,4);
      aleak = aliceParityCallCount;
      return false;
}

/* print parities for debugging */
void PPB_Error_Correction::printPairities()
{
  Serial.println("Pairities: ");
  for(int i = 0; i < number_blocks[0]; i++)
  {
    Serial.println((String) pair_bob[i*4] + " " + (String) pair_alice[i*4]);
  }

  for(int i = 0; i < number_blocks[1]; i++)
  {
    Serial.println((String)pair_bob[1 + i*4] + " " + (String) pair_alice[1+4*i]);
  }

  for(int i = 0; i < number_blocks[2]; i++)
  {
   Serial.println((String)pair_bob[2+ 4*i] + " " + (String) pair_alice[2+4*i]);
  }

  for(int i = 0; i < number_blocks[3]; i++)
  {
    Serial.println((String)pair_bob[3+i*4] + " " + (String) pair_alice[3+4*i]);
  }
}

/* Function that prepares simulated data, should be called before startErrorCorrection() */
void PPB_Error_Correction::prepare_Data(uint8_t* key, int length)
{
   Serial.println("Bob: start prepare data");
    mykey = key;
    BLOCK_SIZE = length;

    block_sizes[0] =  0.73/0.03; //BLOCK_SIZE / 16;
    number_blocks[0] = (BLOCK_SIZE /  block_sizes[0]) + 1;
    last_block_size[0] =  BLOCK_SIZE %  block_sizes[0];
    for(int i = 1; i < 4; i++)
    {
      block_sizes[i] = block_sizes[i-1] * 2;

      number_blocks[i] = (BLOCK_SIZE /  block_sizes[i]) + 1;

      last_block_size[i] =  BLOCK_SIZE %  block_sizes[i];
    }

    pair_bob = new boolean[4* number_blocks[0]];
    pair_alice = new boolean[4* number_blocks[0]];

    Serial.println("Bob: start prepare blocks");
    prepare_blocks();
}

/* calculates the parity values */
void PPB_Error_Correction::calc_permutations()
{
  for(int i = 0; i < number_blocks[0]; i++)
  {
    int block_size;
    if(i == number_blocks[0]-1)
        block_size = last_block_size[0];
      else
         block_size = block_sizes[0];

    pair_bob[i*4] = 0;
    for(int j = 0; j < block_size; j++)
    {
      if(pair_bob[i*4])
        pair_bob[i*4] = !bitRead(mykey[block0[i * block_sizes[0] + j]/8],block0[i * block_sizes[0] + j]%8);
      else
        pair_bob[i*4] = bitRead(mykey[block0[i * block_sizes[0] + j]/8],block0[i * block_sizes[0] + j]%8);
    }
  }


  for(int i = 0; i < number_blocks[1]; i++)
  {
    int block_size;
    if(i == number_blocks[1]-1)
      block_size = last_block_size[1];
    else
        block_size = block_sizes[1];

    pair_bob[1 + 4*i] = 0;
    for(int j = 0; j < block_size; j++)
    {
      if(pair_bob[1 + 4*i])
        pair_bob[1 + 4*i] = !bitRead(mykey[block1[i * block_sizes[1] + j]/8],block1[i * block_sizes[1] + j]%8);
      else
        pair_bob[1 + 4*i] = bitRead(mykey[block1[i * block_sizes[1] + j]/8],block1[i * block_sizes[1] + j]%8);
    }
  }


  for(int i = 0; i < number_blocks[2]; i++)
  {
    int block_size;
    if(i == number_blocks[2]-1)
      block_size = last_block_size[2];
    else
      block_size = block_sizes[2];

    pair_bob[2 + 4*i] = 0;
    for(int j = 0; j < block_size; j++)
    {
      if(pair_bob[2 + 4*i])
        pair_bob[2 + 4*i] = !bitRead(mykey[block2[i * block_sizes[2] + j]/8],block2[i * block_sizes[2] + j]%8);
      else
        pair_bob[2 + 4*i] = bitRead(mykey[block2[i * block_sizes[2] + j]/8],block2[i * block_sizes[2] + j]%8);
    }
  }

  for(int i = 0; i < number_blocks[3]; i++)
  {
    int block_size;
    if(i == number_blocks[3]-1)
      block_size = last_block_size[3];
    else
        block_size = block_sizes[3];

    pair_bob[3 + 4*i] = 0;
    for(int j = 0; j < block_size; j++)
    {
      if(pair_bob[3 + 4*i])
        pair_bob[3 + 4*i] = !bitRead(mykey[block3[i * block_sizes[3] + j]/8],block3[i * block_sizes[3] + j]%8);
      else
        pair_bob[3 + 4*i] = bitRead(mykey[block3[i * block_sizes[3] + j]/8],block3[i * block_sizes[3] + j]%8);
    }
  }
}

/* prints an array of bytes as bit values */
void PPB_Error_Correction::printBitArray(byte b[])
{
  for(int i = 0; i < 32; i++)
  {
    for(int j = 0; j < 8; j++)
    {
      Serial.print(bitRead(b[i],j));
    }
  }
  Serial.println();
}

/* prepare shuffeled indices */
void PPB_Error_Correction::prepare_blocks()
{
  block0 = (int*)malloc((BLOCK_SIZE) * sizeof(int));
  block1 = (int*)malloc((BLOCK_SIZE) * sizeof(int));
  block2 = (int*)malloc((BLOCK_SIZE) * sizeof(int));
  block3 = (int*)malloc((BLOCK_SIZE) * sizeof(int));

  for(int i = 0; i < BLOCK_SIZE; i++)
  {
    block0[i] = i;
    block1[i] = i;
    block2[i] = i;
    block3[i] = i;
  }

  shuffle_array(block0, 4);
  shuffle_array(block1, 11);
  shuffle_array(block2, 19);
  shuffle_array(block3, 99);

  Serial.println("Bob: finish prepare blocks");
}

/* shuffles an array */
void PPB_Error_Correction::shuffle_array(int *array,int seed)
{
  randomSeed(seed);
  int buf;
  for(int i = BLOCK_SIZE-1; i > 1; i--)
  {
    int j = random(i);
    buf = array[i];
    array[i] = array[j];
    array[j] = buf;
  }
}

/* prints an array of integeres */
void PPB_Error_Correction::printIntArray(int b[], int len)
{
  for(int i = 0; i < len; i++)
  {
    Serial.print(String(b[i]) + " ");
  }
  Serial.println();
}
