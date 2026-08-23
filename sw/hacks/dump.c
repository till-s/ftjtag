#include <stdio.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <getopt.h>
#include <string.h>

static void h(int unused) {}

int
main(int argc, char **argv)
{
	const char *devn = "/dev/ttyACM1";
	struct termios raw;
	int fd;
	uint8_t buf[65536];
	int got,put;
	int opt;
	struct sigaction sa;
	FILE *outf = stdout;

	while ( (opt = getopt(argc, argv, "d:")) > 0 ) {
		switch ( opt ) {
			case 'd': devn = optarg; break;
			default:
				  fprintf(stderr, "invalid option -%c\n", opt);
				  return 1;
		}
	}

	cfmakeraw( &raw );

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = 0; /* ensure SA_RESTART is not set; as it is by signal(3) */
	sa.sa_handler = h;

	if (1) {
		if ( sigaction(SIGINT, &sa, NULL) ) perror( "sigaction failed" );
		if ( sigaction(SIGTERM, &sa, NULL) ) perror( "sigaction failed" );
	}

	if (  (fd = open(devn, O_RDWR)) < 0 ) {
		perror("open");
		return 1;
	}

	if ( tcsetattr(fd, TCSANOW, &raw) ) {
		perror("tcsetattr");
		return 1;
	}

	tcflush(fd, TCIOFLUSH);

	while (1) {
		if (1) {
		struct pollfd pp;
		pp.fd = fd;
		pp.events = POLLIN | POLLERR;
		int st = poll(&pp, 1, -1);
		if ( st <= 0 || !!(pp.revents & (POLLERR | POLLHUP)) ) {
			fflush(outf);
			break;
		}
		}
		got = read(fd, buf, sizeof(buf));
		if ( got <= 0 ) {
			if ( 0 == got ) {
				break;
			}
			perror("read");
			return 1;
		}
		fwrite(buf, 1, got, outf);
	}
}
