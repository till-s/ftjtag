#include <ftd2xx.h>
#include <stdio.h>
#include <vector>

int
main(int argc, char **argv)
{
		DWORD numDevices;
		FT_STATUS st;

		st = FT_CreateDeviceInfoList( &numDevices );
		if ( FT_OK != st ) {
			fprintf(stderr, "Unable to create device info list %d\n", st);
			return 1;
		}
		printf("%d devices found\n", numDevices);
		std::vector<FT_DEVICE_LIST_INFO_NODE>  n;
		n.resize(numDevices);

		st = FT_GetDeviceInfoList(&n[0], &numDevices);
		if ( FT_OK != st ) {
			fprintf(stderr, "Unable to obtain device info list %d\n", st);
			return 1;
		}
		printf("numDevices now %d\n", numDevices);
		for ( auto it = n.begin(); it != n.end(); ++it ) {
			printf("flags 0x%08x\n", (*it).Flags);
			printf("type  0x%08x\n", (*it).Type );
			printf("ID    0x%08x\n", (*it).ID );
			printf("LocID 0x%08x\n", (*it).LocId );
			printf("Serno %s\n",     (*it).SerialNumber);
			printf("Descr %s\n",     (*it).Description);
		}


		return 0;
}
