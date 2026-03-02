#!/usr/bin/env python3
import serial
import serial.tools.list_ports
import time
import sys
import argparse
from tqdm import tqdm

def main():
    parser = argparse.ArgumentParser(description="Raw NAND USB Dumper")
    parser.add_argument("-p", "--port", type=str, help="Serial port (e.g., COM3 or /dev/ttyUSB0)")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default 115200)")
    parser.add_argument("-o", "--output", type=str, default="nand_dump.bin", help="Output file (default nand_dump.bin)")
    args = parser.parse_args()

    port = args.port
    if not port:
        ports = list(serial.tools.list_ports.comports())
        if not ports:
            print("No serial ports found!")
            sys.exit(1)
        # Default to the first found port
        port = ports[0].device
        print(f"Auto-selected port: {port}")

    print(f"Connecting to {port} at {args.baud} baud...")
    
    try:
        ser = serial.Serial(port, args.baud, timeout=2)
    except Exception as e:
        print(f"Failed to open port {port}: {e}")
        sys.exit(1)

    time.sleep(2) # Wait for Arduino to reset
    ser.reset_input_buffer()

    print("Sending 'dumptousb' command...")
    ser.write(b"dumptousb\n")
    
    total_bytes = 0
    # Wait for the ready message
    start_wait = time.time()
    while True:
        line = ser.readline().decode('utf-8', errors='ignore').strip()
        if line.startswith("READY FOR DUMP:"):
            try:
                total_bytes = int(line.split(":")[1])
                print(f"Device ready. Expecting {total_bytes} bytes.")
                break
            except ValueError:
                print("Failed to parse size.")
                sys.exit(1)
        elif "Unknown command" in line:
            print("Error: Firmware does not support 'dumptousb'.")
            sys.exit(1)
            
        if time.time() - start_wait > 5:
            print("Timeout waiting for device.")
            sys.exit(1)
            
    if total_bytes <= 0:
        print("Invalid byte count.")
        sys.exit(1)

    with open(args.output, "wb") as f:
        print(f"Starting dump to {args.output}...")
        ser.write(b'G') # Send 'G' to start
        
        # Read exact number of bytes
        bytes_read = 0
        
        with tqdm(total=total_bytes, unit='B', unit_scale=True, unit_divisor=1024) as pbar:
            while bytes_read < total_bytes:
                # Read in chunks
                chunk = ser.read(min(4096, total_bytes - bytes_read))
                if not chunk:
                    print("\nError: Reading timed out before completion.")
                    break
                f.write(chunk)
                bytes_read += len(chunk)
                pbar.update(len(chunk))
                
    # Wait to see if DUMP COMPLETE comes through
    ser.timeout = 0.5
    tail = ser.read(100)
    
    ser.close()
    print(f"\nDump finished. Read {bytes_read} of {total_bytes} bytes.")
    if bytes_read == total_bytes:
        print("Success!")
    else:
        print("Warning: Dump is incomplete!")

if __name__ == "__main__":
    main()
