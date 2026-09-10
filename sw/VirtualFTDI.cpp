#include <climits>
#include <VirtualFTDIDevice.hpp>
#include <VirtualFTDIDescriptors.h>
#include <Toastbox/RuntimeError.h>
#include <string>
#include <vector>
#include <memory>
#include <FTEmul.hpp>
#include <JtagAna.hpp>
#include <getopt.h>
#include <time.h>
#include <cstring>

using std::string;
using namespace Toastbox;
using namespace ftemul;
using std::vector;

class FWAdapter : public FTInterface {
	std::shared_ptr<ftemul::FTStream> ft_;
public:
	FWAdapter(const char *dev) {
		const char *pre = "raw:";
		size_t      off = strlen(pre);
		if ( 0 == strncmp(dev, pre, off) ) {
			ft_ = std::make_shared<ftemul::RawFifo>(dev + off);
		} else {
			ft_ = std::make_shared<ftemul::FW>(dev);
		}
	}

	virtual void setPortLevels(uint8_t l) override {
		ft_->setPortLevels(l);
	}

        virtual ssize_t mpsse(const uint8_t *tbuf, size_t tsize, int bits, ShiftOp op = ShiftOp::TDI, uint8_t *rbuf = nullptr, size_t rsize = 0) override {
		return ft_->ft(tbuf, tsize, bits, static_cast<int>(op), rbuf, rsize);
	}
};

static void
usage(const char *nm) {
	printf("usage: %s [-gh] [-d ACM_device]\n", nm);
	printf(" -g         : increment debug level (can be given multiple times)\n");
	printf(" -h         : print this message\n");
	printf(" -d dev     : select ttyACM device with MPSSE emulation\n");
}

int main(int argc, char * const argv[]) {
    const char *emulDev = "/dev/ttyACM0";
    int         dbgLvl  = 0;
//    const char *serialNo1 = "TS9D9HD5B";
//    const char *serialNo2 = "FT6WW2FJA";

    int opt;

    while ( (opt = getopt(argc, argv, "d:gh")) > 0 ) {
	    switch ( opt ) {
		case 'd': emulDev = optarg; break;
		case 'g': ++dbgLvl;         break;
		case 'h': usage(argv[0]);   return 0;
		default:
			  throw RuntimeError("Unsupported option -%c", opt);
	    }
    }

    const VirtualUSBDevice::Info deviceInfo = {
        .deviceDesc             = &Descriptor::Device,
        .deviceQualifierDesc    = &Descriptor::DeviceQualifier,
        .configDescs            = Descriptor::Configurations,
        .configDescsCount       = std::size(Descriptor::Configurations),
        .stringDescs            = Descriptor::Strings,
        .stringDescsCount       = std::size(Descriptor::Strings),
        .throwOnErr             = true,
    };
    
    VirtualFTDIDevice dev(deviceInfo);
    dev.addChannel( std::make_shared<FWAdapter>( emulDev ), Endpoint::Out2, Endpoint::In1 );
    dev.addChannel( std::shared_ptr<FWAdapter>(),           Endpoint::Out4, Endpoint::In3 );
    dev.setDebug( dbgLvl );

    try {
        try {
            dev.start();
        } catch (std::exception& e) {
            throw RuntimeError(
                "Failed to start VirtualUSBDevice: %s"                                  "\n"
                "Make sure:"                                                            "\n"
                "  - you're root"                                                       "\n"
                "  - the vhci-hcd kernel module is loaded: `sudo modprobe vhci-hcd`"    "\n",
                e.what()
            );
        }
        
        printf("Started\n");
        
//        // Test device teardown (after 5 seconds)
//        std::thread([&] {
//            sleep(5);
//            printf("Stopping device...\n");
//            dev.stop();
//        }).detach();
        
	dev.run();
        
    } catch (const std::exception& e) {
        fprintf(stderr, "Error: %s\n", e.what());
        // Using _exit to avoid calling destructors for static vars, since that hangs
        // in __pthread_cond_destroy, because our thread is sitting in _State.signal.wait()
        _exit(1);
    }
    
    return 0;
}
