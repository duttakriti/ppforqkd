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
    if (Serial2.available() > 0)
    {
            boolean p = 0;
            
            int len = read_int();
        
            if(len == 0)
              return false;

            do
            {
                if(Serial2.available() > 0)
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

            Serial2.write((byte) p);
    }
    return true;
}

/* Function that prepares simulated data, should be called before startErrorCorrection() */
void PPA_Error_Correction::prepare_Data(uint8_t* key, int length)
{
  mykey = key;
  BLOCK_SIZE = length;
  

  block_sizes[0] =  0.73/0.03; //BLOCK_SIZE / 16;
  number_blocks[0] = (BLOCK_SIZE /  block_sizes[0]) + 1;
  last_block_size[0] =  BLOCK_SIZE %  block_sizes[0];

  Serial.println("start calculating block sizes");
  for(int i = 1; i < 4; i++)
  {
    block_sizes[i] = block_sizes[i-1] * 2;

    number_blocks[i] = (BLOCK_SIZE /  block_sizes[i]) + 1;
    last_block_size[i] =  BLOCK_SIZE %  block_sizes[i];
  }

  pair_alice0 = new boolean[number_blocks[0]];
  pair_alice1 = new boolean[number_blocks[1]];
  pair_alice2 = new boolean[number_blocks[2]];
  pair_alice3 = new boolean[number_blocks[3]];

  Serial.println("start preparing blocks");
  prepare_blocks();



    
  /*printIntArray(block0, BLOCK_SIZE);
  printIntArray(block1, BLOCK_SIZE);
  printIntArray(block2, BLOCK_SIZE);
  printIntArray(block3, BLOCK_SIZE);
  */
}

/* calculates the parity values for each block */
void PPA_Error_Correction::calc_permutations()
{
  Serial.println("start calculating permutations");
  for(int i = 0; i < number_blocks[0]; i++)
  {
    int block_size;
    if(i == number_blocks[0]-1)
        block_size = last_block_size[0];
      else
         block_size = block_sizes[0];

    pair_alice0[i] = 0;
    for(int j = 0; j < block_size; j++)
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


  for(int i = 0; i < number_blocks[1]; i++)
  {
    int block_size;
    if(i == number_blocks[1]-1)
      block_size = last_block_size[1];
    else
        block_size = block_sizes[1];

    pair_alice1[i] = 0;
    for(int j = 0; j < block_size; j++)
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


  for(int i = 0; i <  number_blocks[2]; i++)
  {
    int block_size;
    if(i == number_blocks[2]-1)
      block_size = last_block_size[2];
    else
      block_size = block_sizes[2];

    pair_alice2[i] = 0;
    for(int j = 0; j < block_size; j++)
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
  
  for(int i = 0; i <  number_blocks[3]; i++)
  {
    int block_size;
    if(i == number_blocks[3]-1)
      block_size = last_block_size[3];
    else
      block_size = block_sizes[3];

    pair_alice3[i] = 0;
    for(int j = 0; j < block_size; j++)
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

    Serial.println("finish calculating permutations");
  sendPairities();
}

/* sends the initial blok parities. Is called directly after calcParities */
void PPA_Error_Correction::sendPairities()
{
  for(int i = 0; i < number_blocks[0]; i++)
  {
    Serial2.write(pair_alice0[i]);
  }

  for(int i = 0; i < number_blocks[1]; i++)
  {
    Serial2.write(pair_alice1[i]);
  }

  for(int i = 0; i < number_blocks[2]; i++)
  {
   Serial2.write(pair_alice2[i]);
  }

  for(int i = 0; i < number_blocks[3]; i++)
  {
    Serial2.write(pair_alice3[i]);
  }
}

/* Reads a 4 byte integer value send via serial */
int PPA_Error_Correction::read_int()
{ 
  byte j = 0;
  byte buf[4];
  while(j < 4)
  {
    if(Serial2.available())
    {
      buf[j] = Serial2.read();
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