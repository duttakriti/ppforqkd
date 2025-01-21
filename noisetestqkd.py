
#import transferab
from main import *
#import alice#import bob
import connectab
import subprocess
import random

noise_level = 0.03
num_qubits = 4096*8

#num_qubits = int(input("Enter the block size : "))

print("Alice's message length", num_qubits)
#error_threshold = 1.0   #open param
error_threshold = float(input("Enter the error threshold [0,1] : "))
data = bb84_protocol(num_qubits, error_threshold, noise_level)
alicedata = data[1], data[2]
bobdata = data[3], data[4]






def runaliceandbob():

    #alice.amain(alicedata,num_qubits)
    #bob.bmain(bobdata,num_qubits)
    connectab.connectarduino(alicedata,bobdata,num_qubits)
    #transferab.connectarduino(alicedata,bobdata,num_qubits)
runaliceandbob()

