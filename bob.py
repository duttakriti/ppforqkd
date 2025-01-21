import serial
import time

def setup_serial(port, baud_rate=115200, timeout=10):
    ser = serial.Serial(port, baud_rate, timeout=timeout)
    time.sleep(2)  # Wait to ensure the connection is established
    return ser

def bmain(bobdata, num_qubits):
    if isinstance(bobdata, str):
        print(bobdata)
    else:
        print("Secret Key:", bobdata[2])
        print("Secret Key length:", len(bobdata[2]))
        print("Error rate:", bobdata[3])

    bdata = f"{bobdata[0]};{bobdata[1]}"
    print("bdata:", bdata)
    send_bdata(bdata, num_qubits)

def send_bdata(bdata, num_qubits):
    try:
        ser = setup_serial('COM5', 115200)  # Change to the correct COM port
        if not ser.isOpen():
            ser.open()
        ser.flushInput()
        ser.flushOutput()
        ser.write((bdata + '\n').encode())

        breceived = ser.read(len(bdata)*5).decode('latin1', errors='ignore')
        if isinstance(breceived, str):
            print("Bob's received message and bases:", breceived)
        time.sleep(2)
        alicebases = ser.read(len(bdata) * 3).decode().strip()
        if isinstance(alicebases, str):
            print("Alice's bases from Bob:", alicebases)
    except serial.SerialException as e:
        print(f"Error opening the port: {e}")
    finally:
        if ser.isOpen():
            ser.close()

#if __name__ == "__main__":
    #bobdata = ["data1", "data2", "secret_key", "error_rate"]
    #num_qubits = 5  # Define the number of qubits or expected length
   # bmain(bobdata, num_qubits)