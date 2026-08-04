# Radiant Programmer Hacking

To hook into `FT_Read` & friends (from libftd2xx) we used some hacks
which are documented here (radiant 2025.2).

However, some observations first:

 1) `pgrmain` is not directly linked (directly or indirectly) against
    libftd2xx.so. Rather, `libjtagri.so` (which itself is also run-time
    loaded) links `libftd2xx.so` at run-time. (All in the programmer's
    `bin/lin64` directory.)
 2) `libftd2xx.so` is statically linked against `libusb-1.0`.


## Hooking libusb

libusb can be hooked via std. `LD_PRELOAD`ing.

1) build (matching) libusb from source with desired modifications
2) `LD_PRELOAD` libusb before starting the programmer; this causes
   the run-time linker to resolve against the perloaded version
   instead of the one included in libftd2xx.

## Hooking libftd2xx

Since this is a closed-source library we cannot employ the same trick.
However, we can modify some symbols in libftd2xx using patchelf.
If we e.g., replace the symbol `FT_Read` with `wrap_FT_Read` then
references from *outside* libftd2xx to `FT_Read` will no longer
be resolved from the patched library and we are free to provide
our own version.

Note: this does *not* work for *internal* references; i.e., calls to
`FT_Read` from within libftd2xx will resolve to `wrap_FT_Read` after
patching. `patchelf` doesn't let us modify names of 'defined' or
'undefined' symbols only.

     cp libftd2xx.so.x.y.z libftd2xx.patched.so     
     patchelf --rename-dynamic-symbols mymap --set-soname libftd2xx.patched.so libftd2xx.patched.so

We can now create a mock library with our wrappers

     extern uint32_t wrap_FT_Read(void *h, uint8_t *buf, uint32_t size, uint32_t *pxferred);
     uint32_t FT_Read(void *h, uint8_t *buf, uint32_t size, uint32_t *pxferred) {
        // magick here
        uint32_t st = wrap_FT_Read(h, buf, size, pxferred);
        // and there
        return st;
     }

compile:

     gcc -shared -o libmock.so -Wl,-soname,libftd2xx.so -fPIC mock.c -L<path> -lftd2xx.patched -Wl,-rpath,<path>

We can now either relink libftd2xx.so to point to libmock.so or `LD_PREOAD` it, too.

## JTAGH19SOFT

JRSTN goes hi on negative edge of TCK after leaving `TEST_LOGIC_RESET`;
      goes low in falling edge after entering this state (or initially)
JTCK -> mirror of TCK (seems not gated)
JTDI -> mirror of TDI 

going into `TEST_LOGIC_RESET` does *not* change the current instruction!
Initial instruction after FPGA config is 0x00; TLR does not change bypass
register either.

setting instruction to 0x32 (ER1)
-> JSHIFT asserts when in SHIFT state
-> JUPDATE asserts when in UPDATE state
-> JCE2 not asserted
shifting 0 into DR -> DR out is 0x00000006; 3 lsbits seems stick (0x6)

shift IR: old instruction is shifted out JSHIFT, JUPDATE not asserted

setting instruction to 0x38 (ER2)

-> JSHIFT asserts when in SHIFT state
-> JCE2 asserted in CAPTURE and SHIFT but not UPDATE; deasserted if pause is entered between CAPTURE and SHIFT

`ip_enable` changes on negative edge after entering UPDATE state

ER2 data is `HUB_ID` (0x43) when ER1 data register is 0x800006; nothing is shifted in, zeroes
are appended while shifting more bits. JSHIFT/JUPDATE/JCE2 are still asserted but `ip_enable` is not.
Repeating pattern of 8-bytes: 0x0000 0000 0000 0043.
If ER data register is 0x000006 (nothing selected) zeroes are shifted out.

`ER_TDO` changes state on negative TCK; seems to be registered on rising edge and propagated to TDO on falling edge
