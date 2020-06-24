
#include <stdio.h>
#include <locale.h>

#include "libsusieplugin.h"

static BOOL SaveBitmapFileW(const wchar_t *nameFile, const unsigned char *pbuf, const BITMAPINFO *pbmi)
{
	int bmiSize;
	DWORD writtenByte;
	HANDLE hFile;
	BITMAPFILEHEADER bfh;

	hFile = CreateFileW(nameFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;

	bmiSize = pbmi->bmiHeader.biSize;

	switch (pbmi->bmiHeader.biBitCount) {
		case 1:
			bmiSize += pbmi->bmiHeader.biClrUsed ? sizeof(RGBQUAD) * 2 : 0;
			break;

		case 2:
			bmiSize += sizeof(RGBQUAD) * 4;
			break;

		case 4:
			bmiSize += sizeof(RGBQUAD) * 16;
			break;

		case 8:
			bmiSize += sizeof(RGBQUAD) * 256;
			break;
	}

	ZeroMemory(&bfh, sizeof(bfh));
	bfh.bfType = MAKEWORD('B', 'M');
	bfh.bfOffBits = sizeof(bfh) + bmiSize;
	bfh.bfSize = bfh.bfOffBits + pbmi->bmiHeader.biSizeImage;

	WriteFile(hFile, &bfh, sizeof(bfh), &writtenByte, 0);
	WriteFile(hFile, pbmi, bmiSize, &writtenByte, 0);
	WriteFile(hFile, pbuf, pbmi->bmiHeader.biSizeImage, &writtenByte, 0);

	CloseHandle(hFile);

	return TRUE;
}

int test_main(const wchar_t *fname_in, const wchar_t *spi_dir, const wchar_t *fname_out)
{
	wprintf(L"in '%s'\n", fname_in);
	wprintf(L"dir '%s'\n", spi_dir);
	wprintf(L"out '%s'\n", fname_out);

	HANDLE hbmi;
	HANDLE hbuf;
	BOOL r = SusieLoadPicture(fname_in, spi_dir, &hbmi, &hbuf);
	if (!r) {
		printf("can not read\n");
		return 0;
	}
	SaveBitmapFileW(fname_out, (unsigned char *)hbuf, (BITMAPINFO *)hbmi);
	LocalFree(hbmi);
	LocalFree(hbuf);
	return 0;
}

#if 0
int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	if (argc != 4) {
		printf(
			"spi_test [readfile] [spi_dir] [writefile]\n"
			"example:\n"
			" spi_test lena.jpg ./debug lena.bmp\n");
		return 0;
	}

	wchar_t fname_in[MAX_PATH];
	wchar_t spi_dir[MAX_PATH];
	wchar_t fname_out[MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, argv[1], -1, fname_in, _countof(fname_in));
	MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, argv[2], -1, spi_dir, _countof(spi_dir));
	MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, argv[3], -1, fname_out, _countof(fname_out));
	int r = test_main(fname_in, spi_dir, fname_out);
	return r;
}
#else
int wmain(int argc, wchar_t *argv[])
{
	setlocale(LC_ALL, "");
	if (argc != 4) {
		printf(
			"spi_test [readfile] [spi_dir] [writefile]\n"
			"example:\n"
			" spi_test lena.jpg ./debug lena.bmp\n");
		return 0;
	}

	const wchar_t *fname_in = argv[1];
	const wchar_t *spi_dir = argv[2];
	const wchar_t *fname_out = argv[3];
	int r = test_main(fname_in, spi_dir, fname_out);
	return r;
}
#endif
