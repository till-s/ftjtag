#include <FTEmul.hpp>

#include <stdio.h>
#include <getopt.h>
#include <stdexcept>
#include <system_error>

int
main(int argc, char **argv)
{
const char *dev = "/dev/ttyACM0";
bool        version = false;
int         opt;

	while ( (opt = getopt(argc, argv, "d:v")) > 0 ) {
		switch (opt) {
			case 'd': dev     = optarg; break;
			case 'v': version = true; break;
			default:
				  throw std::runtime_error("unknown option");
		}
	}
	ftemul::FW fw(dev);

	{
	unsigned cnt = fw.countChainLength();
	printf("%u device%s in chain\n", cnt, cnt > 1 ? "s" : "");

	std::vector<uint32_t> ids;
		fw.getIDs(ids, cnt);
		for ( auto it = ids.begin(); it != ids.end(); ++it ) {
			printf("IDCODE: 0x%08" PRIx32 "\n", *it);
		}
	}

	if ( version ) {
		fw.printVersion();
		return 0;
	}

}
