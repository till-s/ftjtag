# cython: c_string_type=unicode, c_string_encoding=utf8
from libcpp.string cimport string
from libcpp.vector cimport vector
from libcpp        cimport *
from libc.stdint   cimport *
from libc.stdio    cimport *

cdef extern from "FTEmul.hpp" namespace "ftemul":
	cppclass FW:
		FW(const char *devnm, int dbg) except+
		void toStateReset() except+
		void toStateRunTestIdle() except+
		void toStateShiftIR(bool resetFirst) except+
		void toStateShiftDR(bool resetFirst) except+
		ssize_t ft(const uint8_t *buf, size_t tsiz, int bits, int tms, uint8_t *rbuf, size_t rsize) except+
		ssize_t shiftToRunTestIdle(const uint8_t *tbuf, size_t tsiz, int bits, uint8_t *rbuf, size_t rsiz) except+

cdef class PyFTEmul:
	cdef FW *c_fw

	def __init__(self, ttynam, dbg = 0):
		self.c_fw = new FW(ttynam, dbg)
		self.c_fw.toStateRunTestIdle()

	def __dealloc__(self):
		del self.c_fw

	def toStateReset(self):
		self.c_fw.toStateReset()

	def toStateRunTestIdle(self):
		self.c_fw.toStateRunTestIdle()

	def _shift(self, bytes tx, lastbits = 8):
		cdef vector[uint8_t] rbuf;
		if ( lastbits < 1 or lastbits > 8 ):
			raise RuntimeError("_shift: lastbits not in 1..8");
		rbuf.resize(len(tx))
		cdef int status
		status = self.c_fw.shiftToRunTestIdle(tx, len(tx), lastbits - 1, &rbuf[0], len(tx))
		if ( status < 0 ):
			raise RuntimeError("_shift: shiftToRunTestIdle failed: {:d}".format(status))
		return rbuf;

	def _ft(self, bytes tx, rlen = 0, lastbits = 8, tms = -1):
		cdef vector[uint8_t] rbuf
		cdef uint8_t        *rptr
		cdef int             status
		if ( lastbits < 1 or lastbits > 8 ):
			raise RuntimeError("_shift: lastbits not in 1..8");
		if ( rlen > 0 ):
			rbuf.resize(rlen)
			rptr = &rbuf[0]
		else:
			rptr = NULL
		status = self.c_fw.ft(tx, len(tx), lastbits - 1, tms, rptr, rlen)
		if ( status < 0 ):
			raise RuntimeError("_ft: {:d}".format(status))
		return rbuf;

	def shiftIR(self, bytes tx, lastbits = 8):
		self.c_fw.toStateShiftIR(False)
		return self._shift(tx, lastbits);

	def shiftDR(self, bytes tx, lastbits = 8):
		self.c_fw.toStateShiftDR(False)
		return self._shift(tx, lastbits);
