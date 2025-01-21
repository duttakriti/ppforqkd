import numpy as np
import random
import math
import time

def create_generator_matrix(k, n):
    """ Create a generator matrix for (n, k) code. """
    I_k = np.eye(k, dtype=int)  # Identity matrix of size k
    P = np.random.randint(0, 2, (k, n - k))  # Random parity matrix
    G = np.hstack((I_k, P))  # Combine to form G
    return G



def calculaterr(suba, subb) :

    if len(suba) != len(subb):
        raise ValueError("Subsets must be of the same length")
    mismatches = sum(1 for a, b in zip(suba, subb) if a != b)
    error_rate = mismatches / len(suba)
    return error_rate


def shuffle_key(key):
    """ Shuffle the noisy key and return original indices. """
    shuffled_key = key[:]
    original_indices = list(range(len(key)))
    random.shuffle(shuffled_key)

    # Create a mapping of original indices
    shuffled_indices = sorted(range(len(original_indices)), key=lambda k: original_indices[k])

    return shuffled_key, shuffled_indices

def restore_key(shuffled_key, original_indices):
    """ Restore the key to its original order. """
    restored_key = [None] * len(shuffled_key)
    for i, original_index in enumerate(original_indices):
        restored_key[original_index] = shuffled_key[i]
    return restored_key
def calculate_block_size( iteration ,error_rate):
    """ Calculate the block size based on the iteration number and current size of the data. """
    if iteration == 1:
        k=  max(1, math.ceil(0.73 / error_rate))  # Set block size to 10% of current size for the first iteration
    else:
        k= (2 ** (iteration - 1)) * max(1, math.ceil(0.73 / error_rate))
    return k
def divide_into_blocks(key, block_size):
    """ Divide the key into blocks of specified size. """
    return [key[i:i + block_size] for i in range(0, len(key), block_size)]

def ask_parity(block):
    """ Simulate asking Alice for the correct parity of the block. """
    return sum(block) % 2  # Even = 0, Odd = 1


def run_binary_algorithm(block, parity_check_count):
    """ Implement the Binary algorithm to correct errors in the block. """
    if len(block) == 1:
        # If the block size is 1 and we know there's an error, we simply flip it
        block[0] = 1 - block[0]  # Flip the single bit
        return block, parity_check_count + 1  # Increment count for this check

    # Split the block into two sub-blocks
    mid = len(block) // 2
    left_subblock = block[:mid]
    right_subblock = block[mid:]

    # Calculate parities
    current_left_parity = sum(left_subblock) % 2
    current_right_parity = sum(right_subblock) % 2

    # Ask Alice for the correct parity of each sub-block
    correct_left_parity = ask_parity(left_subblock)
    correct_right_parity = ask_parity(right_subblock)

    # Increment the count for the parity checks
    parity_check_count += 2  # One for each sub-block

    left_errors = current_left_parity != correct_left_parity
    right_errors = current_right_parity != correct_right_parity

    if left_errors and not right_errors:
        # Only left sub-block has errors
        return run_binary_algorithm(left_subblock, parity_check_count)
    elif right_errors and not left_errors:
        # Only right sub-block has errors
        return run_binary_algorithm(right_subblock, parity_check_count)
    elif left_errors and right_errors:
        # Both sub-blocks have errors
        corrected_left, parity_check_count = run_binary_algorithm(left_subblock, parity_check_count)
        corrected_right, parity_check_count = run_binary_algorithm(right_subblock, parity_check_count)
        return corrected_left + corrected_right, parity_check_count  # Combine corrected blocks
    else:
        # If no errors detected, simply return the block as is
        return block, parity_check_count


def cascadeerror_correction(subset_a, subset_b, error_rate, iterations=4):
    """ Perform the Cascade error correction protocol with parity exchanges. """
    noisy_key = subset_b[:]  # Start with Bob's subset (noisy key)
    original_key = subset_a[:]  # Alice's original version
    leaked_info = []  # Initialize leaked information
    parity_check_count = 0  # Initialize parity check counter

    for iteration in range(1, iterations + 1):
       # print(f"\n--- Iteration {iteration} ---")

        shuffled_key, original_indices = shuffle_key(noisy_key)
      #  print(f"Shuffled Key: {shuffled_key}")

        # Calculate the current size of the noisy key
        current_size = len(shuffled_key)
        block_size = calculate_block_size(iteration, error_rate)
    #    print(f"Block Size: {block_size}, Number of Blocks: {current_size // block_size}")

        blocks = divide_into_blocks(noisy_key, block_size)

        for block_index, block in enumerate(blocks):
            original_block = original_key[block_index * block_size:(block_index + 1) * block_size]
            current_parity = sum(block) % 2  # Parity of Bob's block
            correct_parity = sum(original_block) % 2  # Parity of Alice's block

            # Increment parity check count for every block checked
            parity_check_count += 1
        #    print(
        #        f"Block {block_index + 1}: {block}, Current Parity: {current_parity}, Correct Parity: {correct_parity}")

            if current_parity != correct_parity:
        #        print("Odd number of errors detected.")
                corrected_block, parity_check_count = run_binary_algorithm(block, parity_check_count)
       #         print(f"Corrected Block {block_index + 1}: {corrected_block}")
                leaked_info.append((block_index + 1, current_parity, correct_parity))

       #     else:
       #         print("Even parity, no action taken.")

        restored_key = restore_key(noisy_key, original_indices)

    print(f"Total Parity Checks: {parity_check_count}")  # Print total parity checks
    return restored_key, parity_check_count  # Return the final corrected key and leaked info

def generate_encryptsets(num_bits, error_rate):
    # Generate the original message
    encryptseta = np.random.randint(2, size=num_bits)

    # Create a copy of encryptseta to create encryptsetb
    encryptsetb = np.copy(encryptseta)

    # Calculate the number of bits to flip for the desired error rate
    num_errors = int(num_bits * error_rate)

    # Randomly select indices to flip
    error_indices = np.random.choice(num_bits, num_errors, replace=False)

    # Flip the selected bits
    encryptsetb[error_indices] = 1 - encryptsetb[error_indices]

    return encryptseta, encryptsetb

#n = 100
#error_rate = 0.02
#encryptseta, encryptsetb = generate_encryptsets(n, error_rate)
#print("Original_message", encryptseta)
#start_time = time.time()
#reconciliated_key1, leaked_info1 = cascadeerror_correction(encryptseta, encryptsetb, error_rate)
#cascade_time = time.time() - start_time
#print("Corrected bits:", reconciliated_key1)
#leaked = leaked_info1 / n
#print("Leaked Info:", leaked)