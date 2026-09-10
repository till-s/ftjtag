#pragma once
#include <fwComm.h>

#include <cinttypes>
#include <vector>
#include <memory>

namespace ftemul {

using Bytes = std::vector<uint8_t>;

// base class defining fundamental operations
class FTStream {
 protected:
  int                                  dbg_{0};
 public:

  virtual ssize_t xfer(uint8_t cmd, const tbufvec *tbuf, size_t tbuflen, const rbufvec *rbuf, size_t rbuflen) = 0;

  // debug flags can be ORed together
  static constexpr int DEBUG_FT = (1<<0);

  virtual void setDebug(int mask) { dbg_ = mask; }

  // basic transfers
  virtual void x(const Bytes &req, Bytes &rep, bool tms = false);

  // 'bits': # of bits to use from last byte (zero-based)
  // 'tms' : > 0 -> shift TMS with TDI = 1
  //         = 0 -> shift TMS with TDI = 0
  //         < 0 -> shift TDI
  virtual ssize_t ft(const uint8_t *tbuf, size_t tsiz, int bits, int tms = -1, uint8_t *rbuf = nullptr, size_t rsiz = 0);

  virtual ssize_t ft(const uint8_t *tbuf, size_t tsiz, uint8_t *rbuf = nullptr, size_t rsiz = 0)
  {
	  return ft(tbuf, tsiz, 7, -1, rbuf, rsiz);
  }

  // bit-assignment std. ftdi: 0->tck, 1->tdi, 2->tdo, 3->tms
  virtual void setPortLevels(uint8_t dat);

  virtual void toStateReset();

  virtual void toStateRunTestIdle();

  virtual void toStateShiftIR(bool toRTIFirst = true);

  virtual unsigned countChainLength();

  virtual void toStateShiftDR(bool toRTIFirst = true);

  virtual ssize_t shiftToRunTestIdle(const uint8_t *tbuf, size_t tsiz, int bits, uint8_t *rbuf, size_t rsiz);

  virtual void getIDs(std::vector<uint32_t> &ids, unsigned nDevs = 1);

  virtual ~FTStream() = default;
};

// Full-firmware with command mux
//
struct FWDeleter {
	void operator()(FWInfo *);
};

class FW : public FTStream {
  std::unique_ptr<FWInfo,FWDeleter> fw_;
public:
  static constexpr int DEBUG_FW = (1<<4);

  FW(const char *devnm);

  virtual void printVersion();

  // each byte in TBUF contains
  //  bit(0): tms
  //  bit(1): tdi
  //  bit(2): xxx
  //  bit(3): 0  (will be used for tck)
  //
  //  tdo is returned in rbuf bit(2)
  virtual void bb(const uint8_t *tbuf, uint8_t *rbuf, size_t bufsz);

  virtual void setDebug(int msk) override;

  virtual ssize_t xfer(uint8_t cmd, const tbufvec *tvec, size_t tveclen, const rbufvec *rvec, size_t rveclen) override;
};

struct FifoDeleter {
	void operator()(CmdFifo);
};

// Raw Fifo supporting only JTAG
class RawFifo : public FTStream {
  std::unique_ptr<CmdFifoRec,FifoDeleter> fifo_;
  const uint8_t                        addr_;
  static constexpr const uint8_t       ADDR_MASK = 0x0f;
public:
  struct Config {
	  uint8_t addr;
	  bool    cobs;
	  size_t  windowSize;

	  Config() : addr (0x00), cobs (true), windowSize(0) {}
  };
  RawFifo(const char *devnm, const struct Config &cfg = Config());

  virtual ssize_t xfer(uint8_t cmd, const tbufvec *tvec, size_t tveclen, const rbufvec *rvec, size_t rveclen) override;
};

} // namespace ftemul
