import serial

TARGET_COM_PORT = "COM3"

def main():
    print("Connecting to", TARGET_COM_PORT)
    s = serial.Serial(TARGET_COM_PORT, baudrate=115200)
    if s.is_open:
        print("Conneced! listening...")
    while True:
        length_byte = s.read(size=1)[0]
        data = s.read(size=length_byte)
        print("Got {} bytes from device".format(len(data)))
        print("[{}]".format(", ".join(["0x{}".format(hex(x)) for x in data])))
        print()


if __name__ == "__main__":
    main()