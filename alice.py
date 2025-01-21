import serial
import time

def setup_serial(port, baud_rate=115200, timeout=10):
    try:
        ser = serial.Serial(port, baud_rate, timeout=timeout)
        time.sleep(2)  # Wait to ensure the connection is established
        return ser
    except serial.SerialException as e:
        print(f"Could not open port {port}: {e}")
        return None

def amain(alicedata, num_qubits):
    adata = f"{alicedata[1]};{alicedata[2]}"
    print("adata:", adata)
    connectarduino(adata, num_qubits)

def connectarduino(adata, num_qubits):
    ser = setup_serial('COM4', 115200)  # Change to the correct COM port
    if ser is None:
        return

    if not ser.isOpen():
        ser.open()

    ser.write((adata + '\n').encode())
    time.sleep(2)

    received = ser.read(len(adata)*5).decode('latin1', errors='ignore')
    if isinstance(received, str):
        print("Alice's noisy message and bases:", received)

    matching_indices = ser.read(num_qubits * 3).decode().strip()
    if isinstance(matching_indices, str):
        print("Matching Indices from Bob:", matching_indices)

    ser.close()

#if __name__ == "__main__":
  #  alicedata = ["data1", "data2", "data3"]
  #  num_qubits = 5
  #  amain(alicedata, num_qubits)