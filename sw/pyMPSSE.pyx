# cython: c_string_type=unicode, c_string_encoding=utf8
from libcpp.string cimport string
from libcpp.vector cimport vector
from libc.stdint cimport *
cdef extern from "MPSSE.hpp" namespace "ftdi":
	cppclass MPSSE:
		MPSSE(const string &, uint8_t msk) except+
		MPSSE(unsigned, uint8_t msk) except+
		void write(const vector[uint8_t] &) except+
		void read(vector[uint8_t] &) except+
		uint32_t readable() except+
		void purge() except+
		void loopback(bool) except+

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
		self.c_mpsse.read(buf)
		return buf

	def readable(self):
		return self.c_mpsse.readable()

	def purge(self):
		self.c_mpsse.purge()

	def loopback(self, on):
		self.c_mpsse.loopback(on)
