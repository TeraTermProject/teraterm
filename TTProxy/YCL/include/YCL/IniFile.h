/*
 * $Id: IniFile.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_INIFILE_H_
#define _YCL_INIFILE_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/String.h>
#include <YCL/StringBuffer.h>
#include <YCL/StringUtil.h>
#include <YCL/Integer.h>
#include <YCL/Enumeration.h>
#include <YCL/Vector.h>
#include <YCL/Hashtable.h>

namespace yebisuya {

class IniFile {
private:
	static String YES() {
		static const char YES[] = "yes";
		static String string = YES;
		return string;
	}
	static String NO() {
		static const char NO[] = "no";
		static String string = NO;
		return string;
	}
	static const char* INVALID() {
		static const char string[] = "\n:";
		return string;
	}

	class EnumKeyNames : public Enumeration<String> {
	private:
		Vector<String> list;
		mutable int index;
	public:
		EnumKeyNames(const char* filename, const char* section):index(0) {
			Hashtable<String, int> table;
			int section_len = strlen(section);
			char* buffer;
			size_t size = 256;
			while ((buffer = getSectionNames(filename, size)) == NULL)
				size += 256;
			const char* name = buffer;
			while (*name != '\0') {
				if (strncmp(name, section, section_len) == 0) {
					if (name[section_len] == '\\') {
						int i;
						name += section_len + 1;
						for (i = 0; name[i] != '\0'; i++) {
							if (name[i] == '\\')
								break;
							if (String::isLeadByte(name[i]))
								i++;
						}
						table.put(String(name, i), 1);
					}
				}
				while (*name++ != '\0')
					;
			}
			delete[] buffer;
			Pointer<Enumeration<String> > keys = table.keys();
			while (keys->hasMoreElements()) {
				list.add(keys->nextElement());
			}
		}
		virtual bool hasMoreElements()const {
			return index < list.size();
		}
		virtual String nextElement()const {
			return list.get(index++);
		}
	};
	friend class EnumKeyNames;
	class EnumValueNames : public Enumeration<String> {
	private:
		char* buffer;
		mutable char* p;
	public:
		EnumValueNames(const char* filename, const char* section) {
			size_t size = 256;
			while ((buffer = getValueNames(filename, section, size)) == NULL)
				size += 256;
			p = buffer;
		}
		~EnumValueNames() {
			delete[] buffer;
		}
		virtual bool hasMoreElements()const {
			return *p != '\0';
		}
		virtual String nextElement()const {
			String next = p;
			while (*p++ != '\0')
				;
			return next;
		}
	};
	friend class EnumValueNames;
    class Value {
    	friend class IniFile;
    private:
    	const IniFile*const ini;
        String name;
        Value(const IniFile* ini, const String& name):ini(ini), name(name) {
        }
        Value(); // never implemented
        Value(const Value&); // never implemented
        void operator=(const Value&); // never implemented
    public:
        operator String()const {
            return ini->getString(name);
        }
        void operator=(const String& value) {
            ((IniFile*) ini)->setString(name, value);
        }
        operator long()const {
            return ini->getInteger(name);
        }
        void operator=(long value) {
            ((IniFile*) ini)->setInteger(name, value);
        }
    	operator bool()const {
    		return ini->getBoolean(name);
    	}
    	void operator=(bool value) {
            ((IniFile*) ini)->setBoolean(name, value);
        }
        bool remove() {
            return ((IniFile*) ini)->deleteValue(name);
        }
    };
	static char* getSectionNames(const char* filename, size_t size) {
		char* buffer = new char[size];
		if (::GetPrivateProfileSectionNames(buffer, size, filename) < size - 2)
			return buffer;
		delete[] buffer;
		return NULL;
	}
	static char* getValueNames(const char* filename, const char* section, size_t size) {
		static const char null = '\0';
		char* buffer = new char[size];
		if (::GetPrivateProfileString(section, NULL, &null, buffer, size, filename) < size - 2)
			return buffer;
		delete[] buffer;
		return NULL;
	}
	static String getString(const char* filename, const char* section, const char* name, bool& exists, size_t size) {
		char* buffer = (char*) alloca(size);
		size_t len = ::GetPrivateProfileString(section, name, INVALID(), buffer, size, filename);
		if (len < size - 2) {
			// 改行を含んだ文字列はiniファイルからは取得できないので取得失敗したことが分かる
			if (buffer[0] == '\n') {
				exists = false; 
				return NULL;
			}
			// エスケープ文字を展開
			return StringUtil::unescape(buffer);
		}
		exists = true;
		return NULL;
	}
	static String generateSectionName(const char* parentSection, const char* subkeyName) {
		StringBuffer buffer = parentSection;
		buffer.append('\\');
		buffer.append(subkeyName);
		return buffer.toString();
	}
	static bool deleteAllSubsection(const char* filename, const char* section, const char* subkey) {
		String keyname = generateSectionName(section, subkey);
		int keyname_len = keyname.length();

		char* buffer;
		size_t size = 256;
		while ((buffer = getSectionNames(filename, size)) == NULL)
			size += 256;
		const char* name = buffer;
		bool succeeded = true;
		while (*name != '\0') {
			if (strncmp(name, keyname, keyname_len) == 0) {
				if (name[keyname_len] == '\0' || name[keyname_len] == '\\') {
					if (!::WritePrivateProfileString(name, NULL, NULL, filename)) {
						succeeded = false;
						break;
					}
				}
			}
			while (*name++ != '\0')
				;
		}
		delete[] buffer;
		return succeeded;
	}

	String filename;
	String section;
public:
	IniFile() {
	}
	IniFile(String filename, String section):filename(filename), section(section) {
	}
	IniFile(const IniFile& parent, const char* subkeyName):filename(parent.filename), section(generateSectionName(parent.section, subkeyName)) {
	}

	bool isOpened()const {
		return filename != NULL && section != NULL;
	}
	long getInteger(const char* name, long defaultValue = 0)const {
		return filename != NULL && section != NULL ? ::GetPrivateProfileInt(section, name, defaultValue, filename) : defaultValue;
	}
	String getString(const char* name)const {
		return getString(name, NULL);
	}
	String getString(const char* name, String defaultValue)const {
		if (filename != NULL && section != NULL && name != NULL) {
			size_t size = 256;
			String string;
			bool exists;
			while ((string = getString(filename, section, name, exists, size)) == NULL && exists)
				size += 256;
			if (string != NULL)
				return string;
		}
		return defaultValue;
	}
	bool getBoolean(const char* name, bool defaultValue = false)const {
		String string = getString(name);
		if (string != NULL) {
			if (string == YES())
				return true;
			if (string == NO())
				return false;
		}
		return defaultValue;
	}

	bool setInteger(const char* name, long value) {
		return setString(name, Integer::toString(value));
	}
	bool setString(const char* name, String value) {
		if (filename != NULL && section != NULL && name != NULL) {
			// NULLでなければエスケープしてから""で括る
			if (value != NULL) {
				StringBuffer buffer;
				buffer.append('"');
				if (!StringUtil::escape(buffer, value))
					buffer.append(value);
				buffer.append('"');
				value = buffer.toString();
			}
			return ::WritePrivateProfileString(section, name, value, filename) != FALSE;
		}
		return false;
	}
	bool setBoolean(const char* name, bool value) {
		return setString(name, value ? YES() : NO());
	}

	bool existsValue(const char* name) {
		char buffer[3];
		::GetPrivateProfileString(section, name, INVALID(), buffer, countof(buffer), filename);
		return buffer[0] != '\n';
	}
	bool deleteKey(const char* name) {
		return filename != NULL && section != NULL && name != NULL && deleteAllSubsection(filename, section, name);
	}
	bool deleteValue(const char* name) {
		return filename != NULL && section != NULL && name != NULL
			&& ::WritePrivateProfileString(section, name, NULL, filename) != FALSE;
	}

	bool open(String filename, String section) {
		close();
		this->filename = filename;
		this->section = section;
		return isOpened();
	}
	bool open(const IniFile& parent, const char* subkeyName) {
		close();
		filename = parent.filename;
		section = generateSectionName(parent.section, subkeyName);
		return isOpened();
	}
	void close() {
		filename = NULL;
		section = NULL;
	}

	Pointer<Enumeration<String> > keyNames()const {
		return filename != NULL && section != NULL ? new EnumKeyNames(filename, section) : NULL;
	}
	Pointer<Enumeration<String> > valueNames()const {
		return filename != NULL && section != NULL ? new EnumValueNames(filename, section) : NULL;
	}

	Value operator[](String name) {
		return Value(this, name);
	}
	const Value operator[](String name)const {
		return Value(this, name);
	}
};

}

#endif//_YCL_INIFILE_H_
