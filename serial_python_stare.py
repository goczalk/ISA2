Python 3.5.3 (default, Sep 27 2018, 17:25:39) 
[GCC 6.3.0 20170516] on linux
Type "copyright", "credits" or "license()" for more information.
>>> import serial
>>> ser = serial.Serial(port='/dev/ttyACM0', baudrate=57600, timeout=0.05)
Traceback (most recent call last):
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 265, in open
    self.fd = os.open(self.portstr, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
OSError: [Errno 16] Device or resource busy: '/dev/ttyACM0'

During handling of the above exception, another exception occurred:

Traceback (most recent call last):
  File "<pyshell#1>", line 1, in <module>
    ser = serial.Serial(port='/dev/ttyACM0', baudrate=57600, timeout=0.05)
  File "/usr/lib/python3/dist-packages/serial/serialutil.py", line 236, in __init__
    self.open()
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 268, in open
    raise SerialException(msg.errno, "could not open port {}: {}".format(self._port, msg))
serial.serialutil.SerialException: [Errno 16] could not open port /dev/ttyACM0: [Errno 16] Device or resource busy: '/dev/ttyACM0'
>>> ser = serial.Serial(port='/dev/ttyS0', baudrate=57600, timeout=0.05)
>>> ser.print(1)
Traceback (most recent call last):
  File "<pyshell#3>", line 1, in <module>
    ser.print(1)
AttributeError: 'Serial' object has no attribute 'print'
>>> 
>>> 
>>> ser.write(1)
Traceback (most recent call last):
  File "<pyshell#6>", line 1, in <module>
    ser.write(1)
  File "/usr/lib/python3/dist-packages/serial/serialposix.py", line 558, in write
    return len(data)
TypeError: object of type 'int' has no len()
>>> ser.write(bytes(str(1), 'utf-8'))
1
>>> ser.write(bytes(str(1), 'utf-8'))
1
>>> bytes(str(1), 'utf-8')
b'1'
>>> ser.write(bytes(str(1), 'utf-8'))
1
>>> ser.write(bytes(1, 'utf-8'))
Traceback (most recent call last):
  File "<pyshell#11>", line 1, in <module>
    ser.write(bytes(1, 'utf-8'))
TypeError: encoding without a string argument
>>> ser.write(bytes("\n", 'utf-8'))
1
>>> ser.write(bytes(str(1), 'utf-8'))
1
>>> ser.write(bytes(str(3), 'utf-8'))
1
>>> 
