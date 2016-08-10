import serial
import time
import sys

ser = serial.Serial(port='/dev/ttyUSB0',
                    baudrate=115200,
                    timeout=0.01)

byte_buffer = bytearray()

def serialize8(a):
    global byte_buffer
    if isinstance(a, int):
        a = chr(a)
    byte_buffer += a


def serialize16(a):
    serialize8((a) & 0xFF)
    serialize8((a >> 8) & 0xFF)


def serialize32(a):
    serialize8((a) & 0xFF)
    serialize8((a >> 8) & 0xFF)
    serialize8((a >> 16) & 0xFF)
    serialize8((a >> 24) & 0xFF)

# def serializeFloat(a):
#     b = struct.pack('<f', a)
#     for x in xrange(0, 4):
#         serialize8(b[x])

def send_16(value):
    high = chr(value >> 8)
    low = chr(value % 256)
    ser.write(low)
    ser.write(high)


def send_8(value):
    ser.write(chr(value))


def packu8(operation, target=None, data=None):
    global byte_buffer
    byte_buffer = bytearray()
    header = ord('<')
    serialize8(header)
    serialize8(operation)
    crc = header ^ operation

    if target is not None:
        serialize8(target)
        crc = crc ^ target

    if data is not None:
        serialize8(len(data))
        crc = crc ^ len(data)
        for item in data:
            crc = crc ^ item
            serialize8(item)

    serialize8(crc)
    ser.write(byte_buffer)


WAITING_HEADER = 0  # // <
WAITING_OPERATION = 1  # // request_All, read, write
WAITING_TARGET = 2  # // 0-255. variable register
WAITING_PAYLOAD = 3  # // 0-255. data bytes to receive
WAITING_DATA = 4  # // data itself
WAITING_CRC = 5

REQUEST_ALL = 33
WRITE = 34
READ = 35

databuffer = bytearray()
payloadsize = 0
payloadLeft = 0
crc = 0
status = WAITING_HEADER
operation = 0
target = 0

types = ["_uint8_t",
         "_uint16_t",
         "_uint32_t",
         "_int8_t",
         "_int16_t",
         "_int32_t",
         "_float,"]

variables = {}

messageBuffer = {}


def waitForMsg(type, target, timeout=0.1):
    start = time.time()
    while (type, target) not in messageBuffer.keys():
        for char in ser.readall():
            processByte(char)
        time.sleep(0.001)
        if (time.time() - start) > timeout:
            return None

    return messageBuffer.pop((type, target), None)


def processMessage():
    global messageBuffer
    global operation
    global payloadsize
    global databuffer
    operationNames = ["REQUEST ALL", "WRITE", "READ"]
    operationName = operationNames[operation - 33]

    if operationName == "REQUEST ALL":
        variables[target] = (databuffer[:-1], types[databuffer[-1]])
    messageBuffer[(operation, target)] = databuffer


def processByte(char):
    global databuffer
    global payloadsize
    global payloadLeft
    global crc
    global status
    global operation
    global target

    if status == WAITING_HEADER:
        if char == '<':
            status = WAITING_OPERATION
            crc = 0 ^ ord('<')
        else:
            sys.stdout.write(char)

    elif status == WAITING_OPERATION:
        operation = ord(char)

        if operation in (REQUEST_ALL, READ, WRITE):
            status = WAITING_TARGET
            crc = crc ^ operation
            # print "Op = ", operation
        else:
            print "bad operation!", operation
            status = WAITING_HEADER

    elif status == WAITING_TARGET:
        target = ord(char)
        if target in range(50):  # bad validation
            status = WAITING_PAYLOAD
            crc = crc ^ target

    elif status == WAITING_PAYLOAD:
        payloadsize = ord(char)
        payloadLeft = payloadsize

        databuffer = bytearray()
        crc = crc ^ payloadsize
        status = WAITING_DATA

    elif status == WAITING_DATA:
        if payloadLeft > 0:
            databuffer += char
            crc = crc ^ ord(char)
            payloadLeft -= 1
        if payloadLeft == 0:
            status = WAITING_CRC

    elif status == WAITING_CRC:
        if crc == ord(char):
            processMessage()
        else:
            print "bad crc!", crc, ord(char)
        status = WAITING_HEADER


time.sleep(1)

while len(messageBuffer.keys()) == 0:
    packu8(REQUEST_ALL, 0, [])

    for char in ser.readall():
        processByte(char)

while True:
    # blinking the led, writing, and reading the written data.
    print "on"
    packu8(WRITE, 0, [200])
    time.sleep(1)
    packu8(READ, 0, [0])
    data = waitForMsg(READ, 0)
    time.sleep(1)

    for char in ser.readall():
        processByte(char)

    print "off"
    packu8(WRITE, 0, [0])
    time.sleep(1)
    packu8(READ, 0, [0])
    time.sleep(1)

    for char in ser.readall():
        processByte(char)
