#include <stdint.h>
#include <stdio.h>

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <ntstatus.h>
#include <bcrypt.h>

int main(int argc, char *argv[])
{
	for (int i = 0; i < 256; i++) {
		uint8_t buf[16] = {};
		NTSTATUS r = BCryptGenRandom(NULL, buf, sizeof(buf), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
		if (r != STATUS_SUCCESS) {
			printf("error %d\n", (int)r);
		}
		for (size_t j = 0; j < sizeof(buf); j++) {
			printf("%02x", buf[j]);
		}
		printf("\n");
	}
	return EXIT_SUCCESS;
}
