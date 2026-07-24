#include <JtagTap.hpp>

#include <array>
#include <stdexcept>

namespace {
    std::array<const std::string, 16> stateNames({
        "TestLogicReset", "RunTestIdle",
        "SelectDRScan", "CaptureDR", "ShiftDR", "Exit1DR", "PauseDR", "Exit2DR", "UpdateDR",
        "SelectIRScan", "CaptureIR", "ShiftIR", "Exit1IR", "PauseIR", "Exit2IR", "UpdateIR"
    });
}

JtagTap::JtagTap(unsigned irLen)
: irLen_(irLen)
{
  if ( irLen > sizeof(ir_)*8 ) {
    throw std::runtime_error("irLen too big");
  }
  ir_    = BYPASS;
  state_ = State::TestLogicReset;
}

const std::string &
JtagTap::toString(State state)
{
	return stateNames[static_cast<unsigned>(state)];
}

size_t
JtagTap::getDRLen()
{
    size_t sz = dri_.size();
    // also correct if sz == 0 (drRemBits initialized to 8)
    return 8*sz - (8-drRemBits_);
}

JtagTap::State
JtagTap::nextState(bool tms, bool tdi, bool tdo)
{
    switch ( state_ ) {
       case State::TestLogicReset:
           ir_ = BYPASS;
           if ( ! tms ) {
              state_ = State::RunTestIdle;
           }
           break;
       case State::RunTestIdle:
           if ( tms ) {
              state_ = State::SelectDRScan;
           }
           break;
       case State::SelectDRScan:
           if ( tms ) {
              state_ = State::SelectIRScan;
           } else {
              state_ = State::CaptureDR;
           }
           break;
       case State::CaptureDR:
           dri_.clear();
           dro_.clear();
           drRemBits_ = 8;
           if ( tms ) {
              state_ = State::Exit1DR;
           } else {
              state_ = State::ShiftDR;
           }
           break;
       case State::ShiftDR:
           if ( drRemBits_ >= sizeof(uint8_t)*8 ) {
             dri_.push_back(0x00);
             dro_.push_back(0x00);
             drRemBits_ = 0;
           }
           if ( tdi ) {
               dri_[dri_.size() - 1] |= (1<<drRemBits_);
           }
           if ( tdo ) {
               dro_[dro_.size() - 1] |= (1<<drRemBits_);
           }
           ++drRemBits_;
           
           if ( tms ) {
              state_ = State::Exit1DR;
           }
           break;
       case State::Exit1DR:
           if ( tms ) {
              state_ = State::UpdateDR;
           } else {
              state_ = State::PauseDR;
           }
           break;
       case State::PauseDR:
           if ( tms ) {
              state_ = State::Exit2DR;
           }
           break;
       case State::Exit2DR:
           if ( tms ) {
              state_ = State::UpdateDR;
           } else {
              state_ = State::ShiftDR;
           }
           break;
       case State::UpdateDR:
           updateDR(dri_, dro_, drRemBits_);
           if ( tms ) {
              state_ = State::SelectDRScan;
           } else {
              state_ = State::RunTestIdle;
           }
           break;
       case State::SelectIRScan:
           if ( tms ) {
              state_ = State::TestLogicReset;
           } else {
              state_ = State::CaptureIR;
           }
           break;
       case State::CaptureIR:
	   irLen_ = 0;
	   ir_    = 0;
           if ( tms ) {
              state_ = State::Exit1IR;
           } else {
              state_ = State::ShiftIR;
           }
           break;
       case State::ShiftIR:
	   if ( irLen_ >= sizeof(ir_)*8 ) {
               throw std::runtime_error("irLen too big");
	   }
	   if ( tdi ) {
	       ir_ |= (1ULL<<irLen_);
	   }
	   ++irLen_;
           if ( tms ) {
              state_ = State::Exit1IR;
           }
           break;
       case State::Exit1IR:
           if ( tms ) {
              state_ = State::UpdateIR;
           } else {
              state_ = State::PauseIR;
           }
           break;
       case State::PauseIR:
           if ( tms ) {
              state_ = State::Exit2IR;
           }
           break;
       case State::Exit2IR:
           if ( tms ) {
              state_ = State::UpdateIR;
           } else {
              state_ = State::ShiftIR;
           }
           break;
       case State::UpdateIR:
	   updateIR(ir_, irLen_);
           if ( tms ) {
              state_ = State::SelectDRScan;
           } else {
              state_ = State::RunTestIdle;
           }
           break;
    }
    return state_;
}
