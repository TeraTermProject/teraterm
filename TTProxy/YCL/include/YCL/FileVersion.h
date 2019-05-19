/*
 * $Id: FileVersion.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_FILEVERSION_H_
#define _YCL_FILEVERSION_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#if defined(_MSC_VER)
#pragma comment(lib, "version.lib")
#endif

namespace yebisuya {

class FileVersion {
private:
	static const TCHAR* ROOT() {
		static const TCHAR string[] = _T("\\");
		return string;
	}
	static const TCHAR* TRANS() {
		static const TCHAR string[] = _T("\\VarFileInfo\\Translation");
		return string;
	}
	static const TCHAR* STRINGBLOCK() {
		static const TCHAR string[] = _T("\\StringFileInfo\\%08X\\%s");
		return string;
	}

	static const TCHAR* COMMENTS() {
		static const TCHAR string[] = _T("Comments");
		return string;
	}
	static const TCHAR* COMPANYNAME() {
		static const TCHAR string[] = _T("CompanyName");
		return string;
	}
	static const TCHAR* FILEDESCRIPTION() {
		static const TCHAR string[] = _T("FileDescription");
		return string;
	}
	static const TCHAR* FILEVERSION() {
		static const TCHAR string[] = _T("FileVersion");
		return string;
	}
	static const TCHAR* INTERNALNAME() {
		static const TCHAR string[] = _T("InternalName");
		return string;
	}
	static const TCHAR* LEGALCOPYRIGHT() {
		static const TCHAR string[] = _T("LegalCopyright");
		return string;
	}
	static const TCHAR* LEGALTRADEMARKS() {
		static const TCHAR string[] = _T("LegalTrademarks");
		return string;
	}
	static const TCHAR* ORIGINALFILENAME() {
		static const TCHAR string[] = _T("OriginalFilename");
		return string;
	}
	static const TCHAR* PRIVATEBUILD() {
		static const TCHAR string[] = _T("PrivateBuild");
		return string;
	}
	static const TCHAR* PRODUCTNAME() {
		static const TCHAR string[] = _T("ProductName");
		return string;
	}
	static const TCHAR* PRODUCTVERSION() {
		static const TCHAR string[] = _T("ProductVersion");
		return string;
	}
	static const TCHAR* SPECIALBUILD() {
		static const TCHAR string[] = _T("SpecialBuild");
		return string;
	}

	TCHAR* info;
	mutable LPWORD trans;
	mutable UINT transSize;
	mutable VS_FIXEDFILEINFO* fixedInfo;

	void init(HINSTANCE instance) {
		TCHAR buffer[MAX_PATH];
		::GetModuleFileName(instance, buffer, countof(buffer));
		init(buffer);
	}
	void init(const TCHAR* filename) {
		DWORD handle;
		DWORD size = ::GetFileVersionInfoSize(filename, &handle);
		if (size != 0){
			info = new TCHAR[size];
			if (!::GetFileVersionInfo(filename, handle, size, info)) {
				delete[] info;
				info = NULL;
			}
		}
	}
	bool validateFixedFileInfo()const {
		if (fixedInfo == NULL){
			UINT size;
			if (!::VerQueryValue((void*) (const void*) info, (LPTSTR)ROOT(), (LPVOID*) &fixedInfo, &size))
				return false;
		}
		return true;
	}
	bool validateTranslation()const {
		if (trans == NULL){
			if (!::VerQueryValue((void*) (const void*) info, (LPTSTR)TRANS(), (LPVOID*) &trans, &transSize))
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
	FileVersion(const TCHAR* filename):info(NULL), fixedInfo(NULL), trans(NULL), transSize(0) {
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
	const TCHAR* getString(int charset, const TCHAR* blockName)const {
		TCHAR buffer[64];
		wsprintf(buffer, STRINGBLOCK(), charset, blockName);
		UINT size;
		const TCHAR* string;
		return ::VerQueryValue((void*) (const void*) info, buffer, (LPVOID*) &string, &size) ? string : NULL;
	}
	const TCHAR* getString(const TCHAR* blockName)const {
		return getString(getCharset(), blockName);
	}

	const TCHAR* getComments()const {
		return getString(COMMENTS());
	}
	const TCHAR* getCompanyName()const {
		return getString(COMPANYNAME());
	}
	const TCHAR* getFileDescription()const {
		return getString(FILEDESCRIPTION());
	}
	const TCHAR* getFileVersion()const {
		return getString(FILEVERSION());
	}
	const TCHAR* getInternalName()const {
		return getString(INTERNALNAME());
	}
	const TCHAR* getLegalCopyright()const {
		return getString(LEGALCOPYRIGHT());
	}
	const TCHAR* getLegalTrademarks()const {
		return getString(LEGALTRADEMARKS());
	}
	const TCHAR* getOriginalFilename()const {
		return getString(ORIGINALFILENAME());
	}
	const TCHAR* getPrivateBuild()const {
		return getString(PRIVATEBUILD());
	}
	const TCHAR* getProductName()const {
		return getString(PRODUCTNAME());
	}
	const TCHAR* getProductVersion()const {
		return getString(PRODUCTVERSION());
	}
	const TCHAR* getSpecialBuild()const {
		return getString(SPECIALBUILD());
	}

	static const FileVersion& getOwnVersion() {
		static FileVersion ver;
		return ver;
	}
};

}

#endif//_YCL_FILEVERSION_H_
