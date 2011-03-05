/*
 * $Id: FileVersion.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_FILEVERSION_H_
#define _YCL_FILEVERSION_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#pragma comment(lib, "version.lib")

namespace yebisuya {

class FileVersion {
private:
        static const char* ROOT() {
                static const char string[] = "\\";
                return string;
        }
        static const char* TRANS() {
                static const char string[] = "\\VarFileInfo\\Translation";
                return string;
        }
        static const char* STRINGBLOCK() {
                static const char string[] = "\\StringFileInfo\\%08X\\%s";
                return string;
        }

	static const char* COMMENTS() {
	        static const char string[] = "Comments";
	        return string;
	}
	static const char* COMPANYNAME() {
	        static const char string[] = "CompanyName";
	        return string;
	}
	static const char* FILEDESCRIPTION() {
	        static const char string[] = "FileDescription";
	        return string;
	}
	static const char* FILEVERSION() {
	        static const char string[] = "FileVersion";
	        return string;
	}
	static const char* INTERNALNAME() {
	        static const char string[] = "InternalName";
	        return string;
	}
	static const char* LEGALCOPYRIGHT() {
	        static const char string[] = "LegalCopyright";
	        return string;
	}
	static const char* LEGALTRADEMARKS() {
	        static const char string[] = "LegalTrademarks";
	        return string;
	}
	static const char* ORIGINALFILENAME() {
	        static const char string[] = "OriginalFilename";
	        return string;
	}
	static const char* PRIVATEBUILD() {
	        static const char string[] = "PrivateBuild";
	        return string;
	}
	static const char* PRODUCTNAME() {
	        static const char string[] = "ProductName";
	        return string;
	}
	static const char* PRODUCTVERSION() {
	        static const char string[] = "ProductVersion";
	        return string;
	}
	static const char* SPECIALBUILD() {
	        static const char string[] = "SpecialBuild";
	        return string;
	}

	char* info;
	mutable LPWORD trans;
	mutable UINT transSize;
	mutable VS_FIXEDFILEINFO* fixedInfo;

	void init(HINSTANCE instance) {
		char buffer[MAX_PATH];
		::GetModuleFileName(instance, buffer, countof(buffer));
		init(buffer);
	}
	void init(const char* filename) {
		DWORD handle;
		DWORD size = ::GetFileVersionInfoSize((char*) filename, &handle);
		if (size != 0){
			info = new char[size];
			if (!::GetFileVersionInfo((char*) filename, handle, size, info)) {
				delete[] info;
				info = NULL;
			}
		}
	}
	bool validateFixedFileInfo()const {
		if (fixedInfo == NULL){
			UINT size;
			if (!::VerQueryValue((void*) (const void*) info, (char*) ROOT(), (LPVOID*) &fixedInfo, &size))
				return false;
		}
		return true;
	}
	bool validateTranslation()const {
		if (trans == NULL){
			if (!::VerQueryValue((void*) (const void*) info, (char*) TRANS(), (LPVOID*) &trans, &transSize))
				return false;
		}
		return true;
	}
public:
	FileVersion():info(NULL), fixedInfo(NULL), trans(NULL), transSize(0) {
		init(GetInstanceHandle());
	}
	FileVersion(HINSTANCE hInstance):info(NULL), fixedInfo(NULL), trans(NULL), transSize(0) {
		init(hInstance);
	}
	FileVersion(const char* filename):info(NULL), fixedInfo(NULL), trans(NULL), transSize(0) {
		init(filename);
	}
	~FileVersion() {
		delete[] info;
	}

	VS_FIXEDFILEINFO* getFixedFileInfo()const {
		return validateFixedFileInfo() ? fixedInfo : NULL;
	}
    int getFileVersionMS()const {
		return validateFixedFileInfo() ? fixedInfo->dwFileVersionMS : 0;
	}
    int getFileVersionLS()const {
		return validateFixedFileInfo() ? fixedInfo->dwFileVersionLS : 0;
	}
    int getProductVersionMS()const {
		return validateFixedFileInfo() ? fixedInfo->dwProductVersionMS : 0;
	}
    int getProductVersionLS()const {
		return validateFixedFileInfo() ? fixedInfo->dwProductVersionLS : 0;
	}
	size_t getCharsetCount()const {
		return validateTranslation() ?	transSize / sizeof (DWORD) : 0;
	}
	int getCharset(int index = 0)const {
		return validateTranslation() ? MAKELONG(trans[index * 2 + 1], trans[index * 2]) : 0;
	}
	const char* getString(int charset, const char* blockName)const {
		char buffer[64];
		wsprintf(buffer, STRINGBLOCK(), charset, blockName);
		UINT size;
		const char* string;
		return ::VerQueryValue((void*) (const void*) info, buffer, (LPVOID*) &string, &size) ? string : NULL;
	}
	const char* getString(const char* blockName)const {
		return getString(getCharset(), blockName);
	}

	const char* getComments()const {
		return getString(COMMENTS());
	}
	const char* getCompanyName()const {
		return getString(COMPANYNAME());
	}
	const char* getFileDescription()const {
		return getString(FILEDESCRIPTION());
	}
	const char* getFileVersion()const {
		return getString(FILEVERSION());
	}
	const char* getInternalName()const {
		return getString(INTERNALNAME());
	}
	const char* getLegalCopyright()const {
		return getString(LEGALCOPYRIGHT());
	}
	const char* getLegalTrademarks()const {
		return getString(LEGALTRADEMARKS());
	}
	const char* getOriginalFilename()const {
		return getString(ORIGINALFILENAME());
	}
	const char* getPrivateBuild()const {
		return getString(PRIVATEBUILD());
	}
	const char* getProductName()const {
		return getString(PRODUCTNAME());
	}
	const char* getProductVersion()const {
		return getString(PRODUCTVERSION());
	}
	const char* getSpecialBuild()const {
		return getString(SPECIALBUILD());
	}

	static const FileVersion& getOwnVersion() {
		static FileVersion ver;
		return ver;
	}
};

}

#endif//_YCL_FILEVERSION_H_
