#ifndef _YEBISOCKS_LOGGER_H_
#define _YEBISOCKS_LOGGER_H_

#include <YCL/String.h>
using namespace yebisuya;

class Logger {
private:
	HANDLE logfile;
	Logger():logfile(INVALID_HANDLE_VALUE) {
	}
public:
	~Logger() {
		if (logfile != INVALID_HANDLE_VALUE) {
			::CloseHandle(logfile);
			logfile = INVALID_HANDLE_VALUE;
		}
	}
private:
	static Logger& instance() {
		static Logger instance;
		return instance;
	}
	void debuglog_string(const char* label, const char* data) {
		if (logfile != INVALID_HANDLE_VALUE) {
			DWORD written;
			const char* start = data;
			WriteFile(logfile, label, strlen(label), &written, NULL);
			WriteFile(logfile, ": \"", 3, &written, NULL);
			while (*data != '\0') {
				char escaped[2] = {'\\'};
				switch (*data) {
				case '\n':
					escaped[1] = 'n';
					break;
				case '\r':
					escaped[1] = 'r';
					break;
				case '\t':
					escaped[1] = 't';
					break;
				case '\\':
					escaped[1] = '\\';
					break;
				case '\"':
					escaped[1] = '\"';
					break;
				}
				if (escaped[1] != '\0') {
					if (start < data) {
						WriteFile(logfile, start, data - start, &written, NULL);
					}
					WriteFile(logfile, escaped, 2, &written, NULL);
					start = data + 1;
				}
				data++;
			}
			if (start < data) {
				WriteFile(logfile, start, data - start, &written, NULL);
			}
			WriteFile(logfile, "\"\r\n", 3, &written, NULL);
		}
	}

	void debuglog_binary(const char* label, const unsigned char* data, int size) {
		if (logfile != INVALID_HANDLE_VALUE) {
			DWORD written;
			char buf[16];
			int len;
			WriteFile(logfile, label, strlen(label), &written, NULL);
			WriteFile(logfile, ": [", 3, &written, NULL);
			while (size-- > 0) {
				len = wsprintf(buf, " %02x", *data++);
				WriteFile(logfile, buf, 3, &written, NULL);
			}
			WriteFile(logfile, " ]\r\n", 4, &written, NULL);
		}
	}

public:
	static void open(String filename) {
		close();
		if (filename != NULL) {
			HANDLE logfile = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (logfile != INVALID_HANDLE_VALUE) {
				::SetFilePointer(logfile, 0, NULL, FILE_END);
				instance().logfile = logfile;
			}
		}
	}
	static void close() {
		if (instance().logfile != INVALID_HANDLE_VALUE) {
			::CloseHandle(instance().logfile);
			instance().logfile = INVALID_HANDLE_VALUE;
		}
	}
	static void log(const char* label, const char* data) {
		instance().debuglog_string(label, data);
	}
	static void log(const char* label, const unsigned char* data, int size) {
		instance().debuglog_binary(label, data, size);
	}
};

#endif//_YEBISOCKS_LOGGER_H_
