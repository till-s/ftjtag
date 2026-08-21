#pragma once

#include <ftd2xx.h>

#include <sys/types.h>
#include <cinttypes>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdio>

namespace ftdi {

class FTError : public std::runtime_error {
public:
	FTError(const std::string &msg, int status);
	FTError(const std::string &msg);

	static std::string toString(int);
};

class MPSSE {
	FT_HANDLE ft_;
public:
	using Bytes = std::vector<uint8_t>;

	// NOTE: multi-channel devices have a channel identifier 'A', 'B'...
	//       appended to the serial number of the device. This permits
	//       specifying the exact channel with this API.
	MPSSE(const std::string &serialNumber, uint8_t dirMask);

	MPSSE(unsigned idx, uint8_t dirMask);

	static void printInfoList(FILE *f = nullptr);

	virtual uint32_t readable();

	virtual void purge();

	virtual void read(Bytes &, size_t l = 0);
	virtual size_t read(uint8_t *, size_t, size_t l = 0);
	virtual void write(const Bytes &);
	virtual void write(const uint8_t *, size_t);

	virtual void loopback(bool);

	virtual ~MPSSE();
};

} // namespace ftdi
