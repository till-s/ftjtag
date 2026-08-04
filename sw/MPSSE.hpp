#pragma once

#include <ftd2xx.h>

#include <sys/types.h>
#include <cinttypes>
#include <vector>
#include <string>
#include <stdexcept>

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
	MPSSE(const std::string &serialNumber, uint8_t dirMask);
	MPSSE(unsigned idx, uint8_t dirMask);

	virtual uint32_t readable();

	virtual void purge();

	virtual void read(Bytes &);
	virtual void write(const Bytes &);

	virtual void loopback(bool);

	virtual ~MPSSE();
};

} // namespace ftdi
