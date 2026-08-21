import pyMPSSE

class MPJTAG(object):
    def __init__(self, desc="TS9D9HD5B", inival=0xb):
        self.mpsse = pyMPSSE.PyMPSSE(desc, inival)

    @staticmethod
    def printInfoList():
        PyMPSSE.printInfoList()

    # reset and move to run-test-idle
    def toRTI(self):
        self.write(bytes([0x4b,6,0x3f]))

    # to shift IR (from run-test-idle)
    def toSIR(self):
        self.write(bytes([0x4b,3,0x03]))

    # to shift DR (from run-test-idle)
    def toSDR(self):
        self.write(bytes([0x4b,2,0x01]))

    #lst: number of bits in last byte (1..8)
    # from shift_dr or shift_ir; leaves in run_test_idle
    def _shift(self,b,lst=8):
        if ( isinstance(b, list) ):
            b=bytes(b)
        if ( lst < 1 or lst > 8 ):
            raise RuntimeError("bit count must be between 1..8")
        a         = bytes()
        lastibyte = 0x00
        if ( len(b) > 1 ):
            length = len(b) - 2
            self.write(bytes([0x39,(length & 0xff), (length >> 8) & 0xff])+b[:-1])
            a = bytes(self.mpsse.mustRead(len(b)-1))
        lastobyte=b[-1]
        if ( lst > 1 ):
            self.write(bytes([0x3b,lst-2,lastobyte]))
            lastibyte=self.mpsse.mustRead(1)[0]
            lastobyte >>= lst - 1
            lastibyte >>= 1
        lastobyte <<= 7
        self.write(bytes([0x6b,2, lastobyte | 0x03]))
        lastibyte = (lastibyte | (self.mpsse.mustRead(1)[0] & 0x80))
        lastibyte >>= (8-lst)
        return a + bytes([lastibyte])

    def shiftIR(self, b, lst=8):
        self.toSIR()
        return self._shift(b, lst)

    def shiftDR(self, b, lst=8):
        self.toSDR()
        return self._shift(b, lst)

    def write(self, b):
        self.mpsse.write(b)

    def read(self):
        return self.mpsse.read()

    def readable(self):
        return self.mpsse.readable()

class H19S(MPJTAG):
    def __init__(self, desc, inival=0xb):
        super().__init__(desc, inival)
        self.inst = 0xff
        self.toRTI()

    def shiftIR(self, b, lst=8):
        rv = super().shiftIR(b, lst)
        self.inst = rv[0]
        return rv

    def getInst(self):
        return self.inst

    def er1(self):
        return self.shiftIR([0x32])

    def er2(self):
        return self.shiftIR([0x38])

#m=H19S(2)
