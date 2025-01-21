#include "PPA_Error_Correction.h"


PPA_Error_Correction::PPA_Error_Correction()
{
    
}

/* Call this to start the Error Correction */
void PPA_Error_Correction::startErrorCorrection()
{
    calc_permutations();
}

/* Should be called in a loop until the error correction is finished. It waits for Bob to request some parity, then calculates it and sends it to bob. */
bool PPA_Error_Correction::proceedErrorCorrection()
{
    int i = 0;
    // read the incoming byte:
    if (Serial4.available() > 0)
    {
            boolean p = 0;
            
            int len = read_int();
            //Serial.println("sending " + String(len) + " bits block pairity");
            do
            {
                if(Serial4.available() > 0)
                {
                int index = read_int();
                if(p)
                    p = !bitRead(mykey[index/8],index%8);
                else
                    p = bitRead(mykey[index/8],index%8);
                i++;
                }
            }
            while(i < len);

            Serial4.write((byte) p);
    }
    return (i > 0);
}

/* Function that prepares simulated data, should be called before startErrorCorrection() */
void PPA_Error_Correction::prepare_Data()
{
  // Prepare the key
  for(int i = 0; i < BLOCK_SIZE; i++)
  {
      int b = i % 8;
      int n = i / 8;

      bitWrite(mykey[n],b,bitString[i%256]=='1');
  }
  
  block_sizes[0] = BLOCK_SIZE / 16;
  for(int i = 1; i < 4; i++)
    block_sizes[i] = block_sizes[i-1] * 2;

  prepare_blocks();
    
  printIntArray(block0, BLOCK_SIZE);
  printIntArray(block1, BLOCK_SIZE);
  printIntArray(block2, BLOCK_SIZE);
  printIntArray(block3, BLOCK_SIZE);

}

/* calculates the parity values for each block */
void PPA_Error_Correction::calc_permutations()
{
  for(int i = 0; i < 16; i++)
  {
    pair_alice0[i] = 0;
    for(int j = 0; j < block_sizes[0]; j++)
    {
      if(pair_alice0[i])
      {
        pair_alice0[i] = !bitRead(mykey[block0[i * block_sizes[0] + j]/8],block0[i * block_sizes[0] + j]%8);
      }
      else
      {
        pair_alice0[i] = bitRead(mykey[block0[i * block_sizes[0] + j]/8],block0[i * block_sizes[0] + j]%8);
        
      } 
    }
  }


  for(int i = 0; i < 8; i++)
  {
    pair_alice1[i] = 0;
    for(int j = 0; j < block_sizes[1]; j++)
    {
      if(pair_alice1[i])
      {
        pair_alice1[i] = !bitRead(mykey[block1[i * block_sizes[1] + j]/8],block1[i * block_sizes[1] + j]%8);
      }
      else
      {
        pair_alice1[i] = bitRead(mykey[block1[i * block_sizes[1] + j]/8],block1[i * block_sizes[1] + j]%8);
      }
    }
  }


  for(int i = 0; i < 4; i++)
  {
    pair_alice2[i] = 0;
    for(int j = 0; j < block_sizes[2]; j++)
    {
      if(pair_alice2[i])
      {
        pair_alice2[i] = !bitRead(mykey[block2[i * block_sizes[2] + j]/8],block2[i * block_sizes[2] + j]%8);
      }
      else
      {
        pair_alice2[i] = bitRead(mykey[block2[i * block_sizes[2] + j]/8],block2[i * block_sizes[2] + j]%8);
      }
    }
  }
  
  for(int i = 0; i < 2; i++)
  {
    pair_alice3[i] = 0;
    for(int j = 0; j < block_sizes[3]; j++)
    {
      if(pair_alice3[i])
      {
        pair_alice3[i] = !bitRead(mykey[block3[i * block_sizes[3] + j]/8],block3[i * block_sizes[3] + j]%8);
      }
      else
      {
        pair_alice3[i] = bitRead(mykey[block3[i * block_sizes[3] + j]/8],block3[i * block_sizes[3] + j]%8);
      }
    }
  }
  sendPairities();
}

/* sends the initial blok parities. Is called directly after calcParities */
void PPA_Error_Correction::sendPairities()
{
  for(int i = 0; i < 16; i++)
  {
    Serial4.write(pair_alice0[i]);
  }

  for(int i = 0; i < 8; i++)
  {
    Serial4.write(pair_alice1[i]);
  }

  for(int i = 0; i < 4; i++)
  {
   Serial4.write(pair_alice2[i]);
  }

  for(int i = 0; i < 2; i++)
  {
    Serial4.write(pair_alice3[i]);
  }
}

/* Reads a 4 byte integer value send via serial */
int PPA_Error_Correction::read_int()
{ 
  byte j = 0;
  byte buf[4];
  while(j < 4)
  {
    if(Serial4.available())
    {
      buf[j] = Serial4.read();
      j ++;
    }
  }

  int out;
  memcpy(&out, buf, 4);
  return out;
}

/* prepares the shuffeled indices for each itteration of Cascade */
void PPA_Error_Correction::prepare_blocks()
{
  for(int i = 0; i < BLOCK_SIZE; i++)
  {
    block0[i] = i;
    block1[i] = i;
    block2[i] = i;
    block3[i] = i;
  }

  shuffle_array(block0, 4);   // just some random seed value
  shuffle_array(block1, 11);  // needs to be different for each itteration
  shuffle_array(block2, 19);
  shuffle_array(block3, 99);

}

/* shuffels an array os size BLOCK_SIZE */
void PPA_Error_Correction::shuffle_array(int *array,int seed)
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

/* prints an array for debuggin */
void PPA_Error_Correction::printIntArray(int b[], int len)
{
  for(int i = 0; i < len; i++)
  {
    Serial.print(String(b[i])+ " ");
  }
  Serial.println();
}