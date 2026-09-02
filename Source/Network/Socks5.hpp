#pragma once

/* Minimal SOCKS5 client: RFC 1928, with RFC 1929 username/password auth.
 *
 * Protocol only. No logging, no globals, no threads -- everything that needs
 * those lives in System:: (Functions.cpp). Kept separate for one concrete
 * reason: this file can then be compiled and driven against a stub server by
 * an offline test, which is the only way any of it gets checked without a
 * live SOCKS5 server to hand.
 *
 * Header-only on purpose: adding a .cpp means editing Source.vcxproj, and an
 * edit there that goes wrong stops the project from loading at all.
 */

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
/* @important: winsock2.h before windows.h, always. If windows.h wins the race
   it pulls in winsock 1.1 and every symbol below collides. */
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
/* The proxy is Windows-only; this branch is not part of the shipped program.
   It exists so the protocol code below can be built and exercised by the
   offline test. */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
using SOCKET = int;
inline constexpr SOCKET INVALID_SOCKET = -1;
inline constexpr int    SOCKET_ERROR   = -1;
inline int closesocket(SOCKET s) { return ::close(s); }
inline int WSAGetLastError() { return errno; }
#endif

#include <cstdint>
#include <cstring>
#include <string>

namespace Socks5 {

    /* Distinct outcomes on purpose. "It didn't work" is not an actionable
       message: an unreachable server, a server that refuses username/password
       auth, and a server that rejects the password each need a different fix,
       and the operator should not have to guess which one they are looking at. */
    enum class Status {
        Ok,
        Unreachable,     /* no TCP at all: host down, firewall, wrong host:port */
        NoAuthMethod,    /* answered, but will not do username/password         */
        BadCredentials,  /* username/password rejected                          */
        Refused,         /* handshake fine, but CONNECT/ASSOCIATE was refused   */
        Protocol         /* not SOCKS5, or a truncated reply                    */
    };

    struct Config {
        std::string host;
        uint16_t    port = 0;
        std::string user;
        std::string pass;
    };

    /* An open UDP association. It lives exactly as long as `control` does:
       when that TCP connection closes the relay stops forwarding, and it does
       so silently -- sends keep succeeding and every datagram disappears. So
       the control socket must be watched, not merely held. */
    struct Association {
        SOCKET      control = INVALID_SOCKET;
        sockaddr_in relay{};
    };

    inline const char* Explain(Status s) {
        switch (s) {
        case Status::Ok:             return "ok";
        case Status::Unreachable:    return "the server did not answer";
        case Status::NoAuthMethod:   return "the server refuses username/password auth";
        case Status::BadCredentials: return "the server rejected the credentials";
        case Status::Refused:        return "the server refused the request";
        case Status::Protocol:       return "the reply was not valid SOCKS5";
        }
        return "unknown";
    }

    inline bool SendAll(SOCKET s, const void* data, int len) {
        const char* p = static_cast<const char*>(data);
        int sent = 0;
        while (sent < len) {
            const int n = ::send(s, p + sent, len - sent, 0);
            if (n <= 0) return false;
            sent += n;
        }
        return true;
    }

    inline bool RecvExact(SOCKET s, void* data, int len) {
        char* p = static_cast<char*>(data);
        int got = 0;
        while (got < len) {
            const int n = ::recv(s, p + got, len - got, 0);
            if (n <= 0) return false;   /* closed, or the receive timeout fired */
            got += n;
        }
        return true;
    }

    /* Greeting + auth. Offers exactly one method (username/password) rather
       than also offering "no auth": a server that would have taken us
       anonymously is not what the operator asked for, and silently using it
       would route traffic under an identity they did not choose. */
    inline Status Greet(SOCKET s, const std::string& user, const std::string& pass) {
        if (user.size() > 255 || pass.size() > 255) return Status::Protocol;

        const unsigned char greet[3] = { 0x05, 0x01, 0x02 };
        if (!SendAll(s, greet, 3)) return Status::Protocol;

        unsigned char sel[2] = {};
        if (!RecvExact(s, sel, 2))  return Status::Protocol;
        if (sel[0] != 0x05)         return Status::Protocol;
        if (sel[1] == 0xFF)         return Status::NoAuthMethod;
        if (sel[1] != 0x02)         return Status::NoAuthMethod;

        std::string req;
        req.push_back(0x01);                                    /* auth subnegotiation version */
        req.push_back(static_cast<char>(user.size()));
        req += user;
        req.push_back(static_cast<char>(pass.size()));
        req += pass;
        if (!SendAll(s, req.data(), static_cast<int>(req.size()))) return Status::Protocol;

        unsigned char ar[2] = {};
        if (!RecvExact(s, ar, 2)) return Status::Protocol;
        if (ar[0] != 0x01)        return Status::Protocol;
        if (ar[1] != 0x00)        return Status::BadCredentials;

        return Status::Ok;
    }

    /* TCP connect with a real timeout. The OS default is around 20 seconds on
       a black-holed port, which at startup reads as a hang. */
    inline SOCKET Dial(const std::string& host, uint16_t port, int timeoutMs, int& err) {
        err = 0;

        addrinfo hints{};
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* ai = nullptr;
        const std::string svc = std::to_string(port);
        if (::getaddrinfo(host.c_str(), svc.c_str(), &hints, &ai) != 0 || !ai) {
            err = WSAGetLastError();
            return INVALID_SOCKET;
        }

        SOCKET s = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s == INVALID_SOCKET) {
            err = WSAGetLastError();
            ::freeaddrinfo(ai);
            return INVALID_SOCKET;
        }

#ifdef _WIN32
        u_long nb = 1;
        ::ioctlsocket(s, FIONBIO, &nb);
#else
        const int fl = ::fcntl(s, F_GETFL, 0);
        ::fcntl(s, F_SETFL, fl | O_NONBLOCK);
#endif
        const int rc = ::connect(s, ai->ai_addr, static_cast<int>(ai->ai_addrlen));
        ::freeaddrinfo(ai);

        if (rc == SOCKET_ERROR) {
#ifdef _WIN32
            const bool pending = (WSAGetLastError() == WSAEWOULDBLOCK);
#else
            const bool pending = (errno == EINPROGRESS);
#endif
            if (!pending) {
                err = WSAGetLastError();
                closesocket(s);
                return INVALID_SOCKET;
            }
            fd_set wr, ex;
            FD_ZERO(&wr); FD_SET(s, &wr);
            FD_ZERO(&ex); FD_SET(s, &ex);
            timeval tv{ timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
            const int n = ::select(static_cast<int>(s) + 1, nullptr, &wr, &ex, &tv);
            if (n <= 0 || FD_ISSET(s, &ex)) {
                err = (n == 0) ? 0 : WSAGetLastError();
                closesocket(s);
                return INVALID_SOCKET;
            }
            int so = 0;
#ifdef _WIN32
            int solen = sizeof(so);          /* winsock spells this `int` */
#else
            socklen_t solen = sizeof(so);
#endif
            if (::getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so), &solen) == 0 && so != 0) {
                err = so;
                closesocket(s);
                return INVALID_SOCKET;
            }
        }

#ifdef _WIN32
        nb = 0;
        ::ioctlsocket(s, FIONBIO, &nb);
        DWORD to = static_cast<DWORD>(timeoutMs);
#else
        ::fcntl(s, F_SETFL, fl);
        timeval to{ timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
#endif
        ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&to), sizeof(to));
        ::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&to), sizeof(to));
        return s;
    }

    /* Dial and authenticate. On failure returns INVALID_SOCKET with `st` set. */
    inline SOCKET Open(const Config& cfg, int timeoutMs, Status& st, int& err) {
        SOCKET s = Dial(cfg.host, cfg.port, timeoutMs, err);
        if (s == INVALID_SOCKET) { st = Status::Unreachable; return INVALID_SOCKET; }

        st = Greet(s, cfg.user, cfg.pass);
        if (st != Status::Ok) { closesocket(s); return INVALID_SOCKET; }
        return s;
    }

    /* Answers "is the server there, and will it have us?" and nothing else. */
    inline Status Probe(const Config& cfg, int timeoutMs, int& err) {
        Status st = Status::Protocol;
        SOCKET s = Open(cfg, timeoutMs, st, err);
        if (s != INVALID_SOCKET) closesocket(s);
        return st;
    }

    /* Reads a reply after the 2-byte VER/REP prefix has been consumed, i.e.
       RSV, ATYP, the address and the port. `bnd` may be null. */
    inline bool ReadBoundAddress(SOCKET s, sockaddr_in* bnd) {
        unsigned char rsv_atyp[2] = {};
        if (!RecvExact(s, rsv_atyp, 2)) return false;

        unsigned char addr[256] = {};
        int addrLen = 0;
        switch (rsv_atyp[1]) {
        case 0x01: addrLen = 4;  break;                       /* IPv4   */
        case 0x04: addrLen = 16; break;                       /* IPv6   */
        case 0x03: {                                          /* domain */
            unsigned char n = 0;
            if (!RecvExact(s, &n, 1)) return false;
            addrLen = n;
            break;
        }
        default: return false;
        }
        if (addrLen > static_cast<int>(sizeof(addr))) return false;
        if (!RecvExact(s, addr, addrLen)) return false;

        unsigned char port[2] = {};
        if (!RecvExact(s, port, 2)) return false;

        if (bnd) {
            std::memset(bnd, 0, sizeof(*bnd));
            bnd->sin_family = AF_INET;
            if (rsv_atyp[1] == 0x01) std::memcpy(&bnd->sin_addr, addr, 4);
            std::memcpy(&bnd->sin_port, port, 2);
        }
        return true;
    }

    /* CONNECT to dstHost:dstPort through the server. The destination goes out
       as a domain name when it is not a literal IPv4 address, so the name is
       resolved at the far end -- resolving it here would put a DNS lookup for
       the destination on the local link, which is exactly what routing through
       a proxy is meant to avoid. */
    inline SOCKET Connect(const Config& cfg, const std::string& dstHost, uint16_t dstPort,
                          int timeoutMs, Status& st, int& err) {
        SOCKET s = Open(cfg, timeoutMs, st, err);
        if (s == INVALID_SOCKET) return INVALID_SOCKET;

        std::string req;
        req.push_back(0x05);
        req.push_back(0x01);        /* CONNECT */
        req.push_back(0x00);

        in_addr lit{};
        if (dstHost.size() <= 255 && ::inet_pton(AF_INET, dstHost.c_str(), &lit) == 1) {
            req.push_back(0x01);
            req.append(reinterpret_cast<const char*>(&lit), 4);
        }
        else if (dstHost.size() <= 255) {
            req.push_back(0x03);
            req.push_back(static_cast<char>(dstHost.size()));
            req += dstHost;
        }
        else {
            closesocket(s);
            st = Status::Protocol;
            return INVALID_SOCKET;
        }
        const uint16_t p = htons(dstPort);
        req.append(reinterpret_cast<const char*>(&p), 2);

        if (!SendAll(s, req.data(), static_cast<int>(req.size()))) {
            closesocket(s); st = Status::Protocol; return INVALID_SOCKET;
        }

        unsigned char head[2] = {};
        if (!RecvExact(s, head, 2) || head[0] != 0x05) {
            closesocket(s); st = Status::Protocol; return INVALID_SOCKET;
        }
        if (head[1] != 0x00) {
            closesocket(s); st = Status::Refused; err = head[1]; return INVALID_SOCKET;
        }
        if (!ReadBoundAddress(s, nullptr)) {
            closesocket(s); st = Status::Protocol; return INVALID_SOCKET;
        }

        st = Status::Ok;
        return s;
    }

    /* UDP ASSOCIATE. DST is sent as 0.0.0.0:0, meaning "I will send from
       whatever address I end up with" -- the only honest answer from behind
       NAT, and what every SOCKS5 server expects from a client that cannot know
       its own mapped address. */
    inline Status Associate(const Config& cfg, int timeoutMs, Association& out, int& err) {
        Status st = Status::Protocol;
        SOCKET s = Open(cfg, timeoutMs, st, err);
        if (s == INVALID_SOCKET) return st;

        const unsigned char req[10] = { 5, 3, 0, 1, 0, 0, 0, 0, 0, 0 };
        if (!SendAll(s, req, 10)) { closesocket(s); return Status::Protocol; }

        unsigned char head[2] = {};
        if (!RecvExact(s, head, 2) || head[0] != 0x05) { closesocket(s); return Status::Protocol; }
        if (head[1] != 0x00) { err = head[1]; closesocket(s); return Status::Refused; }

        sockaddr_in bnd{};
        if (!ReadBoundAddress(s, &bnd)) { closesocket(s); return Status::Protocol; }

        /* A server is allowed to answer 0.0.0.0 to mean "the host you are
           already talking to". Taking that literally would send every datagram
           to the wildcard address and lose the lot. */
        if (bnd.sin_addr.s_addr == 0)
            ::inet_pton(AF_INET, cfg.host.c_str(), &bnd.sin_addr);

        out.control = s;
        out.relay   = bnd;
        return Status::Ok;
    }

    /* The 10-byte header that prefixes every relayed datagram (RFC 1928 s7):
       RSV(2) FRAG(1) ATYP(1) ADDR(4) PORT(2). We never fragment, so FRAG is
       always 0. Rewriting it is the whole cost of changing destination -- see
       the sub-server redirect in PACKET_CALL_FUNCTION.hpp. */
    inline void MakeUdpHeader(uint8_t hdr[10], const std::string& ip, uint16_t port) {
        hdr[0] = 0; hdr[1] = 0;
        hdr[2] = 0;
        hdr[3] = 1;
        ::inet_pton(AF_INET, ip.c_str(), &hdr[4]);
        const uint16_t p = htons(port);
        std::memcpy(&hdr[8], &p, sizeof(p));
    }

}   /* namespace Socks5 */
