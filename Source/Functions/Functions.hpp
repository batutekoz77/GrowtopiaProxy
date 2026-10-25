#pragma once

#include <string>

namespace System {
	bool RelaunchAsAdmin();
	bool IsTrueAdmin();
	bool EnableDebugPrivilege();
	bool editHosts(std::string ipAddress);
	void flushDns();
	std::string resolveHostA(const std::string& hostname);
	int findProcess(const std::wstring& program);
	void endProcess(const std::wstring& program);
	void startProcess(const std::wstring& program);
}

namespace Packet {
	bool Contains(const std::string& packet, const std::string& key);

	template <typename T>
	bool ContainsValue(const std::string& packet, const std::string& key);

    template <typename T>
	T ExtractValue(const std::string& packet, const std::string& key, T defaultValue);

	template<typename T>
	T ExtractCustom(const std::string& data, const std::string& starter, size_t startIndex, const std::string& delimiter);

	bool Change(std::string& data, const std::string& starter, size_t startIndex, const std::string& delimiter, const std::string& newValue);
}