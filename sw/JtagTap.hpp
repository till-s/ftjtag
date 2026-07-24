#pragma once

#include <cstdint>
#include <vector>
#include <string>

class JtagTap {
  public:
    enum class State {
      TestLogicReset, RunTestIdle,
      SelectDRScan, CaptureDR, ShiftDR, Exit1DR, PauseDR, Exit2DR, UpdateDR,
      SelectIRScan, CaptureIR, ShiftIR, Exit1IR, PauseIR, Exit2IR, UpdateIR
    };

  private:
    State state_;
    uint64_t             ir_;
    unsigned             irLen_;
    std::vector<uint8_t> dri_;
    std::vector<uint8_t> dro_;
    uint8_t              drRemBits_ {8};
    uint8_t              irBits_ {0};

    static constexpr const uint64_t BYPASS = static_cast<uint64_t>(-1LL);

  public:
    JtagTap(unsigned irLen = 0);

    virtual void
    updateDR(const std::vector<uint8_t> &dri, const std::vector<uint8_t> &dro, uint8_t lastBits)
    {
    }

    virtual void
    updateIR(uint64_t ir, unsigned irLen)
    {
    }

    uint64_t
    getIR()
    {
        return ir_;
    }

    State
    getState()
    {
        return state_;
    }

    const std::string &
    toString(State);

    size_t
    getDRLen();

    virtual State
    nextState(bool tms, bool tdi, bool tdo);
};
