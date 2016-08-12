import serial
import time
import sys

class SerialTester():

    WAITING_HEADER = 0     # // <
    WAITING_OPERATION = 1  # // request_All, read, write
    WAITING_TARGET = 2     # // 0-255. variable register
    WAITING_PAYLOAD = 3    # // 0-255. data bytes to receive
    WAITING_DATA = 4       # // data itself
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
             "_float"]


    testValues = {"_uint8_t" : [0, 255],
             "_uint16_t" : [0, 2352],
             "_uint32_t" : [0, 2325],
             "_int8_t" : [-120, 0, 120],
             "_int16_t" : [-20000, 0, 20000],
             "_int32_t" : [-(2**30), (2**30)],
             "_float": [-0.16, 34.12]
    }


    variables = {}

    messageBuffer = {}

    def __init__(self):
        self.ser = serial.Serial(port='/dev/ttyUSB0',
                                 baudrate=115200,
                                 timeout=0.01)
        self.byte_buffer = bytearray()

    def serialize8(self, a):
        if isinstance(a, int):
            a = chr(a)
        self.byte_buffer += a


    def unpack(self, a, vtype):
        if vtype == "_uint8_t":
            return a
        elif vtype == "_uint16_t":
            return [(a) & 0xFF, (a >> 8) & 0xFF]
        elif vtype == "_uint32_t":
            return [(a) & 0xFF, (a >> 8) & 0xFF, (a >> 16) & 0xFF, (a >> 24) & 0xFF]
        elif vtype == "_int8_t":
            return a
        elif vtype == "_int16_t":
            return [(a) & 0xFF, (a >> 8) & 0xFF]
        elif vtype == "_int32_t":
            return [(a) & 0xFF, (a >> 8) & 0xFF, (a >> 16) & 0xFF, (a >> 24) & 0xFF]
        elif vtype == "_float":
            b = struct.pack('<f', a)
            return [b[i] for i in xrange(0,4)]
        return


    def unpack32(self, a):
        return

    # def serializeFloat(a):
    #     b = struct.pack('<f', a)
    #     for x in xrange(0, 4):
    #         serialize8(b[x])

    def send_16(self, value):
        high = chr(value >> 8)
        low = chr(value % 256)
        self.ser.write(low)
        self.ser.write(high)

    def send_8(self, value):
        self.ser.write(chr(value))

    def packu8(self, operation, target=None, data=None):
        self.byte_buffer = bytearray()
        header = ord('<')
        self.serialize8(header)
        self.serialize8(operation)
        crc = header ^ operation

        if target is not None:
            self.serialize8(target)
            crc = crc ^ target

        if data is not None:
            if type(data) is int:
                data = [data]

            size = len(data)

            self.serialize8(size)
            crc = crc ^ size
            for item in data:
                crc = crc ^ item
                self.serialize8(item)

        self.serialize8(crc)
        self.ser.write(self.byte_buffer)


    def repack(self, data, vtype):
        if vtype == "_uint8_t":
            return data
        elif vtype == "_uint16_t":
            return data[0] + (data[1] << 8)
        elif vtype == "_uint32_t":
            return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)
        elif vtype == "_int8_t":
            return data
        elif vtype == "_int16_t":
            return data[0] + (data[1] << 8)
        elif vtype == "_int32_t":
            return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24)
        elif vtype == "_float":
            b = struct.pack('<f', a)
            return [b[i] for i in xrange(0,4)]


    def waitForMsg(self, op, target, timeout=0.2):
        self.messageBuffer.pop((op, target), None)
        start = time.time()
        while (op, target) not in self.messageBuffer.keys():
            for char in self.ser.readall():
                self.processByte(char)
            time.sleep(0.001)
            if (time.time() - start) > timeout:
                return None
        data = self.messageBuffer.pop((op, target), None)
        return self.repack(data, self.variables[target][1])


    def processMessage(self):
        operationNames = ["REQUEST ALL", "WRITE", "READ"]
        operationName = operationNames[self.operation - 33]

        if operationName == "REQUEST ALL":
            self.variables[self.target] = (self.databuffer[:-1], self.types[self.databuffer[-1]])

        if len(self.databuffer) == 1:
            self.messageBuffer[(self.operation, self.target)] = self.databuffer[0]
        else:
            self.messageBuffer[(self.operation, self.target)] = self.databuffer


    def processByte(self, char):
        if self.status == self.WAITING_HEADER:
            if char == '<':
                self.status = self.WAITING_OPERATION
                self.crc = 0 ^ ord('<')
            else:
                sys.stdout.write(char)

        elif self.status == self.WAITING_OPERATION:
            self.operation = ord(char)

            if self.operation in (self.REQUEST_ALL, self.READ, self.WRITE):
                self.status = self.WAITING_TARGET
                self.crc ^= self.operation
                # print "Op = ", operation
            else:
                print "bad operation!", self.operation
                self.status = self.WAITING_HEADER

        elif self.status == self.WAITING_TARGET:
            self.target = ord(char)
            if self.target in range(50):  # bad validation
                self.status = self.WAITING_PAYLOAD
                self.crc ^= self.target

        elif self.status == self.WAITING_PAYLOAD:
            self.payloadsize = ord(char)
            self.payloadLeft = self.payloadsize

            self.databuffer = bytearray()
            self.crc ^= self.payloadsize
            self.status = self.WAITING_DATA

        elif self.status == self.WAITING_DATA:
            if self.payloadLeft > 0:
                self.databuffer += char
                self.crc ^= ord(char)
                self.payloadLeft -= 1
            if self.payloadLeft == 0:
                self.status = self.WAITING_CRC

        elif self.status == self.WAITING_CRC:
            if self.crc == ord(char):
                self.processMessage()
            else:
                print "bad crc!", self.crc, ord(char)
            self.status = self.WAITING_HEADER


def blink(tester):
    # blinking the led, writing, and reading the written data.
    print "Turning on, writing 200..."
    tester.packu8(tester.WRITE, 0, [200])
    time.sleep(1)
    tester.packu8(tester.READ, 0, [0])
    print "Reading back..."
    read = tester.waitForMsg(tester.READ, 0)
    print "read: ", read
    if read == 200:
        print "OK!"

    for char in tester.ser.readall():
        tester.processByte(char)

    print "Turning off, writing 0..."
    tester.packu8(tester.WRITE, 0, [0])
    time.sleep(1)
    tester.packu8(tester.READ, 0, [0])
    print "Reading back..."
    read = tester.waitForMsg(tester.READ, 0)
    print "read: ", read

    if read == 0:
        print "OK!"

    for char in tester.ser.readall():
        tester.processByte(char)


tester = SerialTester()

while len(tester.variables) == 0:
    tester.packu8(tester.REQUEST_ALL, 0, [200])
    tester.waitForMsg(tester.REQUEST_ALL, 0)


for key, value in tester.variables.iteritems():
    print key, value
for index, var in tester.variables.iteritems():
    name, vartype = var
    varRange = tester.testValues[vartype]
    for value in varRange:
        print "sent: ", value, tester.unpack(value, vartype), vartype
        tester.packu8(tester.WRITE, index, tester.unpack(value, vartype))
        tester.packu8(tester.READ, index, [0])

        print "received", tester.waitForMsg(tester.READ, index)
