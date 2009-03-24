/*
 * $Id: StringUtil.h,v 1.4 2007-08-18 08:52:18 maya Exp $
 */

#ifndef _YCL_STRINGUTIL_H_
#define _YCL_STRINGUTIL_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <YCL/common.h>

#include <YCL/String.h>
#include <YCL/StringBuffer.h>

namespace yebisuya {

class StringUtil {
private:
	static int hexadecimal(char c) {
		if ('0' <= c && c <= '9') {
			return c - '0';
		}else if ('A' <= c && c <= 'F') {
			return c - 'A' + 10;
		}else if ('a' <= c && c <= 'f') {
			return c - 'a' + 10;
		}else{
			return -1;
		}
	}
	static int octet(char c) {
		return '0' <= c && c <= '7' ? c - '0' : -1;
	}
	static const char* ESCAPE_CHARS() {
		static const char string[] = "abfnrtv";
		return string;
	}
	static const char* CONTROL_CHARS() {
		static const char string[] = "\a\b\f\n\r\t\v";
		return string;
	}
public:
	static String escape(String string) {
		if (string == NULL)
			return NULL;
		return escape(string, 0, string.length());
	}
	static String escape(String string, int offset, int length) {
		if (string == NULL)
			return NULL;
		StringBuffer buffer;
		return escape(buffer, string, offset, length) ? buffer.toString() : string;
	}
	static bool escape(StringBuffer& buffer, String string) {
		if (string == NULL)
			return false;
		return escape(buffer, string, 0, string.length());
	}
	static bool escape(StringBuffer& buffer, String string, int offset, int length) {
		if (string == NULL)
			return false;
		const char* start = string;
		start += offset;
		const char* p = start;
		const char* end = start + length;
		while (p < end && *p != '\0') {
			char ch = *p;
			if ('\0' < ch && ch < ' ' || ch == '\'' || ch == '"' || ch == '?' || ch == '\\') {
				bool useOctet;
				if (ch < ' ') {
					useOctet = true;
					for (int index = 0; CONTROL_CHARS()[index] != '\0'; index++) {
						if (ch == CONTROL_CHARS()[index]) {
							ch = ESCAPE_CHARS()[index];
							useOctet = false;
							break;
						}
					}
				}else{
					useOctet = false;
				}
				if (p > start) {
					buffer.append(start, p - start);
				}
				buffer.append('\\');
				if (useOctet) {
					int octet = 6;
					while (octet >= 0) {
						buffer.append((char) ('0' + ((ch >> octet) & 0x07)));
						octet -= 3;
					}
				}else{
					buffer.append(ch);
				}
				start = ++p;
			}else{
				if (String::isLeadByte(ch))
					p++;
				p++;
			}
		}
		if (start == (const char*) string) {
			return false;
		}
		buffer.append(start);
		return true;
	}

	static String unescape(String string) {
		if (string == NULL)
			return NULL;
		return unescape(string, 0, string.length());
	}
	static String unescape(String string, int offset, int length) {
		if (string == NULL)
			return NULL;
		StringBuffer buffer;
		return unescape(buffer, string) ? buffer.toString() : string;
	}
	static bool unescape(StringBuffer& buffer, String string) {
		if (string == NULL)
			return false;
		return unescape(buffer, string, 0, string.length());
	}
	static bool unescape(StringBuffer& buffer, String string, int offset, int length) {
		if (string == NULL)
			return false;
		const char* start = string;
		start += offset;
		const char* p = start;
		const char* end = start + length;
		while (p < end && *p != '\0') {
			if (*p == '\\') {
				if (p > start) {
					buffer.append(start, p - start);
					start = p;
				}
				char ch = '\0';
				switch (*++p) {
				case '\'':
				case '"':
				case '\\':
				case '?':
					ch = *p++;
					break;
				case 'x':
				case 'X':
					{
						int ch1 = hexadecimal(*++p);
						if (ch1 != -1) {
							int ch2 = hexadecimal(*++p);
							if (ch2 != -1) {
								p++;
								ch = (char) (ch1 << 4 | ch2);
							}else{
								ch = (char) ch1;
							}
						}
					}
					break;
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					{
						ch = octet(*p);
						int d = octet(*++p);
						if (d != -1) {
							ch = (ch << 3 | d);
							d = octet(*++p);
							if (d != -1) {
								ch = (ch << 3 | d);
								p++;
							}
						}
					}
					break;
				default:
					for (int index = 0; ESCAPE_CHARS()[index] != '\0'; index++) {
						if (*p == ESCAPE_CHARS()[index]) {
							ch = CONTROL_CHARS()[index];
							p++;
							break;
						}
					}
				}
				if (ch != '\0') {
					buffer.append(ch);
					start = p;
				}
			}else{
				if (String::isLeadByte(*p)) {
					p++;
				}
				p++;
			}
		}
		if (start == (const char*) string) {
			return false;
		}
		buffer.append(start);
		return true;
	}
};

}

#endif//_YCL_STRINGUTIL_H_
