#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "HTTP/Http_Extension.h"
#include <fstream>
#include <filesystem>
#include <mutex>
#include <atomic>

class HttpManager {
public:
    bool Fetcher();
    void Injector();

public:
    bool fetching = false;
    std::atomic<bool> listening{ false };
    std::string server_data_cache;
    std::mutex cache_mutex;
    std::string official_ip;

private:
    static auto getValue(const std::string& src, const std::string& key) -> std::string {
        std::string search = key + "|";
        size_t pos = src.find(search);
        if (pos == std::string::npos) return "";

        pos += search.length();
        size_t end = src.find("\n", pos);
        if (end == std::string::npos) end = src.length();

        return src.substr(pos, end - pos);
    }

    void ensure_cert_files_exist() {
        const char* cert_file = "growtopia.pem";
        const char* key_file = "growtopia.key.pem";

        if (!std::filesystem::exists(cert_file)) {
            std::ofstream(cert_file) << R"(-----BEGIN CERTIFICATE-----
MIIDxTCCAq2gAwIBAgIUGDpZSBJ+ZIZDYsgUafbHkJyFRtwwDQYJKoZIhvcNAQEL
BQAwWDELMAkGA1UEBhMCVVMxCzAJBgNVBAgMAkNBMQswCQYDVQQHDAJMQTESMBAG
A1UECgwJR3Jvd3RvcGlhMRswGQYDVQQDDBJ3d3cuZ3Jvd3RvcGlhMS5jb20wHhcN
MjYxMDIyMTQwODA0WhcNMzYxMDE5MTQwODA0WjBYMQswCQYDVQQGEwJVUzELMAkG
A1UECAwCQ0ExCzAJBgNVBAcMAkxBMRIwEAYDVQQKDAlHcm93dG9waWExGzAZBgNV
BAMMEnd3dy5ncm93dG9waWExLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC
AQoCggEBAN7bQzWj4wVfPcgiwtQzvflwQGpMvNOirVhr8giwFP+m1r1zFFG8bLTW
7NSv9Td4f5wkPqB4MSNI69SJYrPBefBM9HurZ3NAZr6JFQ74rbDZ5sxpM3+47Hui
nJTVFKHV+lEphpO9vsn+ivxa75252wTk7SAjg08igDf7RH6V2Dnxv458E8ReQesH
6uqyYrOum8jWUH2lui40Fn0PsAbGqWF50Fzi82imydACvi9yoHbNffdl0oA+QG1U
uCU3r5TW7+NjC9ouL8/VTDRBlcX4YZHGfMfd0+a1EqFRHRNxvsf2qsPqRq7b7TiA
IexgmUkW7OCC3cirqKSN4HUjUekcEXsCAwEAAaOBhjCBgzAdBgNVHQ4EFgQUaQoh
+du4EcIIXv/lCub2YBH9K94wDwYDVR0TAQH/BAUwAwEB/zBRBgNVHREESjBIgg5n
cm93dG9waWExLmNvbYISd3d3Lmdyb3d0b3BpYTEuY29tgg5ncm93dG9waWEyLmNv
bYISd3d3Lmdyb3d0b3BpYTIuY29tMA0GCSqGSIb3DQEBCwUAA4IBAQCAi30CwEva
uPcBhM84P9E2Tz/uHlMGVsva5JqPypbR4g87hPzubzPEOAl4f7bhz3ru3Bs6lxRB
pSQh48Ov3gpm/7ShjaMyXLI4a5zurUbQuyVWgaNuBH5IB9f4mwH9DPkknr5q9moS
M0KRx5QtW5/Sy5w7MufTcbSZwoDXQ208u4fkZjdlNztFsrVlf7VkmZcJgRED/vq/
FkG04r1E5q1H2IpMSvTzqn+DPAipgZtCfbBIeBgWZ9lFV85MYUQYbrWXLgXrbON7
xs6PFVxgldOtrRY9w5PJjB0p87LM52Mm+8T1S5bhl4zssCIfz2qw87JCCCEJqzvc
DNBbm0C0zaWn
-----END CERTIFICATE-----)";
        }

        if (!std::filesystem::exists(key_file)) {
            std::ofstream(key_file) << R"(-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDe20M1o+MFXz3I
IsLUM735cEBqTLzToq1Ya/IIsBT/pta9cxRRvGy01uzUr/U3eH+cJD6geDEjSOvU
iWKzwXnwTPR7q2dzQGa+iRUO+K2w2ebMaTN/uOx7opyU1RSh1fpRKYaTvb7J/or8
Wu+dudsE5O0gI4NPIoA3+0R+ldg58b+OfBPEXkHrB+rqsmKzrpvI1lB9pbouNBZ9
D7AGxqlhedBc4vNopsnQAr4vcqB2zX33ZdKAPkBtVLglN6+U1u/jYwvaLi/P1Uw0
QZXF+GGRxnzH3dPmtRKhUR0Tcb7H9qrD6kau2+04gCHsYJlJFuzggt3Iq6ikjeB1
I1HpHBF7AgMBAAECggEAI4JCSj4Lwo5eChE9LpH0AiutnS0O+pW1aHJYBU8GeYRR
kQF7dQPrOkFInnb5loC/rIG7QixZ1VUde9GhhXdy1iQc4cZPhyCgKQSEniG84yya
cWrjHBsQqykffo4ZFH7uEe07lozUOoaGtMD0jxLPCrpDQiyxE8nb2OT+EUFk9jj6
Sjob1/oUy7FWgylN3tHscFT/ItLK8Dh3eg7FVvv60rWzIsGrow1vOo8Ki0cxkUoU
1yYrvMOj+HrLD846AlToU7uputPNNGkAzqZGIY03NjqXKBwx5it7HAgIxBN0Fopw
+KpOiptXj1WRjv3f/kzVUeRpwEoJosHKvlAOhKW9qQKBgQD2dUDfeuQcmg1qp/PS
3FuT2ygCMU9tIRKaxrltsqTIYiknLeqQlifz9gmGovnCbwwi1xv/H+sdI0jw+eAa
HKo1se9AbZVaOqNIF6dU5ktaUbzjvE6CIplz8eOXA61L2FTp1D+4qeqBm/ihplKk
5WhuqMXQuwx+DUorAxT3dqdWRwKBgQDnfBWve+QmCn4vgq1oiiE5rCgH2EQEqAsG
tgMd8gFC8/GA2BmUI1ib0KalXNH8OtaK3Eo+x9pfUzI0huBUBgxqfEYvLJ9vtx6v
LHhnGyJRFIkLgfQP2dw5TxP5IpTxIEpeFjKrHxZzfz8Re2wpBzq5LzjGCfEAUqa0
8cmGnXdhLQKBgQCgjgYzEFXmpB01bDCVH2SxT8Yil4kMqMz785X89rIOsJPqqCj1
BD0G2zOdeDKJDdq0H7pnW53JH6F3CvGJUT85N6+LI1h2rViNsIsb7eMM9c699YCX
mn6pX91KFeUhUc8PfWv9mwPgZ4FznWyda6262Uv1DMW4oGO+3YUdKTkeJQKBgQDR
US3WwaObFm/mDDc7yXOjz91ewps2zrExv1HwOQ4U+bsOc9olTxTnyHN4IgerGZU4
eVwViugWvIcwYinlMJb3jypzq9j7zs1LP14cdbfBM11872S/FJQDqlL4O+IMa9Nz
YoUsoW7Ov2hLKZyzONtL6rxer0yiqxodkmlVdXwmfQKBgBDeFLP7P+QsgAZBiTdz
E5SfrBu/QllpW3gRYhD8CzSd6j9RduJwRrMGJuVfl12u1eqHk+Q3Enk15Skv3h0X
ZDPSJeUxpslcndoxnNQUKn1p2XpXb9gpNnAkTC9OKlczaLclXx4VZXuZnW0Wx/lE
HJSSJQ5QVdHa0pqOWPuyT3Qd
-----END PRIVATE KEY-----)";
        }
    }

};
extern HttpManager Http;