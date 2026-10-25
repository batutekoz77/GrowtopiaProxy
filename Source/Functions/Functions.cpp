#pragma once
#include "Functions.hpp"
#include "../Logger/Logger.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <windns.h>
#pragma comment(lib, "Dnsapi.lib")
#endif

#include <string>
#include <fstream>
#include <cstdio>


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
        hostsContent += ipAddress + " growtopia1.com\n";
        hostsContent += ipAddress + " growtopia2.com\n";
    }

    std::ofstream hostsFile(hostsPath, std::ios::trunc);
    if (!hostsFile.is_open()) {
        LOG_DEBUG("Couldn't edit the  hosts file");
        return false;
    }

    hostsFile << hostsContent;
    hostsFile.close();

    flushDns();

    FastLog::Logger::set_thread_name("FUNCTIONS");
    LOG_DEBUG("Edited hosts file [{}]", ipAddress);

    return true;
}

void System::flushDns() {
    using DnsFlushFn = BOOL(WINAPI*)();
    HMODULE dnsapi = GetModuleHandleW(L"dnsapi.dll");
    if (!dnsapi) dnsapi = LoadLibraryW(L"dnsapi.dll");
    if (!dnsapi) return;
    auto fn = reinterpret_cast<DnsFlushFn>(GetProcAddress(dnsapi, "DnsFlushResolverCache"));
    if (fn) fn();
}

std::string System::resolveHostA(const std::string& hostname) {
    PDNS_RECORD records = nullptr;
    const DWORD flags = DNS_QUERY_BYPASS_CACHE | DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_WIRE_ONLY;
    DNS_STATUS status = DnsQuery_A(hostname.c_str(), DNS_TYPE_A, flags, nullptr, &records, nullptr);
    if (status != 0 || !records) {
        LOG_ERROR("DNS query failed for {} (status {})", hostname, static_cast<int>(status));
        return {};
    }

    std::string ip;
    for (PDNS_RECORD p = records; p; p = p->pNext) {
        if (p->wType != DNS_TYPE_A) continue;
        const unsigned long addr = p->Data.A.IpAddress;
        char buf[16]{};
        std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
            static_cast<unsigned>(addr & 0xFFu),
            static_cast<unsigned>((addr >> 8) & 0xFFu),
            static_cast<unsigned>((addr >> 16) & 0xFFu),
            static_cast<unsigned>((addr >> 24) & 0xFFu));
        ip = buf;
        break;
    }

    DnsRecordListFree(records, DnsFreeRecordList);
    return ip;
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