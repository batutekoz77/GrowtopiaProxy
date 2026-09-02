#pragma once
#include "Functions.hpp"

/* @important: Socks5.hpp brings in winsock2.h, which has to be included
   before windows.h -- otherwise windows.h pulls in winsock 1.1 and every
   socket symbol below collides. Keep it above the includes that follow. */
#include "../Network/Socks5.hpp"

#include "../Logger/Logger.hpp"
#include "../Main/Config.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#endif

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>   /* atoi, for the config parser */
#include <atomic>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>


bool System::RelaunchAsAdmin() {
    wchar_t szPath[MAX_PATH];
    if (!GetModuleFileNameW(NULL, szPath, MAX_PATH)) return false;

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = szPath;
    sei.hwnd = NULL;
    sei.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei)) {
        DWORD dwErr = GetLastError();
        if (dwErr == ERROR_CANCELLED) return false;
    }

    ExitProcess(0);
    return true;
}

bool System::IsTrueAdmin() {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdministratorsGroup = NULL;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(
        &NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsGroup))
    {
        CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin);
        FreeSid(pAdministratorsGroup);
    }

    return fIsRunAsAdmin;
}

bool System::EnableDebugPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) return false;
    if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) return false;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    CloseHandle(hToken);

    return GetLastError() == ERROR_SUCCESS;
}

bool System::editHosts(std::string ipAddress) {
    std::string hostsPath = "C:\\Windows\\System32\\drivers\\etc\\hosts";

    std::string hostsContent = "";
    hostsContent += "# Copyright (c) 1993-2009 Microsoft Corp.\n";
    hostsContent += "#\n";
    hostsContent += "# This is a sample HOSTS file used by Microsoft TCP/IP for Windows.\n";
    hostsContent += "#\n";
    hostsContent += "# This file contains the mappings of IP addresses to host names. Each\n";
    hostsContent += "# entry should be kept on an individual line. The IP address should\n";
    hostsContent += "# be placed in the first column followed by the corresponding host name.\n";
    hostsContent += "# The IP address and the host name should be separated by at least one\n";
    hostsContent += "# space.\n";
    hostsContent += "#\n";
    hostsContent += "# Additionally, comments (such as these) may be inserted on individual\n";
    hostsContent += "# lines or following the machine name denoted by a '#' symbol.\n";
    hostsContent += "#\n";
    hostsContent += "# For example:\n";
    hostsContent += "#\n";
    hostsContent += "#      102.54.94.97     rhino.acme.com          # source server\n";
    hostsContent += "#       38.25.63.10     x.acme.com              # x client host\n";
    hostsContent += "\n";
    hostsContent += "# localhost name resolution is handled within DNS itself.\n";
    hostsContent += "#\t127.0.0.1       localhost\n";
    hostsContent += "#\t::1             localhost\n";

    if (!ipAddress.empty()) {
        hostsContent += ipAddress + " www.growtopia1.com\n";
        hostsContent += ipAddress + " www.growtopia2.com\n";
    }

    std::ofstream hostsFile(hostsPath, std::ios::trunc);
    if (!hostsFile.is_open()) {
        LOG_DEBUG("Couldn't edit the  hosts file");
        return false;
    }

    hostsFile << hostsContent;
    hostsFile.close();

    FastLog::Logger::set_thread_name("FUNCTIONS");
    LOG_DEBUG("Edited hosts file [{}]", ipAddress);

    return true;
}

int System::findProcess(const std::wstring& program) {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return -1;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, program.c_str()) == 0) {
                CloseHandle(snapshot);
                return static_cast<int>(entry.th32ProcessID);
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return -1;
}

void System::endProcess(const std::wstring& program) {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, program.c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
                if (!hProcess) continue;

                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
}

void System::startProcess(const std::wstring& program) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessW(program.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}



bool Packet::Contains(const std::string& packet, const std::string& key) {
    return packet.find(key) != std::string::npos;
}

template <typename T>
bool Packet::ContainsValue(const std::string& packet, const std::string& key) {
    size_t pos = packet.find(key + "|");
    if (pos == std::string::npos) return false;
    pos += key.size() + 1;
    size_t end = packet.find('\n', pos);
    std::string valueStr = packet.substr(pos, end - pos);
    if constexpr (std::is_same_v<T, std::string>) return !valueStr.empty();
    else if constexpr (std::is_integral_v<T>) {
        if (valueStr.empty()) return false;
        for (char c : valueStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }
    else return false;
}
template bool Packet::ContainsValue<std::string>(const std::string&, const std::string&);
template bool Packet::ContainsValue<int>(const std::string&, const std::string&);
template bool Packet::ContainsValue<unsigned int>(const std::string&, const std::string&);
template bool Packet::ContainsValue<long>(const std::string&, const std::string&);
template bool Packet::ContainsValue<unsigned long>(const std::string&, const std::string&);
template bool Packet::ContainsValue<long long>(const std::string&, const std::string&);
template bool Packet::ContainsValue<unsigned long long>(const std::string&, const std::string&);
template bool Packet::ContainsValue<short>(const std::string&, const std::string&);
template bool Packet::ContainsValue<unsigned short>(const std::string&, const std::string&);
template bool Packet::ContainsValue<char>(const std::string&, const std::string&);
template bool Packet::ContainsValue<unsigned char>(const std::string&, const std::string&);


template <typename T>
T Packet::ExtractValue(const std::string& packet, const std::string& key, T defaultValue) {
    size_t pos = packet.find(key + "|");
    if (pos == std::string::npos) return defaultValue;
    pos += key.size() + 1;
    size_t end = packet.find('\n', pos);
    std::string valueStr = packet.substr(pos, end - pos);
    if (valueStr.empty()) return defaultValue;
    if constexpr (std::is_same_v<T, std::string>) return valueStr;
    else if constexpr (std::is_integral_v<T>) {
        for (char c : valueStr) {
            if (!std::isdigit(static_cast<unsigned char>(c))) return defaultValue;
        }
        return static_cast<T>(std::stoll(valueStr));
    }
    else return defaultValue;
}
template std::string Packet::ExtractValue<std::string>(const std::string&, const std::string&, std::string);
template int Packet::ExtractValue<int>(const std::string&, const std::string&, int);
template unsigned int Packet::ExtractValue<unsigned int>(const std::string&, const std::string&, unsigned int);
template long Packet::ExtractValue<long>(const std::string&, const std::string&, long);
template unsigned long Packet::ExtractValue<unsigned long>(const std::string&, const std::string&, unsigned long);
template long long Packet::ExtractValue<long long>(const std::string&, const std::string&, long long);
template unsigned long long Packet::ExtractValue<unsigned long long>(const std::string&, const std::string&, unsigned long long);
template short Packet::ExtractValue<short>(const std::string&, const std::string&, short);
template unsigned short Packet::ExtractValue<unsigned short>(const std::string&, const std::string&, unsigned short);
template char Packet::ExtractValue<char>(const std::string&, const std::string&, char);
template unsigned char Packet::ExtractValue<unsigned char>(const std::string&, const std::string&, unsigned char);

template<typename T>
T Packet::ExtractCustom(const std::string& data, const std::string& starter, size_t startIndex, const std::string& delimiter) {
    size_t start = 0;
    size_t count = 0;

    while (count < startIndex) {
        start = data.find(starter, start);
        if (start == std::string::npos) throw std::runtime_error("Not enough '" + starter + "' in string");
        start += starter.length();
        count++;
    }

    size_t end = data.find(delimiter, start);
    if (end == std::string::npos) end = data.length();
    std::string result = data.substr(start, end - start);

    if constexpr (std::is_same_v<T, std::string>) return result;
    else if constexpr (std::is_integral_v<T>) {
        try { return static_cast<T>(std::stoll(result)); }
        catch (...) { return static_cast<T>(0); }
    }
    else static_assert(!sizeof(T*), "Unsupported type");
}
template std::string Packet::ExtractCustom<std::string>(const std::string&, const std::string&, size_t, const std::string&);
template int Packet::ExtractCustom<int>(const std::string&, const std::string&, size_t, const std::string&);
template unsigned int Packet::ExtractCustom<unsigned int>(const std::string&, const std::string&, size_t, const std::string&);
template long Packet::ExtractCustom<long>(const std::string&, const std::string&, size_t, const std::string&);
template unsigned long Packet::ExtractCustom<unsigned long>(const std::string&, const std::string&, size_t, const std::string&);
template long long Packet::ExtractCustom<long long>(const std::string&, const std::string&, size_t, const std::string&);
template unsigned long long Packet::ExtractCustom<unsigned long long>(const std::string&, const std::string&, size_t, const std::string&);
template short Packet::ExtractCustom<short>(const std::string&, const std::string&, size_t, const std::string&);
template unsigned short Packet::ExtractCustom<unsigned short>(const std::string&, const std::string&, size_t, const std::string&);
template char Packet::ExtractCustom<char>(const std::string&, const std::string&, size_t, const std::string&);
template unsigned char Packet::ExtractCustom<unsigned char>(const std::string&, const std::string&, size_t, const std::string&);


bool Packet::Change(std::string& data, const std::string& starter, size_t startIndex, const std::string& delimiter, const std::string& newValue) {
    size_t start = 0;
    size_t count = 0;

    while (count < startIndex) {
        start = data.find(starter, start);
        if (start == std::string::npos) return false;
        start += starter.length();
        count++;
    }

    size_t end = data.find(delimiter, start);
    if (end == std::string::npos) end = data.length();
    data.replace(start, end - start, newValue);

    return true;
}


/* --- SOCKS5 route: configuration and preflight ---------------------------

   Everything here runs before the logger is started and before anything on
   the machine has been touched, so it talks to the operator with plain
   cout. That is deliberate: the point of doing this first is to be able to
   stop with nothing changed. */

namespace {

    std::string TrimSpace(const std::string& s) {
        const auto b = s.find_first_not_of(" \t\r\n");
        if (b == std::string::npos) return "";
        return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
    }

    Socks5::Config RouteConfig() {
        Socks5::Config c;
        c.host = Route.HOST;
        c.port = Route.PORT;
        c.user = Route.USER;
        c.pass = Route.PASS;
        return c;
    }

    /* --- local HTTP CONNECT shim --------------------------------------

       cpp-httplib can be pointed at an HTTP proxy but not at a SOCKS5 one,
       and the server_data.php fetch goes through cpp-httplib. So we put the
       smallest possible HTTP proxy on loopback: it accepts one CONNECT,
       opens the matching SOCKS5 CONNECT, answers 200, and then moves bytes.

       This replaces shelling out to a general-purpose tunnelling binary,
       which is what an earlier version of this did. That approach had two
       problems that are not fixable from outside: the operator has to obtain
       and place a second executable, and the credentials have to be written
       to a config file on disk for it to read. It also outlives us -- if the
       proxy window is closed the child keeps running, holding the port, and
       the next start fails to bind for a reason that looks like nothing at
       all. In-process there is no child to leak. */

    SOCKET            g_shimFd  = INVALID_SOCKET;
    std::atomic<bool> g_shimRun{ false };
    std::thread       g_shimThread;

    /* Reads until the blank line that ends the request head. Bounded, because
       a peer that never sends one must not be able to grow this forever. */
    bool ReadRequestHead(SOCKET s, std::string& head) {
        char c = 0;
        while (head.size() < 8192) {
            const int n = ::recv(s, &c, 1, 0);
            if (n <= 0) return false;
            head.push_back(c);
            if (head.size() >= 4 && head.compare(head.size() - 4, 4, "\r\n\r\n") == 0)
                return true;
        }
        return false;
    }

    /* "CONNECT host:port HTTP/1.1" -> host, port. */
    bool ParseConnect(const std::string& head, std::string& host, uint16_t& port) {
        if (head.compare(0, 8, "CONNECT ") != 0) return false;
        const auto sp = head.find(' ', 8);
        if (sp == std::string::npos) return false;

        const std::string target = head.substr(8, sp - 8);
        const auto colon = target.rfind(':');
        if (colon == std::string::npos || colon == 0) return false;

        host = target.substr(0, colon);
        const int p = std::atoi(target.c_str() + colon + 1);
        if (p <= 0 || p > 65535) return false;
        port = static_cast<uint16_t>(p);
        return true;
    }

    bool SendText(SOCKET s, const char* text) {
        return Socks5::SendAll(s, text, static_cast<int>(std::strlen(text)));
    }

    void Pump(SOCKET a, SOCKET b) {
        std::vector<char> buf(16 * 1024);
        for (;;) {
            fd_set r;
            FD_ZERO(&r); FD_SET(a, &r); FD_SET(b, &r);
            timeval tv{ 1, 0 };
            const int n = ::select(static_cast<int>((a > b ? a : b)) + 1, &r, nullptr, nullptr, &tv);
            if (n < 0) return;
            if (n == 0) { if (!g_shimRun) return; continue; }

            if (FD_ISSET(a, &r)) {
                const int k = ::recv(a, buf.data(), static_cast<int>(buf.size()), 0);
                if (k <= 0 || !Socks5::SendAll(b, buf.data(), k)) return;
            }
            if (FD_ISSET(b, &r)) {
                const int k = ::recv(b, buf.data(), static_cast<int>(buf.size()), 0);
                if (k <= 0 || !Socks5::SendAll(a, buf.data(), k)) return;
            }
        }
    }

    void ServeConnect(SOCKET client) {
        std::string head;
        std::string host;
        uint16_t    port = 0;

        if (!ReadRequestHead(client, head) || !ParseConnect(head, host, port)) {
            SendText(client, "HTTP/1.1 400 Bad Request\r\n\r\n");
            closesocket(client);
            return;
        }

        Socks5::Status st = Socks5::Status::Protocol;
        int err = 0;
        const SOCKET up = Socks5::Connect(RouteConfig(), host, port, 10000, st, err);
        if (up == INVALID_SOCKET) {
            /* @important: answer, do not just drop. cpp-httplib waiting on a
               socket that never replies is a hang with no message; a 502 is a
               failed fetch it reports immediately. */
            LOG_ERROR("CONNECT {}:{} through the SOCKS5 server failed: {}", host, port, Socks5::Explain(st));
            SendText(client, "HTTP/1.1 502 Bad Gateway\r\n\r\n");
            closesocket(client);
            return;
        }

        SendText(client, "HTTP/1.1 200 Connection Established\r\n\r\n");
        Pump(client, up);

        closesocket(up);
        closesocket(client);
    }

    void ShimAcceptLoop() {
        while (g_shimRun) {
            /* @important: wait in select, not in accept. Closing a listening
               socket from another thread does not reliably wake a blocked
               accept -- so StopRoute would join a thread that never returns
               and the proxy would hang on exit, which is the same "still
               running, kill it by hand" problem this design exists to avoid.
               (Written the other way first; it hung.) */
            fd_set r;
            FD_ZERO(&r);
            FD_SET(g_shimFd, &r);
            timeval tv{ 0, 200000 };
            if (::select(static_cast<int>(g_shimFd) + 1, &r, nullptr, nullptr, &tv) <= 0)
                continue;

            const SOCKET c = ::accept(g_shimFd, nullptr, nullptr);
            if (c == INVALID_SOCKET) continue;

            std::thread(ServeConnect, c).detach();
        }
    }

}   /* namespace */

bool System::LoadRouteConfig(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cout << "\n"
                     "   The SOCKS5 route needs '" << path << "' next to this exe.\n"
                     "   Create it with:\n"
                     "\n"
                     "       host = 203.0.113.10\n"
                     "       port = 1080\n"
                     "       user = yourname\n"
                     "       pass = yourpassword\n"
                     "\n"
                     "   Keep it out of git: it holds a password in clear text.\n";
        return false;
    }

    std::string line;
    while (std::getline(f, line)) {
        line = TrimSpace(line);
        if (line.empty() || line[0] == '#') continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        const std::string k = TrimSpace(line.substr(0, eq));
        const std::string v = TrimSpace(line.substr(eq + 1));

        if      (k == "host")      Route.HOST = v;
        else if (k == "user")      Route.USER = v;
        else if (k == "pass")      Route.PASS = v;
        else if (k == "port")      Route.PORT      = static_cast<uint16_t>(std::atoi(v.c_str()));
        else if (k == "http_port") Route.HTTP_PORT = static_cast<uint16_t>(std::atoi(v.c_str()));
    }

    if (Route.HOST.empty() || Route.PORT == 0 || Route.USER.empty() || Route.PASS.empty()) {
        std::cout << "\n   '" << path << "' is missing one of: host, port, user, pass.\n";
        return false;
    }

    /* @important: never print Route.PASS -- not here, not in an error path,
       not at any verbosity. Console output ends up in screenshots. */
    std::cout << "\n   SOCKS5 " << Route.HOST << ":" << Route.PORT
              << " as '" << Route.USER << "'\n";
    return true;
}

bool System::RoutePreflight() {
    int err = 0;
    std::cout << "   Checking the server before anything is started...\n";

    const Socks5::Status st = Socks5::Probe(RouteConfig(), 5000, err);
    if (st == Socks5::Status::Ok) {
        std::cout << "   OK -- reachable, and the credentials were accepted.\n";
        return true;
    }

    std::cout << "\n   SOCKS5 preflight failed: " << Socks5::Explain(st) << "\n\n";

    /* Each of these needs a different fix, so each gets its own advice
       rather than one message that covers all of them and helps with none. */
    switch (st) {
    case Socks5::Status::Unreachable:
        std::cout << "   Nothing answered on " << Route.HOST << ":" << Route.PORT
                  << " (socket error " << err << ").\n"
                     "     1. Is the server up, and the SOCKS5 daemon running on it?\n"
                     "     2. If the port is restricted to a list of addresses and\n"
                     "        yours has changed, the server is fine and you are not\n"
                     "        on the list any more. This is the common one.\n"
                     "     3. host= and port= in socks5.cfg.\n";
        break;
    case Socks5::Status::NoAuthMethod:
        std::cout << "   It answered, but will not do username/password auth.\n"
                     "   The network path is fine -- this is server configuration.\n";
        break;
    case Socks5::Status::BadCredentials:
        std::cout << "   It rejected the credentials for user '" << Route.USER << "'.\n"
                     "   The network path is fine. Fix user= / pass= in socks5.cfg.\n";
        break;
    default:
        std::cout << "   Something is listening there, but it is not a SOCKS5\n"
                     "   server. Wrong port, most likely.\n";
        break;
    }
    return false;
}

bool System::StartConnectShim() {
    WSADATA wsa{};
    WSAStartup(MAKEWORD(2, 2), &wsa);   /* refcounted; enet has already called it */

    g_shimFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (g_shimFd == INVALID_SOCKET) {
        std::cout << "   Could not create the CONNECT shim socket (winsock "
                  << WSAGetLastError() << ").\n";
        return false;
    }

    sockaddr_in la{};
    la.sin_family = AF_INET;
    la.sin_port   = htons(Route.HTTP_PORT);
    ::inet_pton(AF_INET, Route.LOCAL_IP.c_str(), &la.sin_addr);

    if (::bind(g_shimFd, reinterpret_cast<sockaddr*>(&la), sizeof(la)) != 0 ||
        ::listen(g_shimFd, 8) != 0) {
        std::cout << "   Could not listen on " << Route.LOCAL_IP << ":" << Route.HTTP_PORT
                  << " (winsock " << WSAGetLastError() << ").\n"
                     "   Something else is on that port. Set a different one with\n"
                     "   http_port = <n> in socks5.cfg.\n";
        closesocket(g_shimFd);
        g_shimFd = INVALID_SOCKET;
        return false;
    }

    g_shimRun = true;
    g_shimThread = std::thread(ShimAcceptLoop);

    std::cout << "   CONNECT shim on " << Route.LOCAL_IP << ":" << Route.HTTP_PORT
              << " -> " << Route.HOST << ":" << Route.PORT << "\n";
    return true;
}

void System::StopRoute() {
    /* @important: this has to run on every exit path, not just the tidy one.
       See the console control handler in Main.cpp. */
    g_shimRun = false;
    if (g_shimThread.joinable()) g_shimThread.join();   /* it polls, so it returns */
    if (g_shimFd != INVALID_SOCKET) {
        closesocket(g_shimFd);
        g_shimFd = INVALID_SOCKET;
    }
}
