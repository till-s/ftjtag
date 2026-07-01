#pragma once
#include <fwComm.h>

#include <cinttypes>
#include <vector>

namespace ftemul {

using Bytes = std::vector<uint8_t>;

class FW {
  FWInfo *fw_;
public:
  FW(const char *devnm);

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

  void toStateReset();

  void toStateShiftIR(bool resetFirst = true);

  unsigned countChainLength();

  void toStateShiftDR(bool resetFirst = true);

  void getIDs(std::vector<uint32_t> &ids, unsigned nDevs = 1);

  ~FW();
};

} // namespace ftemul
