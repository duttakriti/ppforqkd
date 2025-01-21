import hashlib
import math
import time
import random
import numpy as np
from cascade import cascadeerror_correction,calculaterr

def noise(qubits, noise_level, num_bits):
    nqubits = []

    nqubits = np.copy(qubits)

    # Calculate the number of bits to flip for the desired error rate
    num_errors = int(num_bits * noise_level)

    # Randomly select indices to flip
    error_indices = np.random.choice(num_bits, num_errors, replace=False)

    # Flip the selected bits
    nqubits[error_indices] = 1 - nqubits[error_indices]







    return nqubits


def generate_qubits(num_qubits):

    qubits = []
    bases = []
    for _ in range(num_qubits):
        qubit = random.randint(0, 1)
        #where 0 is rectilinear and 1 is diagonal
        basis = random.randint(0, 1)
        qubits.append(qubit)
        bases.append(basis)
    return qubits, bases

def generate_bobbases(num_qubits):
    bases = []
    for _ in range(num_qubits):
        #where 0 is rectilinear and 1 is diagonal
        basis = random.randint(0, 1)
        bases.append(basis)
    return bases


def measure_qubits(aqubits, abases,bbases):

    measurements = []
    for aqubits, abases, bbases in zip(aqubits, abases, bbases):
        if abases == bbases:  # rectilinear basis
            measurement = aqubits
        elif abases == 0 and bbases == 1:  # diagonal basis
            measurement = (aqubits + 1) % 2
        else :
            measurement = (aqubits + 1) % 2
        measurements.append(measurement)
    return measurements


def compare_bases(bases_A, bases_B):

    matching_indices = [i for i in range(len(bases_A)) if bases_A[i] == bases_B[i]]
    mi = [1 if bases_A[i] == bases_B[i] else 0 for i in range(len(bases_A))]
    return matching_indices,mi



def randumsubset(num_qubits, alice_qubits, bob_qubits, matching_indices):
    key_usage = int(0.4 * len(matching_indices))
    key_usage = min(key_usage, len(matching_indices))  # Ensure it does not exceed

    # Create the subset based on the matching indices
    sifteda = [alice_qubits[i] for i in matching_indices]
    siftedb = [bob_qubits[i] for i in matching_indices]
    

    if len(sifteda) > key_usage:
        # Randomly sample from the subset
        sampled_indices = random.sample(range(len(sifteda)), key_usage)
        sifteda = [sifteda[i] for i in sampled_indices]
        siftedb = [siftedb[i] for i in sampled_indices]
        selected_indices = [matching_indices[i] for i in sampled_indices]

    remaining_indices = list(set(matching_indices) - set(selected_indices))
    # Create encrypt set with data not in the subset
    encryptseta = [alice_qubits[i] for i in remaining_indices]
    encryptsetb = [bob_qubits[i] for i in remaining_indices]


    return sifteda, siftedb, encryptseta, encryptsetb

def bobsubsetgnrt(bob_measures, matching_indices, subsetindices):
    # Create the subset based on the provided indices
    bob_subset = [bob_measures[i] for i in subsetindices]

    # Identify the remaining indices that are in matching_indices but not in subsetindices
    remaining_indices = set(matching_indices) - set(subsetindices)

    # Create the encrypt set with Bob's data corresponding to the remaining matching indices
    encryptsetb = [bob_measures[i] for i in remaining_indices]

    return bob_subset, encryptsetb

def create_generator_matrix(k, n):
    """ Create a generator matrix for (n, k) code. """
    I_k = np.eye(k, dtype=int)  # Identity matrix of size k
    P = np.random.randint(0, 2, (k, n - k))  # Random parity matrix
    G = np.hstack((I_k, P))  # Combine to form G
    return G


def create_toeplitz_matrix(first_col, first_row):
    """ Create a Toeplitz matrix from the first column and first row. """
    toeplitz_matrix = np.zeros((len(first_col),len(first_row)))
    offset = len(first_col)
    for y in range(len(first_col)):
        for x in range(len(first_row)):
            toeplitz_matrix[y,x] = first_col[y - x] if y > x else first_row[x - y]
    return toeplitz_matrix

def calc_key_rate(key_length, qber, leaked_bits):
    entropy_qber = -qber * math.log2(qber) - (1 - qber) * math.log2(1 - qber)
    result = int(math.floor(key_length * (1 - 2 * entropy_qber))) - leaked_bits
    return result

def privacyamp(reconciliated_key, leaked_info, error_rate):

    R =  calc_key_rate(len(reconciliated_key), error_rate, leaked_info)

    if R <= 0:
        return "Too much information leaked, can't proceed with encryption."

    # Generate a Toeplitz matrix
    first_col = [random.randint(0, 1) for _ in range(R)]
    first_row = [random.randint(0, 1) for _ in range(len(reconciliated_key))]
    toeplitz_matrix = create_toeplitz_matrix(first_col, first_row)

    final_key =np.dot(toeplitz_matrix, reconciliated_key) % 2

    return final_key.astype(int).tolist()


import time


def bb84_protocol(num_qubits, error_threshold, noise_level):
    # Generate qubits with random values and bases
    alice_qubits, alice_bases = generate_qubits(num_qubits)
    nalice_qubits = noise(alice_qubits, noise_level, num_qubits)
    bob_bases = generate_bobbases(num_qubits)

    # Introducing noise in Alice
    bob_measures = measure_qubits(nalice_qubits, alice_bases, bob_bases)

    # Comparing bases used by Alice and Bob
    matching_indices, mi = compare_bases(alice_bases, bob_bases)
    subseta,subsetb, encryptseta, encryptsetb = randumsubset(num_qubits, alice_qubits, bob_measures, matching_indices)

    error_rate = calculaterr(subseta, subsetb)

    #Measure time for Cascade error correction
    start_time_cascade = time.time()
    reconciliated_key1, leaked_info1 = cascadeerror_correction(encryptseta, encryptsetb, error_rate)
    end_time_cascade = time.time()
    cascade_duration = end_time_cascade - start_time_cascade


    cencrypted_key = privacyamp(reconciliated_key1, leaked_info1, error_rate)

    """
    print(" Matching Indices ")
    print(mi)
    print(" Subset of Alice Chosen ")
    print(subseta)
    print(" Subset of Bob Chosen ")
    print(subsetb)
    print(" Encrypt of Alice Chosen ")
    print(encryptseta)
    print(" Encrypt of Bob Chosen ")
    print(encryptsetb)
    print("  Error correction done, Cascade reconciliated key : ")
    print(reconciliated_key1)

    print("  Cascade Encrypted Key ")
    print(cencrypted_key)
    print(f"Cascade Error Correction Duration: {cascade_duration:.6f} seconds")
    """

    # Check error rate and generate secret key
    if error_rate <= error_threshold:
        print(" Error rate ")
        print(error_rate)
        data = cencrypted_key, alice_qubits, alice_bases, bob_measures, bob_bases, error_rate
        return data
    else:
        return "Error rate above threshold. Protocol aborted."



def withoutnoisebb84_protocol(num_qubits, error_threshold):

    #Generate qubits with random values and bases
    alice_qubits, alice_bases = generate_qubits(num_qubits)

    #Bob measures qubits with random bases
    bob_bases = generate_bobbases(num_qubits)
    bob_measures = measure_qubits(alice_qubits, alice_bases, bob_bases)

    #Comparing bases used by Alice and Bob
    matching_indices,mi = compare_bases(alice_qubits, bob_measures)

    #Error correction
    secret_key = cascadeerror_correction(alice_qubits, alice_bases, matching_indices)

    #Calculate error rate
    error_rate = 1 - (len(matching_indices) / num_qubits)

    #Check error rate and generate secret key
    if error_rate <= error_threshold:
        #data = alice_qubits,alice_bases,bob_measures,bob_bases,secret_key,error_rate
        data = alice_qubits, alice_bases, bob_measures, bob_bases, mi, error_rate
        return  data
    else:
        return "Error rate above threshold. Protocol aborted."





