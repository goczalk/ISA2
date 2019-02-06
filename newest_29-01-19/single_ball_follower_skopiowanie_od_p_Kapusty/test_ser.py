import serial
ser = serial.Serial(port='/dev/ttyAMA0', baudrate=9600, timeout=0.05)

yaw = 1
pitch = 2

packet = '<packet, {}, {}>'.format(yaw, pitch)
packetBytes = bytes(packet, 'utf-8')
   
while(1):   
    ser.write(packetBytes)
    print(ser.read_all())