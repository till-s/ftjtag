#include <FTEmul.hpp>

#include <stdexcept>
#include <system_error>

#include <termios.h>
#include <vector>
#include <cstring>

namespace ftemul {

namespace {
	static constexpr const uint8_t CMD_JTAG   = 0x03;
	static constexpr const uint8_t CMD_TMS    = 0x10;
	static constexpr const uint8_t CMD_BB     = 0x20;
	static constexpr const uint8_t CMD_TDI_WO = 0x30;
	static constexpr const uint8_t CMD_TDI_RO = 0x40;
	static constexpr const uint8_t TMS_TDI_HI = 0x80;

	static constexpr const uint8_t CMD_BB_SPI = 0x14;
	static constexpr const uint8_t BB_TCK_BIT = 0x08;

}

void FWDeleter::operator()(FWInfo *fw)
{
	fw_close(fw);
}

FW::FW(const char *devnm)
{
	if ( ! ( fw_ = std::unique_ptr<FWInfo, FWDeleter>(fw_open(devnm, B115200), FWDeleter()) ) ) {
		throw std::runtime_error("unable to open device");
	}
}

void
FW::setDebug(int mask)
{
	FTStream::setDebug( mask );
	int lvl =  !!(dbg_ && DEBUG_FW) ? 2 : 0;
	fw_set_debug(fw_.get(), lvl);
}

void
FW::printVersion()
{
	printf("GIT 0x%08" PRIx32 "\n", fw_get_version(fw_.get()));
	printf("BRD 0x%02" PRIx8  "\n", fw_get_board_version(fw_.get()));
	printf("API 0x%02" PRIx8  "\n", fw_get_api_version(fw_.get()));
	printf("FUN 0x%02" PRIx8  "\n", fw_get_api_function(fw_.get()));
}

// returns #bits in 1st byte
void
FTStream::x(const Bytes &req, Bytes &rep, bool tms)
{
	int st;
	Bytes discard;
	tbufvec tvec[1];
	rbufvec rvec[1];
	tvec[0].buf = &req[0];
	tvec[0].len = req.size();
	if ( rep.size() ) {
		rvec[0].buf = &rep[0];
		rvec[0].len = rep.size();
	} else {
		discard.resize(req.size());
		rvec[0].buf = &discard[0];
		rvec[0].len = discard.size();
	}
	if ( !!(dbg_ & DEBUG_FT) ) {
		printf("sending OUT %zd\n", req.size());
		for (size_t i = 0; i < req.size(); ++i ) {
			printf("0x%02x\n", req[i]);
		}
	}
	if ( (st = xfer((tms ? (CMD_JTAG | CMD_TMS) : CMD_JTAG), tvec,  1 , rvec,  1)) < 0 ) {
		throw std::system_error(-st, std::generic_category(), "xfer failed");
	}
	rep.resize(st);
	if ( !!(dbg_ & DEBUG_FT) ) {
		printf("transferred IN %d\n", st);
		for (size_t i = 0; i < rep.size(); ++i ) {
			printf("0x%02x\n", rep[i]);
		}
	}
}

ssize_t
FW::xfer(uint8_t cmd, const tbufvec *tvec, size_t tveclen, const rbufvec *rvec, size_t rveclen)
{
	return fw_xfer_vec(fw_.get(), cmd, tvec,  tveclen , rvec, rveclen );
}

void
FTStream::toStateReset()
{
	Bytes req, rep;
	// RESET
	req.push_back(0x07);
	req.push_back(0xff);
	x(req, rep, true);
}

void
FTStream::toStateRunTestIdle()
{
	Bytes req, rep;
	// RESET -> RUN_TEST_IDLE
	req.push_back(0x07);
	req.push_back(0x7f);
	x(req, rep, true);
}


void
FTStream::toStateShiftIR(bool resetFirst)
{
	if ( resetFirst ) {
		toStateRunTestIdle();
	}
	Bytes req, rep;
	// goto SHIFT-IR
	req.push_back(0x03);
	req.push_back(0x03);
	x(req, rep, true);
}

unsigned
FTStream::countChainLength()
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
FTStream::toStateShiftDR(bool resetFirst)
{
	if ( resetFirst ) {
		toStateRunTestIdle();
	}
	Bytes req, rep;
	req.push_back(0x02);
	req.push_back(0x01);
	x(req,rep,true);
}

void
FTStream::getIDs(std::vector<uint32_t> &ids, unsigned nDevs)
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

void
FTStream::setPortLevels(const uint8_t dat)
{
	uint8_t cmd = CMD_JTAG | CMD_BB;
	uint8_t unused;
	int got;

	tbufvec tvec[1];
	rbufvec rvec[1];

	tvec[0].buf = &dat;
	tvec[0].len = sizeof(dat);

	rvec[0].buf = &unused;
	rvec[0].len = sizeof(unused);

	got = xfer(cmd, tvec, sizeof(tvec)/sizeof(tvec[0]), rvec, sizeof(rvec)/sizeof(rvec[0]));
	if ( got < 0 ) {
		throw std::system_error(-got, std::generic_category(), "fw_xfer failed");
	}
}

ssize_t
FTStream::ft(const uint8_t *tbuf, size_t tsiz, int bits, int tms, uint8_t *rbuf, size_t rsiz)
{
	if ( tsiz && rsiz && (rsiz != tsiz) ) {
		throw std::system_error(-EINVAL, std::generic_category(), "ft: tsiz/rsiz mismatch");
	}
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
	Bytes empty;
	if ( tbuf && tsiz > 0 ) {
		tvec[tveclen].buf = tbuf;
		tvec[tveclen].len = tsiz;
		tveclen++;
	} else {
		if ( 7 == bits && tms < 0 ) {
			cmd |= CMD_TDI_RO;
		} else
		{
			empty.resize(rsiz);
			tvec[tveclen].buf = &empty[0];
			tvec[tveclen].len = rsiz;
			tveclen++;
		}
	}
	if ( rbuf && rsiz > 0 ) {
		rvec[rveclen].buf = rbuf;
		rvec[rveclen].len = rsiz;
		rveclen++;
	} else {
		// empty has been used if tsiz == 0 and rsiz > 0
		// which is not possible in this branch; thus
		// we may use empty here
		if ( 7 == bits && tms < 0 ) {
			cmd |= CMD_TDI_WO;
			// a dummy byte is returned once the transfer is done
			rvec[rveclen].len = 1;
		} else {
			empty.resize(tsiz);
			rvec[rveclen].len = tsiz;
		}
		empty.resize(rvec[rveclen].len);
		rvec[rveclen].buf = &empty[0];
		rveclen++;
	}
	int got;
	if ( (got = xfer(cmd, tvec, tveclen, rvec, rveclen)) < 0 ) {
		throw std::system_error(-got, std::generic_category(), "xfer failed");
	}
	if ( !!(dbg_ & DEBUG_FT) ) {
		size_t vi;
		size_t ii;
		int    tt;
		printf("CMD: %02x\n", cmd);
		if ( tveclen ) {
			printf("TX: ");
			for ( vi = 0; vi < tveclen; ++vi ) {
				for ( ii=0; ii < tvec[vi].len; ++ii ) {
					printf("%02x ", tvec[vi].buf[ii]);
				}
			}
			printf("\n");
		}
		if ( rveclen ) {
			printf("RX: ");
			tt = 0;
			for ( vi = 0; vi < rveclen; ++vi ) {
				for ( ii=0; ii < rvec[vi].len; ++ii ) {
					if ( tt < got ) {
						printf("%02x ", rvec[vi].buf[ii]);
					}
					tt++;
				}
			}
			printf("\n");
		}
	}
	return got;
}

ssize_t
FTStream::shiftToRunTestIdle(const uint8_t *tbuf, size_t tsiz, int bits, uint8_t *rbuf, size_t rsiz)
{
	if ( tsiz && rsiz && (tsiz != rsiz) ) {
		throw std::system_error(-EINVAL, std::generic_category(), "ft: tsiz/rsiz mismatch");
	}
	if ( !tsiz && !rsiz ) {
		return 0;
	}
	size_t  tsiz1   = tsiz;
	size_t  rsiz1   = rsiz;
	int     lasttdi = (tsiz ? (tbuf[tsiz - 1] >> bits) : 0) & 1;
	if ( 0 == bits ) {
		tsiz1 = tsiz1 ? tsiz1 - 1 : 0;
		rsiz1 = rsiz1 ? rsiz1 - 1 : 0;
		bits  = 7;
	} else {
		bits--;
	}
	size_t got = ft(tbuf, tsiz1, bits, -1, rbuf, rsiz1);
	if ( got < 0 ) {
		return got;
	}
	uint8_t toRTI = 0x03; // TMS: 1->1->0
	uint8_t rxdat;
	int     txbits = 3 - 1;

	got = ft(&toRTI, 1, txbits, lasttdi, &rxdat, 1);
	if ( got < 0 ) {
		return got;
	}
	if ( rsiz ) {
	        uint8_t lastTDOBit = (rxdat << txbits) & 0x80;
		rbuf[rsiz - 1] = (lastTDOBit | (rbuf[rsiz - 1] >> 1));
	}
	return rsiz;
}

void
FW::bb(const uint8_t *tbuf, uint8_t *rbuf, size_t bufsz)
{
	std::vector<uint8_t> b;
	b.resize( 2*bufsz );
	for ( size_t i = 0; i < bufsz; ++i ) {
		b[2*i + 0] = (tbuf[i] & ~BB_TCK_BIT);
		b[2*i + 1] = (tbuf[i] |  BB_TCK_BIT);
	}
	int st = fw_xfer( fw_.get(), CMD_BB_SPI, &b[0], rbuf ? &b[0] : nullptr, b.size() );
	if ( st < 0 ) {
		throw std::system_error(-st, std::generic_category(), "fw_xfer failed");
	}
	if ( rbuf ) {
		for ( size_t i = 0; i < bufsz; ++i ) {
			rbuf[i] = b[2*i+1];
		}
	}
}

RawFifo::RawFifo(const char *devnm, const struct RawFifo::Config &config)
 : addr_( config.addr & ADDR_MASK )
{
	CmdFifoConfig cfg;
	memset( &cfg, 0, sizeof(cfg) );

	cfg.ttyName    = devnm;
	cfg.windowSize = config.windowSize;
	if ( config.cobs ) {
		cfg.codec = CMD_FIFO_CFG_CODEC_COBS;
	} else {
		cfg.codec = CMD_FIFO_CFG_CODEC_BYTESTUFF;
	}
	cfg.flags = CMD_FIFO_CFG_WINSIZE;

	CmdFifo fifo   = nullptr;
	int     status = fifoOpenConfig( &fifo, &cfg );
	if ( status < 0 ) {
		throw std::system_error(-status, std::generic_category(), "fifoOpenConfig failed");
	}
	fifo_ = std::unique_ptr<CmdFifoRec, FifoDeleter>( fifo, FifoDeleter());
}

ssize_t
RawFifo::xfer(uint8_t cmd, const tbufvec *tvec, size_t tveclen, const rbufvec *rvec, size_t rveclen)
{
	cmd |= addr_;
	return fifoXferFrameVec( fifo_.get(), &cmd, tvec, tveclen, rvec, rveclen );
}

void
FifoDeleter::operator()(CmdFifo fifo)
{
	fifoClose( fifo );
}


} // namespace ftemul
