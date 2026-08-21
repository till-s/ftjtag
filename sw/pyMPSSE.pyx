# cython: c_string_type=unicode, c_string_encoding=utf8
from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stdint   cimport *
from libc.stdio    cimport *
cdef extern from "MPSSE.hpp" namespace "ftdi":
	cppclass MPSSE:
		MPSSE(const string &, uint8_t msk) except+
		MPSSE(unsigned, uint8_t msk) except+
		void write(const vector[uint8_t] &) except+
		void read(vector[uint8_t] &, size_t l) except+
		uint32_t readable() except+
		void purge() except+
		void loopback(bool) except+

cdef extern from "MPSSE.hpp" namespace "ftdi":
	void c_printInfoList "ftdi::MPSSE::printInfoList" (FILE *f) except+

def printInfoList():
	c_printInfoList(NULL)

cdef class PyMPSSE:
	cdef MPSSE *c_mpsse

	def __cinit__(self, ident, msk):
		cdef string sn
		cdef uint8_t c_msk
		cdef unsigned idx
		c_msk = msk
		if isinstance(ident, str):
			sn = ident;
			self.c_mpsse = new MPSSE(sn, c_msk)
		else:
			idx = ident
			self.c_mpsse = new MPSSE(idx, c_msk)

	def __dealloc__(self):
		del self.c_mpsse

	def write(self, bytes buf):
		self.c_mpsse.write(buf)

	def read(self):
		cdef vector[uint8_t] buf
		self.c_mpsse.read(buf, 0)
		return buf

	def mustRead(self, l):
		cdef vector[uint8_t] buf
		self.c_mpsse.read(buf, l)
		return buf

	def readable(self):
		return self.c_mpsse.readable()

	def purge(self):
		self.c_mpsse.purge()

	def loopback(self, on):
		self.c_mpsse.loopback(on)
