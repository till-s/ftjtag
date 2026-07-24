#include <JtagAna.hpp>
#include <cinttypes>
#include <cstdio>

void
JtagAna::updateDR(const std::vector<uint8_t> &dri, const std::vector<uint8_t> &dro, uint8_t lastBits)
{
    printf("DR Update; IR %" PRIx64 ", DR-len %zu\n", getIR(), getDRLen());
}

void
JtagAna::updateIR(uint64_t ir, unsigned irLen)
{
    printf("IR Update; IR %" PRIx64 ", IR-len %u\n", ir, irLen);
}

