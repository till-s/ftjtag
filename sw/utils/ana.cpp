#include <cstdio>
#include <cstring>
#include <getopt.h>
#include <JtagAna.hpp>
#include <FTEmul.hpp>

#include <stdexcept>

int
main(int argc, char **argv)
{
int         opt;
const char *dnam = "/dev/ttyACM0";
const char *fnam = nullptr;
FILE       *f    = nullptr;
const char *gnam = nullptr;
FILE       *g    = nullptr;
int         ch;
bool        bb   = false;
int         verb = 0;
int        *i_p;
	while ( (opt = getopt(argc, argv, "f:g:d:Bv:")) > 0 ) {
		i_p = nullptr;
		switch ( opt ) {
			case 'f': fnam = optarg; break;
			case 'g': gnam = optarg; break;
			case 'd': dnam = optarg; break;
			case 'B': bb   = true;   break;
			case 'v': i_p  = &verb;  break;
			default:
				throw std::runtime_error("unknown option");
		}
		if ( i_p && 1 != sscanf(optarg, "%i", i_p) ) {
			fprintf(stderr, "unable to scan arg of option -%c\n", opt);
			return 1;
		}
}
	if ( ! fnam ) {
		fprintf(stderr, "Missing -f <file> arg");
		return 1;
	}

	if ( ! (f = fopen(fnam, "r")) ) {
		perror("error opening -f <file>");
		return 1;
	}

	if ( gnam && !(g = fopen(gnam, "r")) ) {
		perror("error opening -g <file>");
	}

	if ( f && g ) {
		JtagAna fana;
		JtagAna gana;
		fana.setVerb(verb);
		gana.setVerb(verb);
		int     chf, chg;
		int     fsync,gsync;
		fsync = gsync = 0;
		// sync both input streams to 
		while ( (chf = getc(f)) >= 0 && JtagTap::State::RunTestIdle != fana.nextState(chf) ) {
			++fsync;
			// do nothing
		}
		while ( (chg = getc(g)) >= 0 && JtagTap::State::RunTestIdle != gana.nextState(chg) ) {
			++gsync;
			// do nothing
		}
		while ( (chf = getc(f)) >= 0 && (chg = getc(g)) >= 0 ) {
			JtagTap::State fstate, gstate;
			fstate = fana.getState();
			if ( (chf & 3) != (chg & 3) ) {
				printf("Input Mismatch, f: 0x%02x, g: 0x%02x, fstate %s, gstate %s\n", chf, chg, fana.toString(fstate).c_str(), gana.toString(gana.getState()).c_str());
			}
			if ( JtagTap::State::ShiftDR == fstate || JtagTap::State::ShiftIR == fstate ) {
				if ( (chf & 4) != (chg & 4) ) {
					printf("TDO Mismatch (in %s), f: 0x%02x, g: 0x%02x\n", fana.toString(fstate).c_str(), chf & 4, chg & 4);
				}
			}
			if ( (fstate = fana.nextState(chf)) != (gstate = gana.nextState(chg)) ) {
				printf("State Mismatch, f: %s, g: %s\n", fana.toString(fstate).c_str(), gana.toString(gstate).c_str());
			}
		}
	} else {
		JtagAna ana;
		ana.setVerb(verb);
		if ( bb ) {
			ftemul::FW fw(dnam);
			unsigned bitcnt = 0;
			while ( (ch = getc(f)) >= 0 ) {
				uint8_t line = 0;
				line |= (!!(ch&0)) << 3; // TMS
				line |= (!!(ch&1)) << 1; // TDI
				fw.setPortLevels(line);
				line |= (1<<0); // TCK
				fw.setPortLevels(line);
				if ( (++bitcnt & 0xffff) == 0 ) {
					printf("bitcnt %u\n", bitcnt);
				}
			}
			fw.setPortLevels(0x00);
		} else {
			while ( (ch = getc(f)) >= 0 ) {
				ana.nextState(ch);
			}	
		}
	}
	return 0;
}
