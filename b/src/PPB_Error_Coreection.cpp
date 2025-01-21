
#include "PPB_Error_Correction.h"


// ------- variables for error correction --------
PPB_Error_Correction::PPB_Error_Correction()
{

}

/* function that executes one level of the cascade algorithm */
void PPB_Error_Correction::itterate_block(byte level)
{
  boolean flipped = false;
  calc_permutations();
  Serial.println("itterate block");
  for(int n = 0; n < number_blocks[level]; n++)
  {
    if(!pair_bob[level][n] == pair_alice[level][n])
    {
      flipped = true;
      switch(level)
      {
        case 0:
          for(int i = 0; i < block_sizes[level]; i++)
          {
            current_block_indices[i] = block0[n  * block_sizes[level] + i];
          }
          break;
        case 1:
          for(int i = 0; i < block_sizes[level]; i++)
          {
            current_block_indices[i] = block1[n  * block_sizes[level] + i];
          }
          break;
        case 2:
          for(int i = 0; i < block_sizes[level]; i++)
          {
            current_block_indices[i] = block2[n  * block_sizes[level] + i];
          }
          break;
        case 3:
          for(int i = 0; i < block_sizes[level]; i++)
          {
            current_block_indices[i] = block3[n  * block_sizes[level] + i];
          }
          break;
      }
      int index = binary_search(0,block_sizes[level]);
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
  Serial.println("binary_search");
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
  int len =  sto - sta;
  Serial4.write((byte*)&len,4);
  Serial4.write((byte*) &current_block_indices[sta], (sto - sta) * 4);

  while(!Serial4.available()){}
  

  return (Serial4.read() == (byte) 1);
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
bool PPB_Error_Correction::proceedErrorCorrection()
{
    int i = 0;
    // read the incoming byte:
    if(Serial4.available() > 0)
    {
        // retrieve pairities from Alice
        do
        {
            if(Serial4.available() > 0)
            {
            pair_alice[0][i] = Serial4.read() == (byte) 1;
            i ++;
            }
        }
        while(i < 16);
        i = 0;
        do
        {
            if(Serial4.available() > 0)
            {
            pair_alice[1][i] = Serial4.read() == (byte) 1;
            i ++;
            }
        }
        while(i < 8);
        i = 0;
        do
        {
            if(Serial4.available() > 0)
            {
            pair_alice[2][i] = Serial4.read() == (byte) 1;
            i ++;
            }
        }
        while(i < 4);
        i = 0;
        do
        {
            if(Serial4.available() > 0)
            {
            pair_alice[3][i] = Serial4.read() == (byte) 1;
            i ++;
            }
        }
        while(i < 2);      

        itterate_block(0);
        printBitArray(mykey);
        long t_end = micros();
        delay(1000);
        Serial.println("error correction in:" + String(t_end - time_stemp));
    }
    return (i > 0);
}

/* print parities for debugging */
void PPB_Error_Correction::printPairities()
{
  Serial.println("Pairities: ");
  for(int i = 0; i < 16; i++)
  {
    Serial.println((String) pair_bob[0][i] + " " + (String) pair_alice[0][i]);
  }

  for(int i = 0; i < 8; i++)
  {
    Serial.println((String)pair_bob[1][i] + " " + (String) pair_alice[1][i]);
  }

  for(int i = 0; i < 4; i++)
  {
   Serial.println((String)pair_bob[2][i] + " " + (String) pair_alice[2][i]);
  }

  for(int i = 0; i < 2; i++)
  {
    Serial.println((String)pair_bob[3][i] + " " + (String) pair_alice[3][i]);
  }
}

/* Function that prepares simulated data, should be called before startErrorCorrection() */
void PPB_Error_Correction::prepare_Data(uint8_t* key, int length)
{
    mykey = key;
    BLOCK_SIZE = length;
}

/* calculates the parity values */
void PPB_Error_Correction::calc_permutations()
{
  for(int i = 0; i < 16; i++)
  {
    pair_bob[0][i] = 0;
    for(int j = 0; j < block_sizes[0]; j++)
    {
      if(pair_bob[0][i])
        pair_bob[0][i] = !bitRead(mykey[block0[i * block_sizes[0] + j]/8],block0[i * block_sizes[0] + j]%8);
      else
        pair_bob[0][i] = bitRead(mykey[block0[i * block_sizes[0] + j]/8],block0[i * block_sizes[0] + j]%8);
    }
  }


  for(int i = 0; i < 8; i++)
  {
    pair_bob[1][i] = 0;
    for(int j = 0; j < block_sizes[1]; j++)
    {
      if(pair_bob[1][i])
        pair_bob[1][i] = !bitRead(mykey[block1[i * block_sizes[1] + j]/8],block1[i * block_sizes[1] + j]%8);
      else
        pair_bob[1][i] = bitRead(mykey[block1[i * block_sizes[1] + j]/8],block1[i * block_sizes[1] + j]%8);
    }
  }


  for(int i = 0; i < 4; i++)
  {
    pair_bob[2][i] = 0;
    for(int j = 0; j < block_sizes[2]; j++)
    {
      if(pair_bob[2][i])
        pair_bob[2][i] = !bitRead(mykey[block2[i * block_sizes[2] + j]/8],block2[i * block_sizes[2] + j]%8);
      else
        pair_bob[2][i] = bitRead(mykey[block2[i * block_sizes[2] + j]/8],block2[i * block_sizes[2] + j]%8);
    }
  }

  for(int i = 0; i < 2; i++)
  {
    pair_bob[3][i] = 0;
    for(int j = 0; j < block_sizes[3]; j++)
    {
      if(pair_bob[3][i])
        pair_bob[3][i] = !bitRead(mykey[block3[i * block_sizes[3] + j]/8],block3[i * block_sizes[3] + j]%8);
      else
        pair_bob[3][i] = bitRead(mykey[block3[i * block_sizes[3] + j]/8],block3[i * block_sizes[3] + j]%8);
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