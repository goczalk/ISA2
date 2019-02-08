import serial
ser = serial.Serial(port='/dev/ttyS0', baudrate=9600, timeout=0.05)
print(ser.isOpen())
yaw = 1
pitch = 2

packet = '<packet, {}, {}>'.format(yaw, pitch)
packetBytes = bytes(packet, 'utf-8')
   
while(1):
    print("1a")
    ser.write(b'aaa\n')
    print("2")
    print(ser.read_all())