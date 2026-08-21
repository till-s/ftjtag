#pragma once
#include <fwComm.h>

#include <cinttypes>
#include <vector>

namespace ftemul {

using Bytes = std::vector<uint8_t>;

class FW {
  FWInfo *fw_;
  int    dbg_{0};
public:
  FW(const char *devnm, int dbg = 0);

  void printVersion();

  void x(const Bytes &req, Bytes &rep, bool tms = false);

  // 'bits': # of bits to use from last byte (zero-based)
  // 'tms' : > 0 -> shift TMS with TDI = 1
  //         = 0 -> shift TMS with TDI = 0
  //         < 0 -> shift TDI
  ssize_t ft(const uint8_t *tbuf, size_t tsiz, int bits, int tms = -1, uint8_t *rbuf = nullptr, size_t rsiz = 0);

  ssize_t ft(const uint8_t *tbuf, size_t tsiz, uint8_t *rbuf = nullptr, size_t rsiz = 0)
  {
	  return ft(tbuf, tsiz, 7, -1, rbuf, rsiz);
  }

  // each byte in TBUF contains
  //  bit(0): tms
  //  bit(1): tdi
  //  bit(2): xxx
  //  bit(3): 0  (will be used for tck)
  //
  //  tdo is returned in rbuf bit(2)
  void bb(const uint8_t *tbuf, uint8_t *rbuf, size_t bufsz);

  // bit-assignment std. ftdi: 0->tck, 1->tdi, 2->tdo, 3->tms
  void setPortLevels(uint8_t dat);

  void toStateReset();

  void toStateRunTestIdle();

  void toStateShiftIR(bool toRTIFirst = true);

  unsigned countChainLength();

  void toStateShiftDR(bool toRTIFirst = true);

  ssize_t shiftToRunTestIdle(const uint8_t *tbuf, size_t tsiz, int bits, uint8_t *rbuf, size_t rsiz);

  void getIDs(std::vector<uint32_t> &ids, unsigned nDevs = 1);

  ~FW();
};

} // namespace ftemul
