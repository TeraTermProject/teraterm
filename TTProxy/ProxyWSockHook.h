#ifndef _YEBISOCKS_PROXYWSOCKHOOK_H_
#define _YEBISOCKS_PROXYWSOCKHOOK_H_

#include <YCL/StringBuffer.h>
#include <YCL/Dialog.h>
#include <YCL/ComboBoxCtrl.h>
#include <YCL/EditBoxCtrl.h>
#include <YCL/IniFile.h>
#include <YCL/Resource.h>
#include <YCL/FileVersion.h>
using namespace yebisuya;

#include "resource.h"
#include "Hooker.h"
#include "Logger.h"
#include "SSLSocket.h"

#include "ttlib.h"
#include "i18n.h"

extern char UILanguageFile[MAX_PATH];

void UTIL_get_lang_msg(const char *key, PCHAR buf, int buf_len, const char *def)
{
    GetI18nStr("TTProxy", key, buf, buf_len, def, UILanguageFile);
}

class ProxyWSockHook {
public:
    class MessageShower {
    public:
        virtual void showMessage(const char* message)const = 0;
    };
private:
    struct DUMMYHOSTENT {
        struct hostent entry;
        struct addrinfo *ai;
        struct addrinfo ainfo[2];
        in_addr addr;
        struct in6_addr addr6;
        struct sockaddr_in saddr;
        struct sockaddr_in6 saddr6;
        char* addrlist[2];
        char* alias;
        char hostname[1];
    };

    class AsyncSelectInfoTable {
    private:
        struct AsyncSelectInfo {
            HWND window;
            UINT message;
            long event;
            AsyncSelectInfo():window(NULL), message(0), event(0) {
            }
            AsyncSelectInfo(const AsyncSelectInfo& src):window(src.window), message(src.message), event(src.event) {
            }
            AsyncSelectInfo(HWND window, UINT message, long event):window(window), message(message), event(event) {
            }
            bool operator==(const AsyncSelectInfo& src) {
                return window == src.window && message == src.message && event == src.event;
            }
            void operator=(const AsyncSelectInfo& src) {
                window = src.window;
                message = src.message;
                event = src.event;
            }
            void operator=(int v) {
                if (v == 0) {
                    window = NULL;
                    message = 0;
                    event = 0;
                }
            }
            operator HWND() const {
                return window == NULL && message == 0 && event == 0 ? 0 : window;
            }
        };
        Hashtable<SOCKET, AsyncSelectInfo> table;
        mutable CRITICAL_SECTION section;
    public:
        AsyncSelectInfoTable() {
            ::InitializeCriticalSection(&section);
        }
        ~AsyncSelectInfoTable() {
            ::DeleteCriticalSection(&section);
        }
        void put(SOCKET s, HWND window, UINT message, long event) {
            ::EnterCriticalSection(&section);
            if (message != 0 || event != 0) {
                table.put(s, AsyncSelectInfo(window, message, event));
            }else{
                table.remove(s);
            }
            ::LeaveCriticalSection(&section);
        }
        void get(SOCKET s, HWND& window, UINT& message, long& event)const {
            ::EnterCriticalSection(&section);
            AsyncSelectInfo info = table.get(s);
            ::LeaveCriticalSection(&section);
            window = info.window;
            message = info.message;
            event = info.event;
        }
    };
    class ProxyInfo {
    public:
        enum Type {
            TYPE_NONE,
            TYPE_HTTP,
            TYPE_TELNET,
            TYPE_SOCKS4,
            TYPE_SOCKS5,
            TYPE_SSL,
            TYPE_HTTP_SSL,
            TYPE_TELNET_SSL,
            TYPE_SOCKS4_SSL,
            TYPE_SOCKS5_SSL,
            TYPE_NONE_FORCE,
        };
    private:
        struct PROXY_TYPE_TABLE {
            Type type;
            const char* name;
        };
        static const PROXY_TYPE_TABLE* proxy_type_table() {
            static PROXY_TYPE_TABLE table[] = {
                {TYPE_HTTP,   "http"},
                {TYPE_SOCKS5, "socks"},
                {TYPE_SOCKS4, "socks4"},
                {TYPE_TELNET, "telnet"},
                {TYPE_SOCKS5, "socks5"},
                {TYPE_NONE_FORCE, "none"},
                {TYPE_HTTP_SSL,   "http+ssl"},
                {TYPE_SOCKS5_SSL, "socks+ssl"},
                {TYPE_SOCKS4_SSL, "socks4+ssl"},
                {TYPE_TELNET_SSL, "telnet+ssl"},
                {TYPE_SOCKS5_SSL, "socks5+ssl"},
                {TYPE_SSL, "ssl"},
                {TYPE_SSL, "none+ssl"},
                {TYPE_NONE, NULL},
            };
            return table;
        }
    public:
        static Type parseType(const char* string) {
            return parseType(string, string + strlen(string));
        }
        static Type parseType(const char* string, const char* end) {
            int length = end - string;
            char* buffer = (char*) alloca(length + 1);
            char* dst = buffer;
            while (length-- > 0 && *string != '\0') {
                int ch = *string++;
                if ('A' <= ch && ch <= 'Z') {
                    ch += 'a' - 'A';
                }
                *dst++ = ch;
            }
            *dst = '\0';
            const PROXY_TYPE_TABLE* table = proxy_type_table();
            bool ssl_enabled = SSLSocket::isEnabled();
            while (table->name != NULL) {
                if (!ssl_enabled && strstr(table->name, "ssl") != NULL) {
                    table++;
                    continue;
                }
                if (strcmp(buffer, table->name) == 0)
                    return table->type;
                table++;
            }
            return TYPE_NONE;
        }
        static const char* getTypeName(Type type) {
            if (type != TYPE_NONE && type != TYPE_NONE_FORCE) {
                const PROXY_TYPE_TABLE* table = proxy_type_table();
                while (table->name != NULL) {
                    if (type == table->type)
                        return table->name;
                    table++;
                }
            }
            return NULL;
        }

        static int parsePort(const char* string) {
            return parsePort(string, string + strlen(string));
        }
        static int parsePort(const char* string, const char* end) {
            if (string > end || *string < '1' || '9' < *string)
                return -1;
            int digit = 0;
            while (string < end) {
                if (*string < '0' || '9' < *string)
                    return -1;
                digit = digit * 10 + (*string - '0');
                if (digit > 65535)
                    return -1;
                string++;
            }
            if (digit == 0)
                return -1;
            return digit;
        }

#if 0
        static String parse(const char* url, ProxyInfo& proxy) {
            char* p = strstr((char*) url, "://");
            if (p == NULL) {
                proxy.type = TYPE_NONE;
                return NULL;
            }
            proxy.type = parseType(url, p);
            if (proxy.type == TYPE_NONE)
                return NULL;
            p += 3;
            const char* start = p;
            while (*p != '\0' && *p != '/') {
                if (*p == ':') {
                    if (proxy.host != NULL || start >= p) {
                        proxy.type = TYPE_NONE;
                        return NULL;
                    }
                    proxy.host = urldecode(start, p);
                    start = p + 1;
                }else if (*p == '@') {
                    if (proxy.user != NULL) {
                        proxy.type = TYPE_NONE;
                        return NULL;
                    }
                    if (proxy.host == NULL) {
                        proxy.user = urldecode(start, p);
                    }else{
                        proxy.user = proxy.host;
                        proxy.pass = urldecode(start, p);
                        proxy.host = NULL;
                    }
                    start = p + 1;
                }
                p++;
            }
            if (proxy.type != TYPE_NONE_FORCE && proxy.type != TYPE_SSL) {
                if (proxy.host == NULL) {
                    if (start >= p) {
                        proxy.type = TYPE_NONE;
                        return NULL;
                    }
                    proxy.host = urldecode(start, p);
                }else{
                    proxy.port = parsePort(start, p);
                    if (proxy.port < 0) {
                        proxy.type = TYPE_NONE;
                        return NULL;
                    }
                }
            }
            if (*p == '/') {
                p++;
                return p;
            }
            return url;
        }
#else
        static String parse(const char* url, ProxyInfo& proxy) {
            char* p = strstr((char*) url, "://");
            if (p == NULL) {
                proxy.type = TYPE_NONE;
                return NULL;
            }
            proxy.type = parseType(url, p);
            if (proxy.type == TYPE_NONE)
                return NULL;
            p += 3;
            const char* start = p;

            // user:pass があれば格納する
            String tmp = String(start);
            int index = tmp.indexOf('@');
            if (index > -1) {
                tmp = String(p, index);
                index = tmp.indexOf(':');
                if (index == -1) {
                    proxy.user = urldecode(p, p + tmp.length());
                }
                else {
                    proxy.user = urldecode(p, p + index);
                    proxy.pass = urldecode(p + index + 1, p + tmp.length());
                }
                p += tmp.length() + 1;
            }

            // ホスト名を格納する
            start = p;
            int in_blacket = false;
            while (*p != '\0' && *p != '/') {
                if (*p == '[') {
                    in_blacket = true;
                }else if (in_blacket && *p == ']') {
                    in_blacket = false;
                }else if (!in_blacket && *p == ':') {
                    if (*start == '[') {
                        proxy.host = String(start + 1, p - start - 2);
                    }
                    else {
                        proxy.host = String(start, p - start);
                    }
                    start = p + 1;
                }
                p++;
            }

            if (proxy.type != TYPE_NONE_FORCE && proxy.type != TYPE_SSL) {
                if (proxy.host == NULL) {
                    if (start >= p) {
                        proxy.type = TYPE_NONE;
                        return NULL;
                    }
                    proxy.host = String(start, p - start);
                }else{
                    // ポート番号を格納する
                    proxy.port = parsePort(start, p);
                    if (proxy.port < 0) {
                        proxy.type = TYPE_NONE;
                        return NULL;
                    }
                }
            }
            if (*p == '/') {
                p++;
                return p;
            }
            return url;
        }
#endif
        String generateURL()const {
            if (type == TYPE_NONE || host == NULL)
                return NULL;
            StringBuffer buffer;
            buffer.append(getTypeName(type));
            buffer.append("://");
            if (type != TYPE_SSL) {
                if (user != NULL) {
                    urlencode(user, buffer);
                    if (pass != NULL) {
                        buffer.append(':');
                        urlencode(pass, buffer);
                    }
                    buffer.append('@');
                }
                if (host.indexOf(':') == -1) {
                    buffer.append(host);
                }
                else {
                    buffer.append("[");
                    buffer.append(host);
                    buffer.append("]");
                }
                if (port != 0) {
                    buffer.append(':');
                    int digit = 10000;
                    while (digit > 0 && (unsigned) port < (unsigned) digit) {
                        digit /= 10;
                    }
                    while (digit > 0) {
                        int d = (unsigned) port / digit % 10;
                        digit /= 10;
                        buffer.append('0' + d);
                    }
                }
            }
            return buffer.toString();
        }
    private:
        static int hexdigit(int ch) {
            if ('0' <= ch && ch <= '9') {
                return ch - '0';
            }else if ('A' <= ch && ch <= 'F') {
                return ch - 'A' + 10;
            }else if ('a' <= ch && ch <= 'f') {
                return ch - 'a' + 10;
            }else{
                return -1;
            }
        }

        static String urldecode(const char* start, const char* end) {
            char* buffer = (char*) alloca((end - start) * 3 + 1);
            char* dst = buffer;
            int c1, c2;
            while (start < end) {
                if (*start == '%' && start + 2 < end && (c1 = hexdigit(start[1])) != -1 && (c2 = hexdigit(start[2])) != -1) {
                    *dst++ = (char) (c1 << 4 | c2);
                    start += 3;
                }else{
                    if (IsDBCSLeadByte(*start)) {
                        *dst++ = *start++;
                    }
                    *dst++ = *start++;
                }
            }
            *dst = '\0';
            return buffer;
        }
        static String urlencode(const char* string, StringBuffer& buffer) {
            static const char table[] = "0123456789ABCDEF";
            const char* start = string;
            while (*string != '\0') {
                if ('0' <= *string && *string <= '9'
                    || 'A' <= *string && *string <= 'Z'
                    || 'a' <= *string && *string <= 'z'
                    || *string == '-' || *string == '.' || *string == '_') {
                }else if (::IsDBCSLeadByte(*string)) {
                    string++;
                }else{
                    if (start < string) {
                        buffer.append(start, string - start);
                    }
                    buffer.append('%');
                    buffer.append(table[(unsigned) *string >> 4]);
                    buffer.append(table[(unsigned) *string & 0x0f]);
                    start = string + 1;
                }
                string++;
            }
            if (start < string) {
                buffer.append(start);
            }
            return buffer;
        }
    public:
        ProxyInfo():type(TYPE_NONE), port(0) {
        }
        ~ProxyInfo() {
        }
        unsigned short getPort() {
            if (port == 0) {
                switch (type) {
                case TYPE_SOCKS4:
                case TYPE_SOCKS5:
                case TYPE_SOCKS4_SSL:
                case TYPE_SOCKS5_SSL:
                    return 1080;
                case TYPE_TELNET:
                case TYPE_TELNET_SSL:
                    return 23;
                case TYPE_HTTP:
                case TYPE_HTTP_SSL:
                    return 80;
                }
            }
            return port;
        }
        Type type;
        String host;
        unsigned short port;
        String user;
        String pass;
    };

    struct ConnectionInfo {
        ProxyInfo proxy;
        String realhost;
        unsigned short  realport;
        in_addr addr;
        struct in6_addr addr6;
        char* buffer;
        DWORD time;
        ConnectionInfo(ProxyInfo& proxy, String realhost):proxy(proxy), realhost(realhost), buffer(NULL) {
        }
        ~ConnectionInfo() {
            delete[] buffer;
        }
        void fillBuffer(char* buffer, int bufferLength) {
          fillBuffer(buffer, bufferLength, "ssh");
        }
        void fillBuffer(char* buffer, int bufferLength, const char *portname) {
          fillBuffer(buffer, bufferLength, "ssh", AF_UNSPEC);
        }
        void fillBuffer(char* buffer, int bufferLength, const char *portname, int addr_family) {
            struct servent* sv;
            struct addrinfo* res;
            struct addrinfo* res0;
            struct addrinfo hints;
            int portnum = 0;
            if (sv = getservbyname(portname, "tcp")) {
              portnum = sv->s_port;
            }
            else {
              portnum = atoi(portname);
            }
            memset(buffer, 0, bufferLength);
            DUMMYHOSTENT* dst = (DUMMYHOSTENT*) buffer;
            dst->addr = addr;
            dst->addr6 = addr6;
            dst->addrlist[0] = (char*) &dst->addr;
            dst->addrlist[1] = NULL;
            dst->entry.h_addr_list = dst->addrlist;
            dst->alias = NULL;
            dst->entry.h_aliases = &dst->alias;
            dst->entry.h_addrtype = AF_INET;
            dst->entry.h_length = sizeof (in_addr);
            dst->entry.h_name = dst->hostname;

            dst->saddr6.sin6_family = AF_INET6;
            dst->saddr6.sin6_port = htons(portnum);
            dst->saddr6.sin6_flowinfo = 0;
            dst->saddr6.sin6_addr = dst->addr6;
            dst->saddr6.sin6_scope_id = 0;

            dst->saddr.sin_family = AF_INET;
            dst->saddr.sin_port = htons(portnum);
            dst->saddr.sin_addr = dst->addr;

            dst->ainfo[0].ai_flags = 0;
            dst->ainfo[0].ai_family = AF_INET6;
            dst->ainfo[0].ai_socktype = SOCK_STREAM;
            dst->ainfo[0].ai_protocol = IPPROTO_TCP;
            dst->ainfo[0].ai_addrlen = sizeof(sockaddr_in6);
            dst->ainfo[0].ai_canonname = NULL;
            dst->ainfo[0].ai_addr = (struct sockaddr *)(&dst->saddr6);
            dst->ainfo[0].ai_next = NULL;

            dst->ainfo[1].ai_flags = 0;
            dst->ainfo[1].ai_family = AF_INET;
            dst->ainfo[1].ai_socktype = SOCK_STREAM;
            dst->ainfo[1].ai_protocol = IPPROTO_TCP;
            dst->ainfo[1].ai_addrlen = sizeof(sockaddr_in);
            dst->ainfo[1].ai_canonname = NULL;
            dst->ainfo[1].ai_addr = (struct sockaddr *)(&dst->saddr);
            dst->ainfo[1].ai_next = NULL;

            switch (addr_family) {
                case AF_INET:
                    dst->ai = &dst->ainfo[1];
                    break;
                case AF_INET6:
                    dst->ai = &dst->ainfo[0];
                    break;
                case AF_UNSPEC:
                default:
                    memset(&hints, 0, sizeof hints);
                    getaddrinfo(proxy.host, NULL, &hints, &res0);
                    if (res0) {
                        int flag = 0;
                        for (res = res0; res; res = res->ai_next) {
                            switch (res->ai_family) {
                            case AF_INET6:
                                flag |= 1;
                                break;
                            case AF_INET:
                                flag |= 2;
                                break;
                            }
                            if (flag == 3)
                                break;
                        }
                        switch (flag) {
                        case 1:
                            dst->ai = &dst->ainfo[0];
                            break;
                        case 2:
                            dst->ai = &dst->ainfo[1];
                            break;
                        case 3:
                        default:
                            dst->ainfo[0].ai_next = &dst->ainfo[1];
                            dst->ai = &dst->ainfo[0];
                            break;
                        }
                        freeaddrinfo(res0);
                    }
                    else {
                        dst->ainfo[0].ai_next = &dst->ainfo[1];
                        dst->ai = &dst->ainfo[0];
                    }
                    break;
            }

            strcpy_s(dst->hostname, bufferLength - sizeof (DUMMYHOSTENT), realhost);
        }
    };
    class ConnectionInfoHolder {
    private:
        ConnectionInfo* info;
    public:
        ConnectionInfoHolder():info(NULL) {
        }
        ~ConnectionInfoHolder() {
            delete info;
        }
        operator ConnectionInfo*()const {
            return info;
        }
        void operator=(ConnectionInfo* newInfo) {
            if (info != newInfo) {
                delete info;
                info = newInfo;
            }
        }
        // メンバ選択演算子
        ConnectionInfo* operator->()const {
            return info;
        }
    };

    class ConnectionInfoList {
    private:
        CRITICAL_SECTION section;
        ConnectionInfoHolder list[254];
        Hashtable<String,ConnectionInfo*> table;
    public:
        ConnectionInfoList() {
            ::InitializeCriticalSection(&section);
        }
        ~ConnectionInfoList() {
            ::DeleteCriticalSection(&section);
        }
        HANDLE getTask(ConnectionInfo* info) {
            if (info == NULL)
                return NULL;
			// s_b4のマイナスの値をハンドルとして使用する
            return (HANDLE)(intptr_t)-info->addr.S_un.S_un_b.s_b4;
        }
        ConnectionInfo* get(HANDLE task) {
			// ハンドルは数値としてマイナスの値のはず
            if ((intptr_t) task >= 0)
                return NULL;
            return get((int) -((intptr_t) task) - 1);
        }
        ConnectionInfo* get(in_addr addr) {
            if (addr.S_un.S_un_b.s_b1 != 0 || addr.S_un.S_un_b.s_b2 != 0 || addr.S_un.S_un_b.s_b3 != 0)
                return NULL;
            return get(addr.S_un.S_un_b.s_b4 - 1);
        }
        ConnectionInfo* get(in6_addr addr) {
            int i;
            for (i=0; i<10; i++) {
              if (addr.s6_addr[i] != 0)
                return NULL;
            }
            if (addr.s6_addr[10] != 0xff || addr.s6_addr[11] != 0xff)
                return NULL;
            for (i=12; i<15; i++) {
              if (addr.s6_addr[i] != 0)
                return NULL;
            }
            return get(addr.s6_addr[15] - 1);
        }
        ConnectionInfo* get(int index) {
            ::EnterCriticalSection(&section);
            ConnectionInfo* info = 0 <= index && index < countof(list) ? (ConnectionInfo*) list[index] : NULL;
            ::LeaveCriticalSection(&section);
            return info;
        }
        ConnectionInfo* find(const char* url) {
            ::EnterCriticalSection(&section);
            ConnectionInfo* info = table.get(url);
            ::LeaveCriticalSection(&section);
            if (info != NULL) {
                info->time = ::GetTickCount();
                return info;
            }
            ProxyInfo proxy;
            String realhost = ProxyInfo::parse(url, proxy);
            if (realhost == NULL || realhost.length() == 0) {
                proxy = instance().defaultProxy;
                if (proxy.type != ProxyInfo::TYPE_NONE) {
                    realhost = url;
                }
            }
            if (proxy.type == ProxyInfo::TYPE_NONE)
                return NULL;
            info = new ConnectionInfo(proxy, realhost);

            int i;
            ::EnterCriticalSection(&section);
            for (i = 0; i < countof(list); i++) {
                if (list[i] == NULL) {
                    list[i] = info;
                    break;
                }
            }
            if (i >= countof(list)) {
                DWORD now = ::GetTickCount();
                DWORD max = 0;
                int index = -1;
                for (i = 0; i < countof(list); i++) {
                    if (list[i] != NULL) {
                        if (now - list[i]->time > max) {
                            max = now - list[i]->time;
                            index = i;
                        }
                    }
                }
                list[index] = info;
            }
            table.put(url, info);
            ::LeaveCriticalSection(&section);
            info->addr.s_addr = htonl(i + 1);
            memset(info->addr6.s6_addr, 0, 16);
            info->addr6.s6_addr[10] = 0xff;
            info->addr6.s6_addr[11] = 0xff;
            info->addr6.s6_addr[15] = i + 1;

            info->time = ::GetTickCount();
            return info;
        }
    };
    friend class ConnectionInfoList;

    class OptionsSettingDialog : public Dialog {
    private:
        ComboBoxCtrl resolveCombo;
        Window host;
        Window user;
        Window pass;
        Window conn;
        Window erro;
        Window log;
    protected:
        virtual bool dispatch(UINT message, WPARAM wParam, LPARAM lParam) {
            if (message == WM_COMMAND && wParam == MAKEWPARAM(IDC_REFER, BN_CLICKED)) {
                char buffer[1024];
                char uimsg[MAX_UIMSG];
                OPENFILENAME ofn = {
                    sizeof ofn,
                    *this,
                };
                ofn.lpstrFile = buffer;
                ofn.nMaxFile = countof(buffer);
                ofn.Flags = OFN_LONGNAMES | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST | OFN_NOREADONLYRETURN | OFN_HIDEREADONLY;
                UTIL_get_lang_msg("MSG_LOGFILE_SELECT", uimsg, sizeof(uimsg),
                                  "Select Logfile");
                ofn.lpstrTitle = uimsg;
                if (logfile != NULL) {
                    strcpy_s(buffer, sizeof buffer, logfile);
                }else{
                    buffer[0] = '\0';
                }
                if (::GetSaveFileName(&ofn)) {
                    if (buffer[0] != '\0') {
                        logfile = buffer;
                    }else{
                        logfile = NULL;
                    }
                    log.SetWindowText(buffer);
                }
                return true;
            }
            return Dialog::dispatch(message, wParam, lParam);
        }
        virtual bool onInitDialog() {
            const static DlgTextInfo text_info[] = {
                { 0, "DLG_OTHER_TITLE" },
                { IDC_GRP_COMMON, "DLG_OTHER_COMMON" },
                { IDC_TIMEOUT_LABEL, "DLG_OTHER_TIMEOUT" },
                { IDC_TIMEOUT_SECONDS, "DLG_OTHER_SECOND" },
                { IDC_LOGFILE_LABEL, "DLG_OTHER_LOG" },
                { IDC_REFER, "DLG_OTHER_REFER" },
                { IDC_GRP_SOCKS, "DLG_OTHER_SOCKS" },
                { IDC_RESOLVE_LABEL, "DLG_OTHER_RESOLV" },
                { IDC_GRP_TELNET, "DLG_OTHER_TELNET" },
                { IDC_HOSTNAME_LABEL, "DLG_OTHER_HOST" },
                { IDC_USERNAME_LABEL, "DLG_OTHER_USER" },
                { IDC_PASSWORD_LABEL, "DLG_OTHER_PASS" },
                { IDC_CONNECTED_LABEL, "DLG_OTHER_CONNECT" },
                { IDC_ERROR_LABEL, "DLG_OTHER_ERROR" },
//              { IDOK, "BTN_OK" },
//              { IDCANCEL, "BTN_CANCEL" },
            };
            char uimsg[MAX_UIMSG];

            Dialog::onInitDialog();

            HWND hWnd = (HWND)this;
			SetI18DlgStrs("TTProxy", hWnd, text_info, _countof(text_info), UILanguageFile);

            host = GetDlgItem(IDC_HOSTNAME);
            user = GetDlgItem(IDC_USERNAME);
            pass = GetDlgItem(IDC_PASSWORD);
            conn = GetDlgItem(IDC_CONNECTED);
            erro = GetDlgItem(IDC_ERROR);
            log = GetDlgItem(IDC_LOGFILE);

            SetDlgItemInt(IDC_TIMEOUT, timeout, false);

            resolveCombo <<= GetDlgItem(IDC_RESOLVE);
            UTIL_get_lang_msg("DLG_OTHER_RESOLV_AUTO", uimsg, sizeof(uimsg), "auto resolve");
            resolveCombo.addString(uimsg);
            UTIL_get_lang_msg("DLG_OTHER_RESOLV_REMOTE", uimsg, sizeof(uimsg), "resolve remote");
            resolveCombo.addString(uimsg);
            UTIL_get_lang_msg("DLG_OTHER_RESOLV_LOCAL", uimsg, sizeof(uimsg), "resolve local");
            resolveCombo.addString(uimsg);
            resolveCombo.setCurSel(resolve);

            host.SetWindowText(HostnamePrompt);
            user.SetWindowText(UsernamePrompt);
            pass.SetWindowText(PasswordPrompt);
            conn.SetWindowText(ConnectedMessage);
            erro.SetWindowText(ErrorMessage);

            if (logfile != NULL)
                log.SetWindowText(logfile);

            CenterWindow((HWND)*this, GetParent());

            return true;
        }
        virtual void onOK() {
            if    (host.GetWindowTextLength() == 0
                || user.GetWindowTextLength() == 0
                || pass.GetWindowTextLength() == 0
                || conn.GetWindowTextLength() == 0
                || erro.GetWindowTextLength() == 0) {
                char uimsg[MAX_UIMSG];
                UTIL_get_lang_msg("MSG_EMPTY_PARAMETER", uimsg, sizeof(uimsg),
                                  "Some parameters are empty!");
                MessageBox(uimsg, FileVersion::getOwnVersion().getProductName(), MB_OK | MB_ICONERROR);
                return;
            }

            timeout = GetDlgItemInt(IDC_TIMEOUT, 0, false);

            resolve = resolveCombo.getCurSel();

            HostnamePrompt   = host.GetWindowText();
            UsernamePrompt   = user.GetWindowText();
            PasswordPrompt   = pass.GetWindowText();
            ConnectedMessage = conn.GetWindowText();
            ErrorMessage     = erro.GetWindowText();

            logfile = log.GetWindowTextLength() > 0 ? log.GetWindowText() : NULL;

            Dialog::onOK();
        }
        virtual void onCancel() {
            Dialog::onCancel();
        }
    public:
        String logfile;
        int timeout;
        int resolve;

        String HostnamePrompt;
        String UsernamePrompt;
        String PasswordPrompt;
        String ConnectedMessage;
        String ErrorMessage;

        int open(HWND owner) {
            return Dialog::open(instance().resource_module, IDD_OPTION_SETTING, owner);
        }
    };
    friend class OptionsSettingDialog;

    class SettingDialog : public Dialog {
    private:
        EditBoxCtrl  url;
        ComboBoxCtrl type;
        EditBoxCtrl  host;
        EditBoxCtrl  port;
        EditBoxCtrl  user;
        EditBoxCtrl  pass;
        bool lock;
    protected:
        virtual bool dispatch(UINT message, WPARAM wParam, LPARAM lParam) {
            if (message == WM_COMMAND) {
                switch (wParam) {
                case MAKEWPARAM(IDC_OPTIONS, BN_CLICKED):
                    onOptions();
                    return true;
                case MAKEWPARAM(IDC_TYPE, CBN_SELCHANGE):
                case MAKEWPARAM(IDC_HOSTNAME, EN_CHANGE):
                case MAKEWPARAM(IDC_PORT, EN_CHANGE):
                case MAKEWPARAM(IDC_USERNAME, EN_CHANGE):
                case MAKEWPARAM(IDC_PASSWORD, EN_CHANGE):
                    if (!lock)
                        onChanged(LOWORD(wParam));
                    return true;
                }
            }
            return Dialog::dispatch(message, wParam, lParam);
        }
        virtual bool onInitDialog() {
            char uimsg[MAX_UIMSG];

            Dialog::onInitDialog();

            const static DlgTextInfo text_info[] = {
                { 0, "DLG_SETUP_TITLE" },
                { IDC_URL_LABEL, "DLG_SETUP_URL" },
                { IDC_TYPE_LEBEL, "DLG_SETUP_TYPE" },
                { IDC_HOSTNAME_LABEL, "DLG_SETUP_HOST" },
                { IDC_PORT_LABEL, "DLG_SETUP_PORT" },
                { IDC_USERNAME_LABEL, "DLG_SETUP_USER" },
                { IDC_PASSWORD_LABEL, "DLG_SETUP_PASS" },
                { IDC_OPTIONS, "DLG_SETUP_OTHER" },
//              { IDOK, "BTN_OK" },
//              { IDCANCEL, "BTN_CANCEL" },
            };
			HWND hWnd = HWND(this);		// うまく動かない?
			SetI18DlgStrs("TTProxy", hWnd, text_info, _countof(text_info), UILanguageFile);

            url  <<= GetDlgItem(IDC_URL);
            type <<= GetDlgItem(IDC_TYPE);
            host <<= GetDlgItem(IDC_HOSTNAME);
            port <<= GetDlgItem(IDC_PORT);
            user <<= GetDlgItem(IDC_USERNAME);
            pass <<= GetDlgItem(IDC_PASSWORD);

            lock = true;
            UTIL_get_lang_msg("MSG_TYPE_NONE", uimsg, sizeof(uimsg), "Without Proxy");
            type.addString(uimsg);
            type.addString("HTTP");
            type.addString("TELNET");
            type.addString("SOCKS4");
            type.addString("SOCKS5");
            if (SSLSocket::isEnabled()) {
                type.addString("SSL");
                type.addString("HTTP+SSL");
                type.addString("TELNET+SSL");
                type.addString("SOCKS4+SSL");
                type.addString("SOCKS5+SSL");
            }
            type.setCurSel(proxy.type);

            if (proxy.type != ProxyInfo::TYPE_NONE && proxy.type != ProxyInfo::TYPE_SSL) {
                if (proxy.host != NULL) {
                    host.SetWindowText(proxy.host);
                    if (proxy.port != 0) {
                        char buffer[16];
                        _itoa_s(proxy.port, buffer, sizeof buffer, 10);
                        port.SetWindowText(buffer);
                    }
                    if (proxy.user != NULL) {
                        user.SetWindowText(proxy.user);
                        if (proxy.pass != NULL) {
                            pass.SetWindowText(proxy.pass);
                        }
                    }
                }
            }
            lock = false;
            onChanged(0);
            CenterWindow((HWND)*this, GetParent());
            return true;
        }
        virtual void onOK() {
            char uimsg[MAX_UIMSG];
            if (proxy.type != ProxyInfo::TYPE_NONE && proxy.type != ProxyInfo::TYPE_SSL) {
                if (proxy.host == NULL) {
                    UTIL_get_lang_msg("MSG_EMPTY_HOSTNAME", uimsg, sizeof(uimsg),
                                      "Hostname is empty!");
                    MessageBox(uimsg, FileVersion::getOwnVersion().getProductName(), MB_OK | MB_ICONERROR);
                    return;
                }
                if (port.GetWindowTextLength() != 0 && proxy.port <= 0) {
                    UTIL_get_lang_msg("MSG_ILLEGAL_PORT", uimsg, sizeof(uimsg),
                                      "Illegal port number!");
                    MessageBox(uimsg, FileVersion::getOwnVersion().getProductName(), MB_OK | MB_ICONERROR);
                    return;
                }
            }
            Dialog::onOK();
        }
        virtual void onCancel() {
            Dialog::onCancel();
        }
        void onOptions() {
            OptionsSettingDialog dlg;
            dlg.timeout = instance().timeout;
            switch (instance().resolve) {
            case RESOLVE_REMOTE:
                dlg.resolve = 1;
                break;
            case RESOLVE_LOCAL:
                dlg.resolve = 2;
                break;
            default:
                dlg.resolve = 0;
                break;
            }
            dlg.HostnamePrompt = instance().prompt_table[0];
            dlg.UsernamePrompt = instance().prompt_table[1];
            dlg.PasswordPrompt = instance().prompt_table[2];
            dlg.ConnectedMessage = instance().prompt_table[3];
            dlg.ErrorMessage = instance().prompt_table[4];
            dlg.logfile = instance().logfile;
            if (dlg.open(*this) == IDOK) {
                instance().timeout = dlg.timeout;
                switch (dlg.resolve) {
                case 1:
                    instance().resolve = RESOLVE_REMOTE;
                    break;
                case 2:
                    instance().resolve = RESOLVE_LOCAL;
                    break;
                default:
                    instance().resolve = RESOLVE_AUTO;
                    break;
                }
                instance().prompt_table[0] = dlg.HostnamePrompt;
                instance().prompt_table[1] = dlg.UsernamePrompt;
                instance().prompt_table[2] = dlg.PasswordPrompt;
                instance().prompt_table[3] = dlg.ConnectedMessage;
                instance().prompt_table[4] = dlg.ErrorMessage;

                if (instance().logfile != dlg.logfile) {
                    instance().logfile = dlg.logfile;
                    Logger::open(dlg.logfile);
                }
            }
        }
        void onChanged(int id) {
            if (id == 0 || id == IDC_TYPE || id == IDC_HOSTNAME || id == IDC_USERNAME) {
                int enabled = 0;
                int typeN = type.getCurSel();
                if (typeN != ProxyInfo::TYPE_NONE && typeN != ProxyInfo::TYPE_SSL) {
                    enabled |= 1;
                    if (::GetWindowTextLength(GetDlgItem(IDC_HOSTNAME)) > 0) {
                        enabled |= 2;
                        if (::GetWindowTextLength(GetDlgItem(IDC_USERNAME)) > 0) {
                            enabled |= 4;
                        }
                    }
                }
                ::EnableWindow(GetDlgItem(IDC_HOSTNAME), (enabled & 1) != 0);
                ::EnableWindow(GetDlgItem(IDC_PORT), (enabled & 2) != 0);
                ::EnableWindow(GetDlgItem(IDC_USERNAME), (enabled & 2) != 0);
                ::EnableWindow(GetDlgItem(IDC_PASSWORD), (enabled & 4) != 0);
            }

            if (id != 0) {
                proxy.type = (ProxyInfo::Type) type.getCurSel();
                if (proxy.type != ProxyInfo::TYPE_NONE && proxy.type != ProxyInfo::TYPE_SSL) {
                    proxy.host = host.GetWindowText();
                    if (host.GetWindowTextLength() == 0) {
                        proxy.host = NULL;
                    }else{
                        proxy.host = host.GetWindowText();
                    }
                    proxy.port = GetDlgItemInt(IDC_PORT, NULL, FALSE);
                    proxy.user = user.GetWindowTextLength() > 0 ? user.GetWindowText() : NULL;
                    proxy.pass = pass.GetWindowTextLength() > 0 ? pass.GetWindowText() : NULL;
                }else{
                    proxy.host = NULL;
                    proxy.port = 0;
                    proxy.user = NULL;
                    proxy.pass = NULL;
                }
            }
            String urlS = proxy.generateURL();
            if (urlS == NULL) {
                urlS = "none:///";
            }
            url.SetWindowText(urlS);
        }
    public:
        ProxyInfo proxy;
        SettingDialog():lock(false) {
        }

        int open(HWND owner) {
            return Dialog::open(instance().resource_module, IDD_SETTING, owner);
        }
    };
    friend class SettingDialog;

    class AboutDialog : public Dialog {
    private:
        virtual bool onInitDialog() {
            String buf;
            char *buf2;
            const char *ver;
            int n, a, b, c, d, len;
            char uimsg[MAX_UIMSG], uimsg2[MAX_UIMSG], uimsg3[MAX_UIMSG];

            GetWindowText(uimsg2, sizeof(uimsg2));
            UTIL_get_lang_msg("DLG_ABOUT_TITLE", uimsg, sizeof(uimsg), uimsg2);
            SetWindowText(uimsg);

            UTIL_get_lang_msg("DLG_ABOUT_EXTENSION", uimsg, sizeof(uimsg),
                              "Tera Term proxy extension");
            UTIL_get_lang_msg("DLG_ABOUT_YEBISUYA", uimsg2, sizeof(uimsg2),
                              "YebisuyaHompo");
            UTIL_get_lang_msg("DLG_ABOUT_HOMEPAGE", uimsg3, sizeof(uimsg3),
                              "TTProxy home page");
            buf = GetDlgItemText(IDC_VERSION);
            len = buf.length() + 50;
            buf2 = (char *)_alloca(len);
            if (buf2 == NULL) {
                return true;
            }
            ver = FileVersion::getOwnVersion().getFileVersion();
            n = sscanf_s(ver, "%d, %d, %d, %d", &a, &b, &c, &d);
            if (n == 4) {
                sprintf_s(buf2, len, buf, uimsg, a, b, c, d, uimsg2, uimsg3);
            }
            SetDlgItemText(IDC_VERSION, (n == 4) ? buf2 : buf);

            GetDlgItemText(IDOK, uimsg, sizeof(uimsg));
            UTIL_get_lang_msg("BTN_OK", uimsg, sizeof(uimsg),"OK");
            SetDlgItemText(IDOK, uimsg);

            CenterWindow((HWND)*this, GetParent());

            return true;
        }
        virtual void onOK() {
            Dialog::onOK();
        }
    public :
        int open(HWND owner) {
            return Dialog::open(instance().resource_module, IDD_ABOUTDIALOG, owner);
        }
    };
    friend class AboutDialog;

    int _sendToSocket(SOCKET s, const unsigned char* buffer, int size) {
        int count = 0;
        while (count < size) {
            struct timeval tv = {timeout, 0};
            fd_set fd;
            FD_ZERO(&fd);
            FD_SET(s, &fd);
            if (select((int) (s + 1), NULL, &fd, NULL, timeout > 0 ? &tv : NULL) == SOCKET_ERROR)
                return SOCKET_ERROR;
            if (!FD_ISSET(s, &fd)) {
                return SOCKET_ERROR;
            }
            int written = ORIG_send(s, (const char*) buffer + count, size - count, 0);
            if (written == SOCKET_ERROR)
                return SOCKET_ERROR;
            count += written;
        }
        return count;
    }

    int sendToSocket(SOCKET s, const unsigned char* buffer, int size) {
        int i = 0;
        Logger::log("send", buffer, size);
        return _sendToSocket(s, buffer, size);
    }

    int sendToSocketFormat(SOCKET s, const char* format, ...) {
        char buf[1025];
        int len;
        va_list arglist;
        va_start(arglist, format);
        len = wvsprintf(buf, format, arglist);
        va_end(arglist);
        Logger::log("send", buf);
        return _sendToSocket(s, (const unsigned char*) buf, len);
    }

    /*
	 * SOCKSサーバからデータを受信する
	 *
	 * return:
	 *    1以上   受信データ数(byte)。sizeより小さい場合もある。
	 *    -1      SOCKET_ERROR 
	 */
	int recieveFromSocketTimeout(SOCKET s, unsigned char* buffer, int size, int timeout) {
        int ready = 0;
		int ret;

        while (!ready) {
            struct timeval tv = {timeout, 0};
            fd_set fd;
			int n;

            FD_ZERO(&fd);
            FD_SET(s, &fd);
			n = select((int)(s + 1), &fd, NULL, NULL, timeout > 0 ? &tv : NULL);
			switch (n) {
            case SOCKET_ERROR:
                return SOCKET_ERROR;
            case 0:
				// 受信タイムアウトの場合にもエラー扱いとする。
                return SOCKET_ERROR;
            default:
                ready = FD_ISSET(s, &fd);
                break;
            }
        }

		// SOCKSサーバから切断された場合、recv()が0を返すため、エラー扱いとする。
        ret = ORIG_recv(s, (char*) buffer, size, 0);
		if (ret == 0)
			ret = SOCKET_ERROR;
		return (ret);
    }

    int recieveFromSocket(SOCKET s, unsigned char* buffer, int size) {
        int result = recieveFromSocketTimeout(s, buffer, size, timeout);
        if (result != SOCKET_ERROR) {
            Logger::log("recv", buffer, result);
        }
        return result;
    }

    int line_input(SOCKET s, char *buf, int size) {
        char* dst = buf;
        if (size == 0)
            return 0; /* no error */
        size--;
        while (size > 0) {
            switch (recieveFromSocketTimeout(s, (unsigned char*) dst, 1, timeout)) {
            case SOCKET_ERROR:
                return SOCKET_ERROR;
            case 0:
                size = 0;
                break;
            default:
                if (*dst == '\n') {
                    size = 0;
                } else {
                    size--;
                }
                dst++;
                break;
            }
        }
        *dst = '\0';
        Logger::log("recv", buf);
        return 0;
    }

    int wait_for_prompt(SOCKET s, String prompts[], int count, int timeout /* sec */) {
        char buf[1024];
        while (1) {
            char *dst = buf;
            char *end = buf + sizeof buf - 1;
            while (dst < end) {
                switch (recieveFromSocketTimeout(s, (unsigned char*) dst, 1, timeout)) {    /* recv one-by-one */
                case SOCKET_ERROR:
                    if (WSAGetLastError() != WSAEWOULDBLOCK) {
                        return SOCKET_ERROR;
                    }
                    // no break
                case 0:
                    end = dst;                              /* end of stream */
                    break;
                default:
                    /* continue reading until last 1 char is EOL? */
                    if (*dst++ == '\n') {
                        /* finished */
                        end = dst;
                    }
                }
            }
            if (dst > buf) {
                int i;
                *dst = '\0';
                Logger::log("recv", buf);
                for (i = 0; i < count; i++) {
                    char* found = strstr(buf, prompts[i]);
                    if (found != NULL) {
                        return i;
                    }
                }
            }
        }
    }

    int begin_relay_http(ProxyInfo& proxy, String realhost, unsigned short realport, SOCKET s) {
        static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        char buf[1024];
        int status_code;
        if (sendToSocketFormat(s, strchr((const char *)realhost,':')?"CONNECT [%s]:%d HTTP/1.1\r\n":"CONNECT %s:%d HTTP/1.1\r\n", (const char *)realhost, realport) == SOCKET_ERROR)
            return SOCKET_ERROR;
        if (sendToSocketFormat(s, strchr((const char *)realhost,':')?"Host: [%s]:%d\r\n":"Host: %s:%d\r\n", (const char *)realhost, realport) == SOCKET_ERROR)
            return SOCKET_ERROR;
        if (proxy.user != NULL) {
            int userlen = strlen(proxy.user);
            int passlen = strlen(proxy.pass);
            int authlen = userlen + 1 + passlen;
            int encodedlen = (authlen + 2) / 3 * 4;
            char* auth = (char*) alloca(authlen + 1);
            char* encoded = (char*) alloca(encodedlen + 1);
            unsigned char *src = (unsigned char *) auth;
            char *dst = encoded;
            int bits = 0;
            int data = 0;
            strcpy_s(auth, authlen + 1, proxy.user);
            auth[userlen] = ':';
            strcpy_s(auth + userlen + 1, passlen + 1, proxy.pass);
        
            /* make base64 string */
            while (*src != '\0') {
                data = (data << 8) | *src++;
                bits += 8;
                while (bits >= 6){
                    bits -= 6;
                    *dst++ = base64_table[0x3F & (data >> bits)];
                    encodedlen--;
                }
            }
            if (bits > 0) {
                *dst++ = base64_table[0x3F & (data << (6 - bits))];
                encodedlen--;
            }
            while (encodedlen-- > 0) { 
                *dst++ = '=';
            }
            *dst = '\0';

            if (sendToSocketFormat(s, "Proxy-Authorization: Basic %s\r\n", encoded) == SOCKET_ERROR)
                return SOCKET_ERROR;
        }
        if (sendToSocketFormat(s, "\r\n") == SOCKET_ERROR)
            return SOCKET_ERROR;
        if (line_input(s, buf, sizeof buf)  == SOCKET_ERROR)
            return SOCKET_ERROR;
        status_code = atoi(strchr(buf, ' '));
        do {
            if (line_input(s, buf, sizeof buf) == SOCKET_ERROR) {
                return SOCKET_ERROR;
            }
        } while (strcmp(buf,"\r\n") != 0);
        if (status_code != 200) {
			char uimsg[MAX_UIMSG] = {0};
			char tmp[MAX_UIMSG + 32];

            switch (status_code) {
            case 401:
            case 407:
                UTIL_get_lang_msg("MSG_PROXY_UNAUTHORIZED", uimsg, sizeof(uimsg),
                                  "Proxy authorization failed!");
                break;
            case 400:
            case 405:
            case 406:
            case 403:
				// 該当しないステータスコードだった場合、不定な内容のuimsg[]が
				// MessageBoxに表示される問題を修正した。
			default:
                UTIL_get_lang_msg("MSG_PROXY_BAD_REQUEST", tmp, sizeof(tmp),
                                  "Proxy prevent this connection!");
				_snprintf_s(uimsg, sizeof(uimsg), _TRUNCATE, "%s(HTTP: status code %d)", 
					tmp, status_code);
                break;
            }
            return setError(s, uimsg);
        }
        return 0;
    }

    enum {
        SOCKS5_VERSION          = 5,
        SOCKS5_REJECT           = 0xFF,    /* No acceptable */
        SOCKS5_CMD_CONNECT      = 1,
        SOCKS5_ATYP_IPv4        = 1,
        SOCKS5_ATYP_DOMAINNAME  = 3,
        SOCKS5_ATYP_IPv6        = 4,
        SOCKS5_AUTH_SUBNEGOVER  = 1,

        /* informations for SOCKS */
        SOCKS5_REP_SUCCEEDED    = 0x00,    /* succeeded */
        SOCKS5_REP_FAIL         = 0x01,    /* general SOCKS serer failure */
        SOCKS5_REP_NALLOWED     = 0x02,    /* connection not allowed by ruleset */
        SOCKS5_REP_NUNREACH     = 0x03,    /* Network unreachable */
        SOCKS5_REP_HUNREACH     = 0x04,    /* Host unreachable */
        SOCKS5_REP_REFUSED      = 0x05,    /* connection refused */
        SOCKS5_REP_EXPIRED      = 0x06,    /* TTL expired */
        SOCKS5_REP_CNOTSUP      = 0x07,    /* Command not supported */
        SOCKS5_REP_ANOTSUP      = 0x08,    /* Address not supported */
        SOCKS5_REP_INVADDR      = 0x09,    /* Inalid address */

        /* SOCKS5 authentication methods */
        SOCKS5_AUTH_NOAUTH      = 0x00,    /* without authentication */
        SOCKS5_AUTH_GSSAPI      = 0x01,    /* GSSAPI */
        SOCKS5_AUTH_USERPASS    = 0x02,    /* User/Password */
        SOCKS5_AUTH_CHAP        = 0x03,    /* Challenge-Handshake Auth Proto. */
        SOCKS5_AUTH_EAP         = 0x05,    /* Extensible Authentication Proto. */
        SOCKS5_AUTH_MAF         = 0x08,    /* Multi-Authentication Framework */
    };
    int begin_relay_socks5(ProxyInfo& proxy, String realhost, unsigned short realport, SOCKET s) {
        int len;
        unsigned char auth_method;
        int auth_result;
        unsigned char buf[256];
        unsigned char* ptr = buf;
        char uimsg[MAX_UIMSG];
        *ptr++ = SOCKS5_VERSION;
        if (proxy.user != NULL && proxy.pass != NULL) {
            *ptr++ = 2; // support 2 auth methods : SOCKS5_AUTH_NOAUTH, SOCKS5_AUTH_USERPASS
            *ptr++ = SOCKS5_AUTH_NOAUTH;
            *ptr++ = SOCKS5_AUTH_USERPASS;
        }else{
            *ptr++ = 1; // support only 1 auth method : SOCKS5_AUTH_NOAUTH
            *ptr++ = SOCKS5_AUTH_NOAUTH;
        }
        if (sendToSocket(s, buf, ptr - buf) == SOCKET_ERROR)
            return SOCKET_ERROR;
        if (recieveFromSocket(s, buf, 2) == SOCKET_ERROR)
            return SOCKET_ERROR;

        if (buf[0] != SOCKS5_VERSION || buf[1] == SOCKS5_REJECT) {
            char uimsg[MAX_UIMSG];
            UTIL_get_lang_msg("MSG_PROXY_BAD_REQUEST", uimsg, sizeof(uimsg),
                              "Proxy prevent this connection!");
            return setError(s, uimsg);
        }
        auth_method = buf[1];

        auth_result = -1;
        switch (auth_method) {
        case SOCKS5_AUTH_NOAUTH:
            /* nothing to do */
            auth_result = 0;
            break;
        case SOCKS5_AUTH_USERPASS:
            /* make authentication packet */
            ptr = buf;
            *ptr++ = SOCKS5_AUTH_SUBNEGOVER;
            len = proxy.user.length();
            *ptr++ = len;
            strcpy_s((char*) ptr, sizeof buf - (ptr - buf), proxy.user);
            ptr += len;
            len = proxy.pass.length();
            *ptr++ = len;
            strcpy_s((char*) ptr, sizeof buf - (ptr - buf), proxy.pass);
            ptr += len;

            /* send it and get answer */
            if (sendToSocket(s, buf, ptr - buf) == SOCKET_ERROR) 
                return SOCKET_ERROR;
            if (recieveFromSocket(s, buf, 2) == SOCKET_ERROR) 
                return SOCKET_ERROR;

            /* check status */
            auth_result = buf[1] != 0;
            break;
        default:
            UTIL_get_lang_msg("MSG_PROXY_BAD_REQUEST", uimsg, sizeof(uimsg),
                              "Proxy prevent this connection!");
            return setError(s, uimsg);
        }
        if (auth_result == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }else if (auth_result != 0) {
            UTIL_get_lang_msg("MSG_PROXY_UNAUTHORIZED", uimsg, sizeof(uimsg),
                              "Proxy authorization failed!");
            return setError(s, uimsg);
        }
        /* request to connect */
        ptr = buf;
        *ptr++ = SOCKS5_VERSION;
        *ptr++ = SOCKS5_CMD_CONNECT;
        *ptr++ = 0; /* reserved */
        struct addrinfo hints, *res = NULL;
        if (resolve != RESOLVE_REMOTE) {
            memset(&hints, 0, sizeof(hints));
            getaddrinfo(realhost, NULL, &hints, &res);
        }
        if (res) {
            switch (res->ai_family) {
              case AF_INET:
                *ptr++ = SOCKS5_ATYP_IPv4;
                memcpy(ptr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(in_addr));
                ptr += sizeof(in_addr);
                break;
              case AF_INET6:
                *ptr++ = SOCKS5_ATYP_IPv6;
                memcpy(ptr, &((struct sockaddr_in6*)res->ai_addr)->sin6_addr, sizeof(in6_addr));
                ptr += sizeof(in6_addr);
                break;
              default:
                freeaddrinfo(res);
                WSASetLastError(WSAECONNREFUSED);
                return SOCKET_ERROR;
                break;
            }
            freeaddrinfo(res);
        }
        else if (resolve == RESOLVE_LOCAL) {
            WSASetLastError(WSAECONNREFUSED);
            return SOCKET_ERROR;
        }
        else {
            *ptr++ = SOCKS5_ATYP_DOMAINNAME;
            len = strlen(realhost);
            *ptr++ = len;
            memcpy(ptr, realhost, len);
            ptr += len;
        }
        *ptr++ = realport >> 8;     /* DST.PORT */
        *ptr++ = realport & 0xFF;
        if (sendToSocket(s, buf, ptr - buf) == SOCKET_ERROR)
            return SOCKET_ERROR;
        if (recieveFromSocket(s, buf, 4) == SOCKET_ERROR)
            return SOCKET_ERROR;
		/* SOCKSリクエストに対するリプライ
		   
		   buf[0] VER  protocol version: X'05'
		   buf[1] REP  Reply field: 
 				 o  X'00' succeeded
				 o  X'01' general SOCKS server failure
				 o  X'02' connection not allowed by ruleset
				 o  X'03' Network unreachable
				 o  X'04' Host unreachable
				 o  X'05' Connection refused
				 o  X'06' TTL expired
				 o  X'07' Command not supported
				 o  X'08' Address type not supported
				 o  X'09' to X'FF' unassigned
		   buf[2] RSV    RESERVED: X'00'
		   buf[3] ATYP   address type of following address
				 o  IP V4 address: X'01'
				 o  DOMAINNAME: X'03'
				 o  IP V6 address: X'04'
		   buf[4:N] BND.ADDR       server bound address
		   buf[N+1] BND.PORT       server bound port in network octet order 
		 */
        if (buf[0] != SOCKS5_VERSION || buf[1] != SOCKS5_REP_SUCCEEDED) {   /* check reply code */
			char tmp[MAX_UIMSG + 32];

			UTIL_get_lang_msg("MSG_PROXY_BAD_REQUEST", uimsg, sizeof(uimsg),
                              "Proxy prevent this connection!");
			// リプライ情報を追記してメッセージ表示する。
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%s(SOCKS5:VER %u REP %u ATYP %u)", 
				uimsg, buf[0], buf[1], buf[3]);
			return setError(s, tmp);
        }
        // buf[2] is reserved
        switch (buf[3]) { /* case by ATYP */
        case SOCKS5_ATYP_IPv4:
            /* recv IPv4 addr and port */
            if (recieveFromSocket(s, buf, 4 + 2) == SOCKET_ERROR)
                return SOCKET_ERROR;
            break;
        case SOCKS5_ATYP_DOMAINNAME:
            /* recv name and port */
            if (recieveFromSocket(s, buf, 1) == SOCKET_ERROR)
                return SOCKET_ERROR;
            if (recieveFromSocket(s, buf, buf[0] + 2) == SOCKET_ERROR)
                return SOCKET_ERROR;
            break;
        case SOCKS5_ATYP_IPv6:
            /* recv IPv6 addr and port */
            if (recieveFromSocket(s, buf, 16 + 2) == SOCKET_ERROR)
                return SOCKET_ERROR;
            break;
        }
        /* Conguraturation, connected via SOCKS5 server! */
        return 0;
    }

    enum {
        SOCKS4_VERSION          = 4,
        SOCKS4_CMD_CONNECT      = 1,

        SOCKS4_REP_SUCCEEDED    = 90,      /* rquest granted (succeeded) */
        SOCKS4_REP_REJECTED     = 91,      /* request rejected or failed */
        SOCKS4_REP_IDENT_FAIL   = 92,      /* cannot connect identd */
        SOCKS4_REP_USERID       = 93,      /* user id not matched */
    };

    int begin_relay_socks4(ProxyInfo& proxy, String realhost, unsigned short realport, SOCKET s) {
        unsigned char buf[256], *ptr;

        /* make connect request packet 
           protocol v4:
             VN:1, CD:1, PORT:2, ADDR:4, USER:n, NULL:1
           protocol v4a:
             VN:1, CD:1, PORT:2, DUMMY:4, USER:n, NULL:1, HOSTNAME:n, NULL:1
        */
        ptr = buf;
        *ptr++ = SOCKS4_VERSION;
        *ptr++ = SOCKS4_CMD_CONNECT;
        *ptr++ = realport >> 8;
        *ptr++ = realport & 0xFF;
        in_addr addr;
        addr.s_addr = INADDR_NONE;
        char ch = realhost.charAt(0);
        if ('0' <= ch && ch <= '9') {
            addr.s_addr = inet_addr(realhost);
        }
        if (addr.s_addr != INADDR_NONE) {
            memcpy(ptr, &addr, sizeof addr);
            ptr += sizeof addr;
        }else{
            struct hostent* entry = resolve != RESOLVE_REMOTE ? gethostbyname(realhost) : NULL;
            if (entry != NULL && entry->h_addrtype == AF_INET && entry->h_length == sizeof (in_addr)) {
                addr.s_addr = INADDR_LOOPBACK;
                memcpy(ptr, entry->h_addr_list[0], entry->h_length);
                ptr += entry->h_length;
            }else if (resolve == RESOLVE_LOCAL) {
                WSASetLastError(WSAECONNREFUSED);
                return SOCKET_ERROR;
            }else{
                /* destination IP  fake, protocol 4a */
                *ptr++ = 0;
                *ptr++ = 0;
                *ptr++ = 0;
                *ptr++ = 1;
            }
        }
        /* username */
        if (proxy.user != NULL) {
            strcpy_s((char*) ptr, sizeof buf - (ptr - buf), proxy.user);
            ptr += proxy.user.length() + 1;
        }else{
            *ptr++ = '\0';
        }
        if (addr.s_addr == INADDR_NONE) {
            /* destination host name (for protocol 4a) */
            strcpy_s((char*) ptr, sizeof buf - (ptr - buf), realhost);
            ptr += strlen(realhost) + 1;
        }
        /* send command and get response
           response is: VN:1, CD:1, PORT:2, ADDR:4 */
        if (sendToSocket(s, buf, ptr - buf) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        if (recieveFromSocket(s, buf, 8) == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
		/* SOCKS4の返答パケット
		 
		  buf[0] VN 常に0
		  buf[1] CD
		           90 request granted
		           91 request rejected or failed
		           92 request rejected becasue SOCKS server cannot connect to
	                  identd on the client
				   93 request rejected because the client program and identd
	                  report different user-ids
		  buf[2:3] DSTPORT ポート番号
		  buf[4:7] DSTIP   IPアドレス
		 */
        char uimsg[MAX_UIMSG];
        uimsg[0] = NULL;
        if (buf[0] != 0) {
            UTIL_get_lang_msg("MSG_PROXY_BAD_REQUEST", uimsg, sizeof(uimsg),
                              "Proxy prevent this connection!");
        }else if (buf[1] == SOCKS4_REP_IDENT_FAIL || buf[1] == SOCKS4_REP_USERID) {
            UTIL_get_lang_msg("MSG_PROXY_UNAUTHORIZED", uimsg, sizeof(uimsg),
                              "Proxy authorization failed!");
        }else if (buf[1] != SOCKS4_REP_SUCCEEDED) {
            UTIL_get_lang_msg("MSG_PROXY_BAD_REQUEST", uimsg, sizeof(uimsg),
                              "Proxy prevent this connection!");
        }
        if (uimsg[0] != NULL) {
			char tmp[MAX_UIMSG + 32];

			// SOCKSの返答パケットのVNとCDを追記してメッセージ表示する。
			_snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%s(SOCKS4:VN %u CD %u)", uimsg, buf[0], buf[1]);
            return setError(s, tmp);
        }
    
        /* Conguraturation, connected via SOCKS4 server! */
        return 0;
    }

    int begin_relay_telnet(ProxyInfo& proxy, String realhost, unsigned short realport, SOCKET s) {
        int err = 0;
        char uimsg[MAX_UIMSG];

        while (!err) {
            switch (wait_for_prompt(s, prompt_table, countof(prompt_table), 10)) {
            case 0: /* Hostname prompt */
                if (sendToSocketFormat(s, strchr((const char *)realhost,':')?"[%s]:%d\n":"%s:%d\n", (const char *)realhost, realport) == SOCKET_ERROR)
                    return SOCKET_ERROR;
                break;
            case 1: /* Username prompt */
                if (sendToSocketFormat(s, "%s\n", (const char *)proxy.user) == SOCKET_ERROR)
                    return SOCKET_ERROR;
                break;
            case 2: /* Password prompt */
                if (sendToSocketFormat(s, "%s\n", (const char *)proxy.pass) == SOCKET_ERROR)
                    return SOCKET_ERROR;
                break;
            case 3: /* Established message */
                return 0;
            case 4: /* Refused message */
            default: /* Connection error, etc. */
                err = 1;
                break;
            }
        }
        UTIL_get_lang_msg("MSG_PROXY_BAD_REQUEST", uimsg, sizeof(uimsg),
                          "Proxy prevent this connection!");
        return setError(s, uimsg);
    }

    int setError(SOCKET s, char *p) {
        if (*p != NULL)
            showMessage(p);
        HWND window;
        UINT message;
        long event;
        asyncselectinfo.get(s, window, message, event);
        if ((event & FD_CONNECT) != 0) {
            MSG msg;
            while (::PeekMessage(&msg, window, message, message, PM_REMOVE))
                ;
        }
        WSASetLastError(WSAECONNREFUSED);
        return SOCKET_ERROR;
    }
    void showMessage(char *p) {
        if (shower != NULL) {
            shower->showMessage(p);
        }
    }

#define DECLARE_HOOKAPI(RETTYPE, APINAME, ARGLIST, ARGS)   \
public:                                                    \
    static FARPROC hook_##APINAME(FARPROC original) {      \
        (FARPROC&) instance().ORIG_##APINAME = original;   \
        return (FARPROC) HOOKEDAPI_##APINAME;              \
    }                                                      \
    static FARPROC unhook_##APINAME(FARPROC hookedProc) {  \
        return hookedProc == (FARPROC) HOOKEDAPI_##APINAME \
            ? (FARPROC) instance().ORIG_##APINAME          \
            : hookedProc;                                  \
    }                                                      \
private:                                                   \
    static RETTYPE PASCAL HOOKEDAPI_##APINAME ARGLIST {    \
        return instance().hooked_##APINAME ARGS;           \
    }                                                      \
    RETTYPE (PASCAL* ORIG_##APINAME) ARGLIST;              \
    RETTYPE hooked_##APINAME ARGLIST

#define INSTALL_HOOKAPI(APINAME) \
    Hooker::hook(module, (FARPROC) instance().ORIG_##APINAME, (FARPROC) HOOKEDAPI_##APINAME);

#define UNINSTALL_HOOKAPI(APINAME) \
    Hooker::hook(module, (FARPROC) HOOKEDAPI_##APINAME, (FARPROC) instance().ORIG_##APINAME);

#define LOADAPI(APINAME) \
    (FARPROC&) ORIG_##APINAME = ::GetProcAddress(ws2_32, #APINAME);

#define SETUP_HOOKAPI(APINAME) \
    Hooker::setup((FARPROC) instance().ORIG_##APINAME, (FARPROC) HOOKEDAPI_##APINAME);

    DECLARE_HOOKAPI(int, connect, (SOCKET s, const struct sockaddr* name, int namelen), (s, name, namelen)) {
        ConnectionInfo* info = NULL;
        ConnectionInfoHolder holder;
        if (name->sa_family == AF_INET) {
            struct sockaddr_in* in = (struct sockaddr_in*) name;
            info = connectioninfolist.get(in->sin_addr);
            if (info == NULL && defaultProxy.type != ProxyInfo::TYPE_NONE) {
                info = new ConnectionInfo(defaultProxy, inet_ntoa(in->sin_addr));
                holder = info;
            }
        }
        else if (name->sa_family == AF_INET6) {
            struct sockaddr_in6* in6 = (struct sockaddr_in6*) name;
            info = connectioninfolist.get(in6->sin6_addr);
            if (info == NULL && defaultProxy.type != ProxyInfo::TYPE_NONE) {
                DWORD bufflen = 64;
                char* buff = new char[64];
                WSAAddressToString((struct sockaddr*)name, sizeof(struct sockaddr_in6), NULL, buff, &bufflen);
                info = new ConnectionInfo(defaultProxy, buff);
                holder = info;
            }
        }
        if (info != NULL) {
            if (info->proxy.type == ProxyInfo::TYPE_NONE_FORCE) {
                info = NULL;
            }else{
                const char* hostname;
                if (info->proxy.type == ProxyInfo::TYPE_SSL) {
                    hostname = info->realhost;
                }else{
                    info->realport = ntohs(((struct sockaddr_in*) name)->sin_port);
                    if (name->sa_family == AF_INET) {
                        ((struct sockaddr_in*) name)->sin_port = htons(info->proxy.getPort());
                    } else { // AF_INET6
                        ((struct sockaddr_in6*) name)->sin6_port = htons(info->proxy.getPort());
                    }
                    hostname = info->proxy.host;
                }
                struct addrinfo hints, *res;
                memset(&hints, 0, sizeof hints);
                hints.ai_family = name->sa_family;
                if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
                    WSASetLastError(WSAECONNREFUSED);
                    return SOCKET_ERROR;
                }
                if (name->sa_family == AF_INET) {
                    memcpy(&((struct sockaddr_in*) name)->sin_addr, &((struct sockaddr_in*)res->ai_addr)->sin_addr, sizeof(struct in_addr));
                } else { // AF_INET6
                    memcpy(&((struct sockaddr_in6*) name)->sin6_addr, &((struct sockaddr_in6*)res->ai_addr)->sin6_addr, sizeof(struct in6_addr));
                }
                freeaddrinfo(res);
            }
        }
        int result = ORIG_connect(s, name, namelen);
        if (info == NULL || info->proxy.port == 0) {
            return result;
        }
        if (result == 0) {
            // do nothing
        }else if (WSAGetLastError() == WSAEWOULDBLOCK) {
            struct timeval tv = {timeout, 0};
            fd_set ifd;
            fd_set ofd;
            fd_set efd;
            FD_ZERO(&ifd);
            FD_ZERO(&ofd);
            FD_ZERO(&efd);
            FD_SET(s, &ifd);
            FD_SET(s, &ofd);
            FD_SET(s, &efd);
            if (select((int) (s + 1), &ifd, &ofd, &efd, timeout > 0 ? &tv : NULL) == SOCKET_ERROR)
                return SOCKET_ERROR;
            if (FD_ISSET(s, &efd)) {
				// Proxy serverへのconnectが失敗した場合、意図的に WSAECONNREFUSED エラーを
				// セットするのをやめた。
				// Proxy serverがホスト名として設定されていて、かつデュアルスタック環境の場合、
				// Tera Term側でのIPv6/IPv4フォールバックが期待通りに動いていなかった。
				// また、Proxy serverにまったく接続できない場合、Connection refusedダイアログが
				// 3回連続表示されるという動作にもなっていた。
                return SOCKET_ERROR;
            }
        }else{
            return SOCKET_ERROR;
        }
        switch (info->proxy.type) {
        default:
            result = 0;
            break;
        case ProxyInfo::TYPE_HTTP:
        case ProxyInfo::TYPE_HTTP_SSL:
            result = begin_relay_http(info->proxy, info->realhost, info->realport, s);
            break;
        case ProxyInfo::TYPE_TELNET:
        case ProxyInfo::TYPE_TELNET_SSL:
            result = begin_relay_telnet(info->proxy, info->realhost, info->realport, s);
            break;
        case ProxyInfo::TYPE_SOCKS4:
        case ProxyInfo::TYPE_SOCKS4_SSL:
            result = begin_relay_socks4(info->proxy, info->realhost, info->realport, s);
            break;
        case ProxyInfo::TYPE_SOCKS5:
        case ProxyInfo::TYPE_SOCKS5_SSL:
            result = begin_relay_socks5(info->proxy, info->realhost, info->realport, s);
            break;
        }
        if (result == 0) {
            if (info->proxy.type == ProxyInfo::TYPE_SSL
                || info->proxy.type == ProxyInfo::TYPE_HTTP_SSL
                || info->proxy.type == ProxyInfo::TYPE_TELNET_SSL
                || info->proxy.type == ProxyInfo::TYPE_SOCKS4_SSL
                || info->proxy.type == ProxyInfo::TYPE_SOCKS5_SSL) {
                if (!SSLSocket::isEnabled()) {
                    WSASetLastError(WSAECONNREFUSED);
                    return SOCKET_ERROR;
                }
                SSLSocket* ssl = new SSLSocket();
                if (!ssl->connect(s, info->realhost)) {
                    shutdown(s, SD_BOTH);
                    int error_code = ssl->get_verify_result();
                    char uimsg[MAX_UIMSG];
                    delete ssl;
                    switch (error_code) {
                        case IDS_UNABLE_TO_GET_ISSUER_CERT:
                            UTIL_get_lang_msg("MSG_UNABLE_TO_GET_ISSUER_CERT", uimsg, sizeof(uimsg),
                                              "Unable to get Issuer Certificate");
                            break;
                        case IDS_UNABLE_TO_GET_CRL:
                            UTIL_get_lang_msg("MSG_UNABLE_TO_GET_CRL", uimsg, sizeof(uimsg),
                                              "Unable to get Certificate CRL");
                            break;
                        case IDS_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
                            UTIL_get_lang_msg("MSG_UNABLE_TO_DECRYPT_CERT_SIGNATURE", uimsg, sizeof(uimsg),
                                              "Unable to decrypt Certificate's Signature");
                            break;
                        case IDS_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
                            UTIL_get_lang_msg("MSG_UNABLE_TO_DECRYPT_CRL_SIGNATURE", uimsg, sizeof(uimsg),
                                              "Unable to decrypt CRL's Signature");
                            break;
                        case IDS_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
                            UTIL_get_lang_msg("MSG_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY", uimsg, sizeof(uimsg),
                                              "Unable to decode Issuer Public Key");
                            break;
                        case IDS_CERT_SIGNATURE_FAILURE:
                            UTIL_get_lang_msg("MSG_CERT_SIGNATURE_FAILURE", uimsg, sizeof(uimsg),
                                              "Certificate Signature failure");
                            break;
                        case IDS_CRL_SIGNATURE_FAILURE:
                            UTIL_get_lang_msg("MSG_CRL_SIGNATURE_FAILURE", uimsg, sizeof(uimsg),
                                              "CRL Signature failure");
                            break;
                        case IDS_CERT_NOT_YET_VALID:
                            UTIL_get_lang_msg("MSG_CERT_NOT_YET_VALID", uimsg, sizeof(uimsg),
                                              "Certificate is not yet valid");
                            break;
                        case IDS_CERT_HAS_EXPIRED:
                            UTIL_get_lang_msg("MSG_CERT_HAS_EXPIRED", uimsg, sizeof(uimsg),
                                              "Certificate has expired");
                            break;
                        case IDS_CRL_NOT_YET_VALID:
                            UTIL_get_lang_msg("MSG_CRL_NOT_YET_VALID", uimsg, sizeof(uimsg),
                                              "CRL is not yet valid");
                            break;
                        case IDS_CRL_HAS_EXPIRED:
                            UTIL_get_lang_msg("MSG_CRL_HAS_EXPIRED", uimsg, sizeof(uimsg),
                                              "CRL has expired");
                            break;
                        case IDS_ERROR_IN_CERT_NOT_BEFORE_FIELD:
                            UTIL_get_lang_msg("MSG_ERROR_IN_CERT_NOT_BEFORE_FIELD", uimsg, sizeof(uimsg),
                                              "Format error in Certificate's notBefore field");
                            break;
                        case IDS_ERROR_IN_CERT_NOT_AFTER_FIELD:
                            UTIL_get_lang_msg("MSG_ERROR_IN_CERT_NOT_AFTER_FIELD", uimsg, sizeof(uimsg),
                                              "Format error in Certificate's notAfter field");
                            break;
                        case IDS_ERROR_IN_CRL_LAST_UPDATE_FIELD:
                            UTIL_get_lang_msg("MSG_ERROR_IN_CRL_LAST_UPDATE_FIELD", uimsg, sizeof(uimsg),
                                              "Format error in CRL's lastUpdate field");
                            break;
                        case IDS_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
                            UTIL_get_lang_msg("MSG_ERROR_IN_CRL_NEXT_UPDATE_FIELD", uimsg, sizeof(uimsg),
                                              "Format error in CRL's nextUpdate field");
                            break;
                        case IDS_OUT_OF_MEM:
                            UTIL_get_lang_msg("MSG_OUT_OF_MEM", uimsg, sizeof(uimsg),
                                              "Out of memory");
                            break;
                        case IDS_DEPTH_ZERO_SELF_SIGNED_CERT:
                            UTIL_get_lang_msg("MSG_DEPTH_ZERO_SELF_SIGNED_CERT", uimsg, sizeof(uimsg),
                                              "Self-signed Certificate");
                            break;
                        case IDS_SELF_SIGNED_CERT_IN_CHAIN:
                            UTIL_get_lang_msg("MSG_SELF_SIGNED_CERT_IN_CHAIN", uimsg, sizeof(uimsg),
                                              "Self-signed Certificate found in Certificate chain");
                            break;
                        case IDS_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
                            UTIL_get_lang_msg("MSG_UNABLE_TO_GET_ISSUER_CERT_LOCALLY", uimsg, sizeof(uimsg),
                                              "Unable to get Local Issuer Certificate");
                            break;
                        case IDS_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
                            UTIL_get_lang_msg("MSG_UNABLE_TO_VERIFY_LEAF_SIGNATURE", uimsg, sizeof(uimsg),
                                              "Unable to verify the first Certificate");
                            break;
                        case IDS_CERT_CHAIN_TOO_LONG:
                            UTIL_get_lang_msg("MSG_CERT_CHAIN_TOO_LONG", uimsg, sizeof(uimsg),
                                              "Certificate chain too long");
                            break;
                        case IDS_CERT_REVOKED:
                            UTIL_get_lang_msg("MSG_CERT_REVOKED", uimsg, sizeof(uimsg),
                                              "Certificate revoked");
                            break;
                        case IDS_INVALID_CA:
                            UTIL_get_lang_msg("MSG_INVALID_CA", uimsg, sizeof(uimsg),
                                              "Invalid CA Certificate");
                            break;
                        case IDS_PATH_LENGTH_EXCEEDED:
                            UTIL_get_lang_msg("MSG_PATH_LENGTH_EXCEEDED", uimsg, sizeof(uimsg),
                                              "Path length constraint exceeded");
                            break;
                        case IDS_INVALID_PURPOSE:
                            UTIL_get_lang_msg("MSG_INVALID_PURPOSE", uimsg, sizeof(uimsg),
                                              "Unsupported Certificate purpose");
                            break;
                        case IDS_CERT_UNTRUSTED:
                            UTIL_get_lang_msg("MSG_CERT_UNTRUSTED", uimsg, sizeof(uimsg),
                                              "Certificate not trusted");
                            break;
                        case IDS_CERT_REJECTED:
                            UTIL_get_lang_msg("MSG_CERT_REJECTED", uimsg, sizeof(uimsg),
                                              "Certificate rejected");
                            break;
                        case IDS_SUBJECT_ISSUER_MISMATCH:
                            UTIL_get_lang_msg("MSG_SUBJECT_ISSUER_MISMATCH", uimsg, sizeof(uimsg),
                                              "Subject Issuer mismatch");
                            break;
                        case IDS_AKID_SKID_MISMATCH:
                            UTIL_get_lang_msg("MSG_AKID_SKID_MISMATCH", uimsg, sizeof(uimsg),
                                              "Authority and Subject Key Identifier mismatch");
                            break;
                        case IDS_AKID_ISSUER_SERIAL_MISMATCH:
                            UTIL_get_lang_msg("MSG_AKID_ISSUER_SERIAL_MISMATCH", uimsg, sizeof(uimsg),
                                              "Authority and Issuer Serial Number mismatch");
                            break;
                        case IDS_KEYUSAGE_NO_CERTSIGN:
                            UTIL_get_lang_msg("MSG_KEYUSAGE_NO_CERTSIGN", uimsg, sizeof(uimsg),
                                              "Key usage does not include Certificate signing");
                            break;
                        case IDS_APPLICATION_VERIFICATION:
                            UTIL_get_lang_msg("MSG_APPLICATION_VERIFICATION", uimsg, sizeof(uimsg),
                                              "Application verification failure");
                            break;
                        case IDS_UNMATCH_COMMON_NAME:
                            UTIL_get_lang_msg("MSG_UNMATCH_COMMON_NAME", uimsg, sizeof(uimsg),
                                              "Unmatch Common Name");
                            break;
                        case IDS_UNABLE_TO_GET_COMMON_NAME:
                            UTIL_get_lang_msg("MSG_UNABLE_TO_GET_COMMON_NAME", uimsg, sizeof(uimsg),
                                              "Unable to get Common Name");
                            break;
                        default:
                            uimsg[0] = NULL;
                            break;
                    }
                    return setError(s, uimsg);
                }
                sslmap.put(s,ssl);
            }
        }
        return result;
    }

    DECLARE_HOOKAPI(struct hostent*, gethostbyname, (const char* hostname), (hostname)) {
        ConnectionInfo* info = connectioninfolist.find(hostname);
        if (info != NULL) {
            if (info->proxy.type == ProxyInfo::TYPE_NONE_FORCE) {
                hostname = info->realhost;
            }else{
                if (info->proxy.type == ProxyInfo::TYPE_SSL
                    || info->proxy.type == ProxyInfo::TYPE_HTTP_SSL
                    || info->proxy.type == ProxyInfo::TYPE_TELNET_SSL
                    || info->proxy.type == ProxyInfo::TYPE_SOCKS4_SSL
                    || info->proxy.type == ProxyInfo::TYPE_SOCKS5_SSL) {
                    if (!SSLSocket::isEnabled()) {
                        ::WSASetLastError(WSAHOST_NOT_FOUND);
                        return NULL;
                    }
                }
                if (info->buffer == NULL) {
                    int bufferLength = sizeof (DUMMYHOSTENT) + strlen(info->realhost);
                    info->buffer = new char[bufferLength];
                    info->fillBuffer(info->buffer, bufferLength);
                }
                return (struct hostent*) info->buffer;
            }
        }
        return ORIG_gethostbyname(hostname);
    }

    DECLARE_HOOKAPI(HANDLE, WSAAsyncGetHostByName, (HWND window, UINT message, const char* hostname, char* buffer, int bufferLength), (window, message, hostname, buffer, bufferLength)) {
        ConnectionInfo* info = connectioninfolist.find(hostname);
        if (info == NULL || info->proxy.type == ProxyInfo::TYPE_NONE_FORCE) {
            return ORIG_WSAAsyncGetHostByName(window, message, hostname, buffer, bufferLength);
        }
        if (info->proxy.type == ProxyInfo::TYPE_SSL
            || info->proxy.type == ProxyInfo::TYPE_HTTP_SSL
            || info->proxy.type == ProxyInfo::TYPE_TELNET_SSL
            || info->proxy.type == ProxyInfo::TYPE_SOCKS4_SSL
            || info->proxy.type == ProxyInfo::TYPE_SOCKS5_SSL) {
            if (!SSLSocket::isEnabled()) {
                ::WSASetLastError(WSAHOST_NOT_FOUND);
                return NULL;
            }
        }
        HANDLE handle = connectioninfolist.getTask(info);
        int len = sizeof (DUMMYHOSTENT) + strlen(hostname);
        if (bufferLength < len) {
            ::PostMessage(window, message, (WPARAM) handle, MAKELPARAM(len, WSAENOBUFS));
            ::WSASetLastError(WSAENOBUFS);
            return handle;
        }
        info->fillBuffer(buffer, bufferLength);
        ::PostMessage(window, message, (WPARAM) handle, MAKELPARAM(len, 0));
        return handle;
    }

    DECLARE_HOOKAPI(HANDLE, WSAAsyncGetAddrInfo, (HWND window, UINT message, const char* hostname, const char* portname, struct addrinfo* hints, struct addrinfo** res), (window, message, hostname, portname, hints, res)) {
        ConnectionInfo* info = connectioninfolist.find(hostname);
        if (info == NULL || info->proxy.type == ProxyInfo::TYPE_NONE_FORCE) {
            return ORIG_WSAAsyncGetAddrInfo(window, message, hostname, portname, hints, res);
        }
        if (info->proxy.type == ProxyInfo::TYPE_SSL
            || info->proxy.type == ProxyInfo::TYPE_HTTP_SSL
            || info->proxy.type == ProxyInfo::TYPE_TELNET_SSL
            || info->proxy.type == ProxyInfo::TYPE_SOCKS4_SSL
            || info->proxy.type == ProxyInfo::TYPE_SOCKS5_SSL) {
            if (!SSLSocket::isEnabled()) {
                ::WSASetLastError(WSAHOST_NOT_FOUND);
                return NULL;
            }
        }
        HANDLE handle = connectioninfolist.getTask(info);
        int bufferLength = sizeof (DUMMYHOSTENT) + strlen(hostname) + 1;
        info->buffer = new char[bufferLength];
        info->fillBuffer(info->buffer, bufferLength, portname, hints->ai_family);
        DUMMYHOSTENT* d = (DUMMYHOSTENT*)info->buffer;
        *res = d->ai;
        if (aicount < 256) {
            ailist[aicount++] = d->ai;
        }
        ::PostMessage(window, message, (WPARAM) handle, MAKELPARAM(0, 0));
        return handle;
    }

    DECLARE_HOOKAPI(void, freeaddrinfo, (struct addrinfo* ai), (ai)) {
        int i;
        for (i=0; i<aicount; i++) {
            if (ailist[i] == ai) {
                return;
            }
        }
        ORIG_freeaddrinfo(ai);
        return;
    }

    DECLARE_HOOKAPI(int, WSAAsyncSelect, (SOCKET s, HWND window, UINT message, long event), (s, window, message, event)) {
        asyncselectinfo.put(s, window, message, event);
        return ORIG_WSAAsyncSelect(s, window, message, event);
    }

    DECLARE_HOOKAPI(int, WSACancelAsyncRequest, (HANDLE task), (task)) {
        ConnectionInfo* info = connectioninfolist.get(task);
        if (info != NULL) {
            return 0;
        }
        return ORIG_WSACancelAsyncRequest(task);
    }

    DECLARE_HOOKAPI(int, send, (SOCKET s, const char* buf, int len, int flags), (s, buf, len, flags)) {
        SSLSocket* ssl = sslmap.get(s);
        if (ssl != NULL)
            return ssl->write(buf, len);
        return ORIG_send(s, buf, len, flags);
    }
    DECLARE_HOOKAPI(int, sendto, (SOCKET s, const char* buf, int len, int flags,   const struct sockaddr* to, int tolen), (s, buf, len, flags, to, tolen)) {
        return ORIG_sendto(s, buf, len, flags, to, tolen);
    }
    DECLARE_HOOKAPI(int, recv, (SOCKET s, char* buf, int len, int flags), (s, buf, len, flags)) {
        SSLSocket* ssl = sslmap.get(s);
        if (ssl != NULL)
            return ssl->read(buf, len);
        return ORIG_recv(s, buf, len, flags);
    }
    DECLARE_HOOKAPI(int, recvfrom, (SOCKET s, char* buf, int len, int flags, struct sockaddr* from, int* fromlen), (s, buf, len, flags, from, fromlen)) {
        return ORIG_recvfrom(s, buf, len, flags, from, fromlen);
    }
    DECLARE_HOOKAPI(int, closesocket, (SOCKET s), (s)) {
        SSLSocket* ssl = sslmap.get(s);
        if (ssl != NULL) {
            ssl->close();
            sslmap.remove(s);
            delete ssl;
        }
        return ORIG_closesocket(s);
    }

    String logfile;
    int timeout;
    enum SocksResolve {
        RESOLVE_AUTO,
        RESOLVE_REMOTE,
        RESOLVE_LOCAL,
    } resolve;
    String prompt_table[5];
    ProxyInfo defaultProxy;
    ConnectionInfoList connectioninfolist;
    Hashtable<SOCKET, SSLSocket*> sslmap;
    MessageShower* shower;
    HWND owner;
    HMODULE resource_module;
    AsyncSelectInfoTable asyncselectinfo;
    struct addrinfo* ailist[256];
    int aicount;

    ProxyWSockHook():shower(NULL), owner(NULL), resource_module(GetInstanceHandle()), timeout(0), aicount(0) {
        HMODULE ws2_32 = ::GetModuleHandle("ws2_32.dll");
        LOADAPI(connect)
        LOADAPI(gethostbyname)
        LOADAPI(WSAAsyncGetHostByName)
        LOADAPI(WSAAsyncGetAddrInfo)
        LOADAPI(freeaddrinfo)
        LOADAPI(WSAAsyncSelect)
        LOADAPI(WSACancelAsyncRequest)
        LOADAPI(send)
        LOADAPI(sendto)
        LOADAPI(recv)
        LOADAPI(recvfrom)
        LOADAPI(closesocket)
    }
    static ProxyWSockHook& instance() {
        static ProxyWSockHook instance;
        return instance;
    }
    void _load(IniFile& ini) {
        String temp;
        temp = ini.getString("ProxyType");
        if (temp != NULL) {
            defaultProxy.type = ProxyInfo::parseType(temp);
            if (defaultProxy.type != ProxyInfo::TYPE_NONE) {
                defaultProxy.host = ini.getString("ProxyHost");
                if (defaultProxy.host == NULL || defaultProxy.type == ProxyInfo::TYPE_NONE_FORCE) {
                    defaultProxy.type = ProxyInfo::TYPE_NONE;
                }else{
                    defaultProxy.port = (unsigned short) ini.getInteger("ProxyPort");
                    defaultProxy.user = ini.getString("ProxyUser");
                    if (defaultProxy.user != NULL)
                        defaultProxy.pass = ini.getString("ProxyPass");
                }
            }
        }

        timeout = ini.getInteger("ConnectionTimeout", 10);

        temp = ini.getString("SocksResolve");
        if (temp.equalsIgnoreCase("remote")) {
            resolve = RESOLVE_REMOTE;
        }else if (temp.equalsIgnoreCase("local")) {
            resolve = RESOLVE_LOCAL;
        }else{
            resolve = RESOLVE_AUTO;
        }

        prompt_table[0] = ini.getString("TelnetHostnamePrompt", ">> Host name: ");
        prompt_table[1] = ini.getString("TelnetUsernamePrompt", "Username:");
        prompt_table[2] = ini.getString("TelnetPasswordPrompt", "Password:");
        prompt_table[3] = ini.getString("TelnetConnectedMessage", "-- Connected to ");
        prompt_table[4] = ini.getString("TelnetErrorMessage", "!!!!!!!!");

        logfile = ini.getString("DebugLog");
        Logger::open(logfile);
    }
    void _save(IniFile& ini) {
        const char* type = NULL;
        const char* host = NULL;
        const char* port = NULL;
        const char* user = NULL;
        const char* pass = NULL;
        if (defaultProxy.type != ProxyInfo::TYPE_NONE && defaultProxy.type != ProxyInfo::TYPE_NONE_FORCE && defaultProxy.host != NULL) {
            type = ProxyInfo::getTypeName(defaultProxy.type);
            host = defaultProxy.host;
            if (defaultProxy.port != 0) {
                char* buffer = (char*) alloca(6);
                _itoa_s(defaultProxy.port, buffer, 6, 10);
                port = buffer;
            }
            if (defaultProxy.user != NULL) {
                user = defaultProxy.user;
                if (defaultProxy.pass != NULL) {
                    pass = defaultProxy.pass;
                }
            }
        }
        ini.setString("ProxyType",  type);
        ini.setString("ProxyHost",  host);
        ini.setString("ProxyPort",  port);
        ini.setString("ProxyUser",  user);
        ini.setString("ProxyPass",  pass);

        ini.setInteger("ConnectionTimeout", timeout);

        switch (resolve) {
        case RESOLVE_REMOTE:
            type = "remote";
            break;
        case RESOLVE_LOCAL:
            type = "local";
            break;
        default:
            type = "auto";
            break;
        }
        ini.setString("SocksResolve", type);

        ini.setString("TelnetHostnamePrompt",   prompt_table[0]);
        ini.setString("TelnetUsernamePrompt",   prompt_table[1]);
        ini.setString("TelnetPasswordPrompt",   prompt_table[2]);
        ini.setString("TelnetConnectedMessage", prompt_table[3]);
        ini.setString("TelnetErrorMessage",     prompt_table[4]);

        ini.setString("DebugLog", logfile);
    }
public:
    static void setOwner(HWND owner) {
        instance().owner = owner;
    }
    static void setMessageShower(MessageShower* shower) {
        instance().shower = shower;
    }
    static void load(IniFile& ini) {
        instance()._load(ini);
    }
    static void save(IniFile& ini) {
        instance()._save(ini);
    }
    static bool setupDialog(HWND owner) {
        SettingDialog dlg;
        dlg.proxy = instance().defaultProxy;
        if (dlg.open(owner) == IDOK) {
            instance().defaultProxy = dlg.proxy;
            return true;
        }
        return false;
    }
    static bool aboutDialog(HWND owner) {
        AboutDialog dlg;
        if (dlg.open(owner) == IDOK) {
            return true;
        }
        return false;
    }


    static void setupHooks() {
        SETUP_HOOKAPI(connect)
        SETUP_HOOKAPI(gethostbyname)
        SETUP_HOOKAPI(WSAAsyncGetHostByName)
        SETUP_HOOKAPI(WSAAsyncGetAddrInfo)
        SETUP_HOOKAPI(freeaddrinfo)
        SETUP_HOOKAPI(WSAAsyncSelect)
        SETUP_HOOKAPI(WSACancelAsyncRequest)
        SETUP_HOOKAPI(send)
        SETUP_HOOKAPI(sendto)
        SETUP_HOOKAPI(recv)
        SETUP_HOOKAPI(recvfrom)
        SETUP_HOOKAPI(closesocket)
    }
    static void installHook(HMODULE module) {
        INSTALL_HOOKAPI(connect)
        INSTALL_HOOKAPI(gethostbyname)
        INSTALL_HOOKAPI(WSAAsyncGetHostByName)
        INSTALL_HOOKAPI(WSAAsyncGetAddrInfo)
        INSTALL_HOOKAPI(freeaddrinfo)
        INSTALL_HOOKAPI(WSAAsyncSelect)
        INSTALL_HOOKAPI(WSACancelAsyncRequest)
        INSTALL_HOOKAPI(send)
        INSTALL_HOOKAPI(sendto)
        INSTALL_HOOKAPI(recv)
        INSTALL_HOOKAPI(recvfrom)
        INSTALL_HOOKAPI(closesocket)
    }

    static void uninstallHook(HMODULE module) {
        UNINSTALL_HOOKAPI(connect)
        UNINSTALL_HOOKAPI(gethostbyname)
        UNINSTALL_HOOKAPI(WSAAsyncGetHostByName)
        UNINSTALL_HOOKAPI(WSAAsyncGetAddrInfo)
        UNINSTALL_HOOKAPI(freeaddrinfo)
        UNINSTALL_HOOKAPI(WSAAsyncSelect)
        UNINSTALL_HOOKAPI(WSACancelAsyncRequest)
        UNINSTALL_HOOKAPI(send)
        UNINSTALL_HOOKAPI(sendto)
        UNINSTALL_HOOKAPI(recv)
        UNINSTALL_HOOKAPI(recvfrom)
        UNINSTALL_HOOKAPI(closesocket)
    }
    static String generateURL() {
        return instance().defaultProxy.generateURL();
    }
        static String parseURL(const char* url, BOOL prefix) {
        ProxyInfo proxy;
        String realhost = ProxyInfo::parse(url, proxy);
        if (realhost != NULL) {
            if (realhost.indexOf("://") != -1 && !prefix) {
                proxy.type = proxy.TYPE_NONE;
            }
            if (realhost.length() == 0) {
                realhost = NULL;
                proxy.type = proxy.TYPE_NONE;
            }
            instance().defaultProxy = proxy;
        }
        return realhost;
    }
    static void setResourceModule(HMODULE module) {
        instance().resource_module = module;
    }
};

#endif//_YEBISOCKS_PROXYWSOCKHOOK_H_
