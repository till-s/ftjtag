#include <MPSSE.hpp>
#include <cstdio>
#include <cstring>
#include <vector>
#include <stdexcept>

using std::vector;

namespace ftdi {

std::string
FTError::toString(int code)
{
	const char *rv;
	switch ( code ) {
		case FT_OK:
			rv = "FT_OK";
		break;
		case FT_INVALID_HANDLE:
			rv = "FT_INVALID_HANDLE";
		break;
		case FT_DEVICE_NOT_FOUND:
			rv = "FT_DEVICE_NOT_FOUND";
		break;
		case FT_DEVICE_NOT_OPENED:
			rv = "FT_DEVICE_NOT_OPENED";
		break;
		case FT_IO_ERROR:
			rv = "FT_IO_ERROR";
		break;
		case FT_INSUFFICIENT_RESOURCES:
			rv = "FT_INSUFFICIENT_RESOURCES";
		break;
		case FT_INVALID_PARAMETER:
			rv = "FT_INVALID_PARAMETER";
		break;
		case FT_INVALID_BAUD_RATE:
			rv = "FT_INVALID_BAUD_RATE";
		break;
		case FT_DEVICE_NOT_OPENED_FOR_ERASE:
			rv = "FT_DEVICE_NOT_OPENED_FOR_ERASE";
		break;
		case FT_DEVICE_NOT_OPENED_FOR_WRITE:
			rv = "FT_DEVICE_NOT_OPENED_FOR_WRITE";
		break;
		case FT_FAILED_TO_WRITE_DEVICE:
			rv = "FT_FAILED_TO_WRITE_DEVICE";
		break;
		case FT_EEPROM_READ_FAILED:
			rv = "FT_EEPROM_READ_FAILED";
		break;
		case FT_EEPROM_WRITE_FAILED:
			rv = "FT_EEPROM_WRITE_FAILED";
		break;
		case FT_EEPROM_ERASE_FAILED:
			rv = "FT_EEPROM_ERASE_FAILED";
		break;
		case FT_EEPROM_NOT_PRESENT:
			rv = "FT_EEPROM_NOT_PRESENT";
		break;
		case FT_EEPROM_NOT_PROGRAMMED:
			rv = "FT_EEPROM_NOT_PROGRAMMED";
		break;
		case FT_INVALID_ARGS:
			rv = "FT_INVALID_ARGS";
		break;
		case FT_NOT_SUPPORTED:
			rv = "FT_NOT_SUPPORTED";
		break;
		case FT_OTHER_ERROR:
			rv = "FT_OTHER_ERROR";
		break;
		case FT_DEVICE_LIST_NOT_READY:
			rv = "FT_DEVICE_LIST_NOT_READY";
		break;
		default:
			rv = "FT_UNKNOWN_ERROR";
		break;
	}
	return rv;
}

FTError::FTError(const std::string &msg, int status)
: std::runtime_error(msg + toString(status) )
{
}

FTError::FTError(const std::string &msg)
: std::runtime_error(msg )
{
}

namespace {

struct Init {
	FT_HANDLE ft_;
	bool      own_{false};

	Init(FT_HANDLE h) : ft_(h)
	{
	}

	FT_HANDLE
	operator()(uint8_t msk = 0x0b)
	{
		FT_STATUS st;
		st = FT_ResetDevice(ft_);
		if ( FT_OK != st ) {
			throw FTError("FT_ResetDevice failed: ", st);
		}
		st = FT_SetChars(ft_, false, 0, false, 0);
		if ( FT_OK != st ) {
			throw FTError("FT_SetChars failed: ", st);
		}
		st = FT_SetLatencyTimer(ft_, 16);
		if ( FT_OK != st ) {
			throw FTError("FT_LatencyTimer failed: ", st);
		}
		st = FT_SetBitMode(ft_, msk, FT_BITMODE_RESET);
		if ( FT_OK != st ) {
			throw FTError("FT_SetBitMode failed: ", st);
		}
		st = FT_SetBitMode(ft_, msk, FT_BITMODE_MPSSE);
		if ( FT_OK != st ) {
			throw FTError("FT_SetBitMode failed: ", st);
		}
		MPSSE::Bytes config({
			0x80, 0x00, msk, // config pin initial value and direction
		});
		uint32_t put;
		st = FT_Write(ft_, &config[0], config.size(), &put);
		if ( FT_OK != st ) {
			throw FTError("FT_Write failed: ", st);
		}
		if ( put != config.size() ) {
			throw FTError("incomplete write");
		}
		own_ = false;
		return ft_;
	}
	

	~Init()
	{
		if ( own_ ) {
			FT_Close(ft_);
		}
	}
};

}

MPSSE::MPSSE(const std::string &serialNumber, const std::string &description, uint8_t dirMask)
{
	FT_STATUS st;
	FT_HANDLE ft;
	DWORD     numDevices;
        st = FT_CreateDeviceInfoList( &numDevices );
	if ( FT_OK != st ) {
		throw FTError("FT_CreateDeviceInfoList failed: ", st);
	}
	vector<FT_DEVICE_LIST_INFO_NODE> l;
	l.resize(numDevices);

        st = FT_GetDeviceInfoList(&l[0], &numDevices);
	if ( FT_OK != st ) {
		throw FTError("FT_GetDeviceInfoList failed: ", st);
	}
	l.resize(numDevices); // just in case

	DWORD locId = -1;
	for ( auto it = l.begin(); it != l.end(); ++it ) {
		if ( 0 == strncmp( serialNumber.c_str(), (*it).SerialNumber, sizeof((*it).SerialNumber) ) ) {
			if ( 0 == description.size() || strstr( (*it).Description, description.c_str() ) ) {
				locId = (*it).LocId;
			}
		}
	}
	if ( -1 == locId ) {
		throw std::runtime_error("Requested unit not found!");
	}

	st = FT_OpenEx( reinterpret_cast<DWORD*>(locId), FT_OPEN_BY_LOCATION, &ft);
	if ( FT_OK != st ) {
		throw FTError("FT_OpenEx failed: ", st);
	}
	Init init(ft);
	ft_ = init(dirMask);
}

MPSSE::MPSSE(unsigned idx, uint8_t dirMask)
{
	FT_HANDLE ft;
	FT_STATUS st = FT_Open(idx, &ft);
	if ( FT_OK != st ) {
		throw FTError("FT_Open failed: ", st);
	}
	Init init(ft);
	ft_ = init(dirMask);
}

void
MPSSE::loopback(bool on)
{
	static constexpr const uint8_t LOOPBACK_ON  = 0x84;
	static constexpr const uint8_t LOOPBACK_OFF = 0x85;
	Bytes b({(on ? LOOPBACK_ON : LOOPBACK_OFF)});
	write(b);
}

uint32_t
MPSSE::readable()
{
	uint32_t have;
	FT_STATUS st = FT_GetQueueStatus(ft_, &have);
	if ( FT_OK != st ) {
		throw FTError("FT_GetQueueStatus failed: ", st);
	}
	return have;
}

void
MPSSE::read(Bytes &buf)
{
	uint32_t got = readable();

	FT_STATUS st;

	buf.resize(got);

	if ( got > 0 ) {
		st = FT_Read(ft_, &buf[0], got, &got);
		if ( FT_OK != st ) {
			throw FTError("FT_Read failed: ", st);
		}
	}

	buf.resize(got);
}

void
MPSSE::purge()
{
	FT_STATUS st = FT_Purge(ft_, (FT_PURGE_RX | FT_PURGE_TX));
	if ( FT_OK != st ) {
		throw FTError("FT_Purge failed: ", st);
	}
}

void
MPSSE::write(const Bytes &buf)
{
	uint32_t put;
	FT_STATUS st = FT_Write(ft_, const_cast<uint8_t*>( &buf[0] ), buf.size(), &put);
	if ( FT_OK != st ) {
		throw FTError("FT_Read failed: ", st);
	}
	if ( put != buf.size() ) {
		throw FTError("FT_Write incomplete - not all data written");
	}
}

MPSSE::~MPSSE()
{
	printf("CLOSSE\n");
	FT_Close(ft_);
}


} // namespace FTDI
