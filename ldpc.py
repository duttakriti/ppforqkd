# -*- coding: utf-8 -*-
"""
Created on Tue Oct 22 10:14:45 2024

@author: fab08812
"""

import numpy as np
from pyldpc import make_ldpc, encode, decode, get_message
import time

d_v = 2
d_c = 5
n = 8200

H, G = make_ldpc(n, d_v, d_c, systematic=True, sparse=True)

k = G.shape[1]
leakedinfo= (n-k)/k
print("leaked info :", leakedinfo)
original_bits = np.random.randint(0, 2, k)


# Determine number of parity checks
num_parity_checks = H.shape[0]
print("Number of parity checks:", num_parity_checks)
codeword = encode(G, original_bits, snr=5)

received_bits = np.copy(codeword)  # received message is original plus some errors
received_bits[0] = 1 - received_bits[0]
received_bits[5] = 1 - received_bits[5]
start_time = time.time()

decoded_bits = decode(H, received_bits, snr=10, maxiter=10)
corrected_message = get_message(G, decoded_bits)
end_time = time.time()
print("Original_message", original_bits)
print(len(original_bits))
print("Corrected bits:", corrected_message)

if (np.sum(corrected_message == original_bits) == len(original_bits)):
    print("all errors fixed")
else:
    print("errors remain")

print("Time taken for error correction: {:.4f} seconds".format(end_time - start_time))