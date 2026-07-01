#include <FTEmul.hpp>

#include <stdexcept>
#include <system_error>

#include <termios.h>

namespace ftemul {

namespace {
	static constexpr const uint8_t CMD_JTAG   = 0x03;
	static constexpr const uint8_t CMD_TMS    = 0x10;
	static constexpr const uint8_t TMS_TDI_HI = 0x80;
}

FW::FW(const char *devnm)
{
	if ( ! ( fw_ = fw_open(devnm, B115200) ) ) {
		throw std::runtime_error("unable to open device");
	}
}

void
FW::printVersion()
{
	printf("GIT 0x%08" PRIx32 "\n", fw_get_version(fw_));
	printf("BRD 0x%02" PRIx8  "\n", fw_get_board_version(fw_));
	printf("API 0x%02" PRIx8  "\n", fw_get_api_version(fw_));
	printf("FUN 0x%02" PRIx8  "\n", fw_get_api_function(fw_));
}

void
FW::x(const Bytes &req, Bytes &rep, bool tms)
{
	int st;
	tbufvec tvec[1];
	rbufvec rvec[1];
	tvec[0].buf = &req[0];
	tvec[0].len = req.size();
	rvec[0].buf = &rep[0];
	rvec[0].len = rep.size();
	if ( (st = fw_xfer_vec(fw_, (tms ? (CMD_JTAG | CMD_TMS) : CMD_JTAG), tvec, req.size() > 0 ? 1 : 0, rvec, rep.size() > 0 ? 1 : 0)) < 0 ) {
		throw std::system_error(-st, std::generic_category(), "fw_xfer failed");
	}
	rep.resize(st);
	printf("transferred %d\n", st);
	for (int i = 0; i < st; ++i ) {
		printf("0x%02x\n", rep[i]);
	}
}

void
FW::toStateReset()
{
	Bytes req, rep;
	// RESET
	req.push_back(0x07);
	req.push_back(0xff);
	x(req, rep, true);
}

void
FW::toStateShiftIR(bool resetFirst)
{
	if ( resetFirst ) {
		toStateReset();
	}
	Bytes req, rep;
	// goto SHIFT-IR
	req.push_back(0x04);
	req.push_back(0x06);
	x(req, rep, true);
}

unsigned
FW::countChainLength()
{
	toStateShiftIR();

	Bytes req, rep;
	// shift ones into IR (bypass);
	req.push_back(0x07);
	req.push_back(0xff);
	req.push_back(0xff);
	req.push_back(0xff);
	req.push_back(0xff);
	x(req, rep);

	req.clear();
	// Exit1-IR, shifting a last '1', then go to Shift_DR
	req.push_back(0x84);
	req.push_back(0x07);
	x(req, rep, true);

	req.clear();
	// shift 8 zeros and 8 ones into DR
	req.push_back(0x07);
	req.push_back(0x00);
	req.push_back(0xff);
	rep.resize(req.size());
	x(req, rep, false);

	unsigned z = 0;
	for ( auto it = rep.begin(); it != rep.end(); ++it ) {
		uint16_t v = *it | 0x100;
		while ( ! (v&1) ) {
			z++;
			v >>= 1;
		}
	}
	return z - 8;
}

void
FW::toStateShiftDR(bool resetFirst)
{
	if ( resetFirst ) {
		toStateReset();
	}
	Bytes req, rep;
	req.push_back(0x03);
	req.push_back(0x02);
	x(req,rep,true);
}

void
FW::getIDs(std::vector<uint32_t> &ids, unsigned nDevs)
{
	ids.clear();
	if ( 0 == nDevs ) {
		nDevs = countChainLength();
	}
	toStateShiftDR(); // test-logic-reset loads IDCODE into IR
	Bytes req(1 + sizeof(uint32_t) * nDevs);
	req[0] = 0x07;
	x(req, req);
	uint32_t v       = 0;
	int      byteCnt = 0;
	for ( auto it = req.begin(); it != req.end(); ++it ) {
		v = (v >> 8) | (*it << 24);
		if ( 4 == ++byteCnt ) {
			ids.push_back(v);
			byteCnt = 0;
		}
	}
}

ssize_t
FW::ft(const uint8_t *tbuf, size_t tsiz, int bits, int tms, uint8_t *rbuf, size_t rsiz)
{
	uint8_t cmd = CMD_JTAG;
	uint8_t len = (bits & 0x07);
	if ( tms >= 0 ) {
		cmd |= CMD_TMS;
		if ( tms > 0 ) {
			len |= TMS_TDI_HI;
		}
	}
	tbufvec tvec[2];
	rbufvec rvec[1];
	size_t  tveclen = 0;
	size_t  rveclen = 0;
	tvec[tveclen].buf = &len;
	tvec[tveclen].len = sizeof(len);
	tveclen++;
	if ( tbuf && tsiz > 0 ) {
		tvec[tveclen].buf = tbuf;
		tvec[tveclen].len = tsiz;
		tveclen++;
	}
	if ( rbuf && rsiz > 0 ) {
		rvec[rveclen].buf = rbuf;
		rvec[rveclen].len = rsiz;
		rveclen++;
	}
	int got;
	if ( (got = fw_xfer_vec(fw_, cmd, tvec, tveclen, rvec, rveclen)) < 0 ) {
		throw std::system_error(-got, std::generic_category(), "fw_xfer failed");
	}
	return got;
}


FW::~FW()
{
	fw_close( fw_ );
}

} // namespace ftemul
