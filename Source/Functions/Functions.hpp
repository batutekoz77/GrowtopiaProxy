#pragma once

#include <string>

namespace System {
	bool RelaunchAsAdmin();
	bool IsTrueAdmin();
	bool EnableDebugPrivilege();
	bool editHosts(std::string ipAddress);
	int findProcess(const std::wstring& program);
	void endProcess(const std::wstring& program);
	void startProcess(const std::wstring& program);

	/* --- optional SOCKS5 route ---------------------------------------------
	   Reads socks5.cfg (host/port/user/pass) into the Route config, and proves
	   the server is both reachable and willing to have us -- before anything
	   on this machine has been changed, so that failing costs nothing.

	   Both print with cout rather than the logger: they run before the logger
	   has been started. */
	bool LoadRouteConfig(const std::string& path);
	bool RoutePreflight();

	/* An HTTP CONNECT proxy on loopback that chains to the SOCKS5 server.
	   It exists because cpp-httplib speaks HTTP proxies but not SOCKS5, and
	   the server_data.php fetch goes through cpp-httplib. */
	bool StartConnectShim();

	/* Tears down everything the route owns. Safe to call more than once, and
	   safe to call when no route was ever started. */
	void StopRoute();
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