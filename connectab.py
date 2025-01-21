import serial
import struct
import threading
import time

start_time = time.time()

serb = serial.Serial('COM4', 230400, timeout=20)
sera = serial.Serial('COM7', 230400, timeout=20)

readyb = False
readya = False
seqa = []
seqb = []
"""print("waiting for restart")
while(not readyb or not readya):
    if(serb.in_waiting):
        for c in serb.read():
            seqb.append(chr(c)) #convert from ANSII
            
            joined_seq = ''.join(str(v) for v in seqb) #Make a string from array
    
            if chr(c) == '\n':
                
                if(joined_seq == "ready"):
                    readyb = True
                
                seq = []
                break

    if(sera.in_waiting):
        for c in sera.read():
            seqa.append(chr(c)) #convert from ANSII
            
            joined_seq = ''.join(str(v) for v in seqa) #Make a string from array
    
            if chr(c) == '\n':
                
                if(joined_seq == "ready"):
                    readyb = True
                
                seq = []
                break

print("Bob and Alice restarted")"""

def send_to_alice(data):
    if not sera.isOpen():
        sera.open()
    start_time = time.time()
    binary_data = struct.pack(f'{len(data)}s', data.encode())
    chunk_size = 512
    for i in range(0, len(binary_data), chunk_size):
        chunk = data[i:i + chunk_size]
        sera.write(chunk.encode())
        time.sleep(0.1)
    elapsed_time = time.time() - start_time
    print(f"PC : Time taken to send to Alice: {elapsed_time:.2f} seconds \n")

def send_to_bob(data):
    if not serb.isOpen():
        serb.open()
    start_time = time.time()
    binary_data = struct.pack(f'{len(data)}s', data.encode())
    chunk_size = 1024
    for i in range(0, len(binary_data), chunk_size):
        chunk = data[i:i + chunk_size]
        serb.write(chunk.encode())
        time.sleep(0.1)

    elapsed_time = time.time() - start_time
    print(f"PC : Time taken to send to Bob: {elapsed_time:.2f} seconds \n")


def receive_from_alice(data_length):
    if not sera.isOpen():
        sera.open()
    start_time = time.time()
    while True:
        areceived = sera.read(data_length).decode('utf-8', errors='replace').strip()
        if areceived:
            print(areceived)
            break
    elapsed_time = time.time() - start_time
    print(f"PC : Time taken to receive from Alice: {elapsed_time:.2f} seconds \n")
    return areceived

def receive_from_bob(data_length):
    if not serb.isOpen():
        serb.open()
    start_time = time.time()
    while True:
        breceived = serb.read(data_length).decode('utf-8', errors='replace').strip()
        if breceived:
            print(breceived)
            break
    elapsed_time = time.time() - start_time
    print(f"PC : Time taken to receive from Bob: {elapsed_time:.2f} seconds \n")
    return breceived

def connectarduino(adata, bdata, num_qubits):

    print("adata:", f"{num_qubits};{''.join(map(str, adata[0]))};{''.join(map(str, adata[1]))}")
    adata_str = f"{num_qubits};{binary_list_to_hex(adata[0])};{binary_list_to_hex(adata[1])}"
    print("adata:", adata_str)



    print("bdata:", f"{num_qubits};{''.join(map(str, bdata[0]))};{''.join(map(str, bdata[1]))}")
    bdata_str = f"{num_qubits};{binary_list_to_hex(bdata[0])};{binary_list_to_hex(bdata[1])}"
    print("bdata:", bdata_str)


    t1 = threading.Thread(target=send_to_alice, args=(adata_str,))
    t2 = threading.Thread(target=send_to_bob, args=(bdata_str,))

    t1.start()
    t2.start()

    t1.join()
    t2.join()

    t3 = threading.Thread(target=receive_from_alice, args=(len(adata_str) * 100,))
    t4 = threading.Thread(target=receive_from_bob, args=(len(bdata_str) * 100,))

    t3.start()
    t4.start()

    t3.join(timeout=20)
    t4.join(timeout=20)

    sera.close()
    serb.close()


def binary_list_to_hex(binary_list):
    # Ensure the binary list length is a multiple of 8 by padding with zeros if necessary
    while len(binary_list) % 8 != 0:
        binary_list.insert(0, 0)

    hex_string = ''

    # Process each group of 8 binary digits
    for i in range(0, len(binary_list), 8):
        # Convert the group of 8 binary digits to a single integer
        binary_group = binary_list[i:i + 8]
        binary_str = ''.join(map(str, binary_group))  # Convert list to string
        hex_byte = format(int(binary_str, 2), '02X')  # Convert binary string to a 2-digit hex and uppercase it
        hex_string += hex_byte

    return hex_string


end_time = time.time()
duration = end_time - start_time

print(f"PC : Total execution time: {duration:.4f} seconds \n")