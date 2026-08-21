#include <stdio.h>
#include <getopt.h>
#include <stdexcept>
#include <vector>
#include <FTEmul.hpp>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cinttypes>
#include <memory>
#include <system_error>

class MMap {
	void  *p_;
	size_t l_;
public:
	MMap(const char *fnam, size_t creatsz)
	{
		int fd;
		int flags = (creatsz ? O_RDWR | O_CREAT : O_RDONLY);
		if ( (fd = open(fnam, flags)) < 0 ) {
			throw std::system_error(errno, std::generic_category(), "unable to open file");
		}
		flags = PROT_READ;
		if ( ! creatsz ) {
			struct stat sb;
			if ( fstat(fd, &sb) ) {
				close(fd);
				throw std::system_error(errno, std::generic_category(), "unable to fstat file");
			}
			l_ = sb.st_size;
		} else {
			if ( ftruncate(fd, creatsz) < 0 ) {
				close(fd);
				throw std::system_error(errno, std::generic_category(), "unable to resize file");
			}
			l_     = creatsz;
			flags |= PROT_WRITE;
		}
		p_ = mmap(nullptr, l_, flags, MAP_SHARED, fd, 0);
		close(fd);
		if ( MAP_FAILED == p_ ) {
			throw std::system_error(errno, std::generic_category(), "unable to mmap file");
		}
	}

	const void *cp() { return const_cast<const void *>(p_); }
	void  *p()    { return p_; }
	size_t size() { return l_; }

	~MMap()
	{
		munmap(p_, l_);
	}
};

int
main(int argc, char **argv)
{
	const char *fnam = nullptr;
	const char *gnam = nullptr;
	const char *ttynam = "/dev/ttyACM0";
	int opt;

	while ( (opt = getopt(argc, argv, "f:d:g:")) > 0 ) {
		switch ( opt ) {
			case 'f':
				fnam = optarg;
				break;
			case 'g':
				gnam = optarg;
				break;
			default:
				throw std::runtime_error("Unsupported option");
		}
	}

	ftemul::FW fw(ttynam);

	if ( ! fnam ) {
		std::vector<uint8_t> tst({
				0x01,
				0x01,
				0x01,
				0x01,
				0x01,
				0x01,
				0x00, // run-test-idle
				0x01, // scan-DR
				0x00, // capture-DR
				0x00, // shift-DR
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x00,
				0x01, // exit-DR
				0x01, // update-DR
				0x00, // run-test IDLE
		});
		fw.bb( &tst[0], &tst[0], tst.size() );
		uint32_t id  = 0;
		uint32_t msk = 1;
		unsigned idx = 10;
		while ( msk ) {
			if ( !!(tst[idx] & 4) ) {
				id |= msk;
			}
			msk <<= 1;
			idx++;
		}
		printf("ID 0x%08" PRIx32 "\n", id);
	} else {
		MMap src(fnam, 0);
		std::unique_ptr<MMap> dstp;
		if ( gnam ) {
			dstp = std::unique_ptr<MMap>( new MMap(gnam, src.size() ) );
		}
		fw.bb( static_cast<const uint8_t *>(src.cp()), dstp ? static_cast<uint8_t*>(dstp->p()) : nullptr, src.size() );
	}
}
