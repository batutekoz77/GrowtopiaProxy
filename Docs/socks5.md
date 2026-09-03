# Routing the proxy through a SOCKS5 server

Optional. Pick route `[1]` at startup and the proxy sends its own traffic
through a SOCKS5 server instead of straight out.

Nothing has to be installed for this. There is no second executable to
download, no tunnel to configure, and no child process left running when you
close the window.

---

## What is routed

| | Protocol | Routed |
|---|---|---|
| `server_data.php` fetch | TCP | **yes**, through an HTTP CONNECT shim on loopback |
| ENet game session | UDP | **yes**, in a SOCKS5 UDP association |
| Sub-server redirect after login | UDP | **yes**, it is a header rewrite |
| The login page | TCP | **yes**, forwarded without being decrypted |

### The login page

This one is different from the other two, so it is worth a paragraph.

The client opens the login page itself, at the `loginurl` address out of
`server_data.php`. No call inside this process makes that request, so there is
nothing here to point at the route. Left alone, the login token is issued to
*your* address while the game session arrives from the SOCKS5 server's — one
account showing up in two places, which is exactly the shape a proxy is
supposed to avoid.

Name resolution is the only lever available, and the proxy already owns the
hosts file. So the login host is pointed at `127.0.0.2`, where a relay accepts
the connection and forwards it through the same SOCKS5 server.

**It does not decrypt anything.** The relay holds no certificate and terminates
no TLS. Your client's handshake runs end to end with the real login server and
validates the real certificate — which is why nothing has to be installed, and
why this process could not read your password if it wanted to.

There is another way to do the same job: a local certificate authority, a
generated certificate for the login host, and TLS terminated in the middle.
It works, and it costs a private key living in the source tree plus a
machine-wide trust change on your PC — to read a password there is no reason to
see. This does not do that.

Turn it off with `login_route = 0` in `socks5.cfg`. Then the login goes out
from your own address while the game session does not, and you should know that
is what you chose.

**Private servers.** Many have no login page at all, and some put an address
rather than a name in `loginurl`. An address cannot be redirected by the hosts
file, so the proxy says so and stops rather than listening on a socket nothing
will ever reach; set `login_route = 0` for those.

---

## Setting it up

1. Have a SOCKS5 server that supports **username/password auth** and **UDP
   ASSOCIATE**. Both are required; see `socks5.cfg.example` for what that
   means on gost, 3proxy and Dante.

2. Copy `socks5.cfg.example` next to `Source.exe`, rename it to `socks5.cfg`,
   and fill in `host`, `port`, `user`, `pass`.

3. Start the proxy and choose `[1]`.

```
  ==========================================================
   How should this proxy reach Growtopia?

     [0]  Direct        straight out from this PC (default)
     [1]  SOCKS5        through the server in socks5.cfg
  ==========================================================

   Choice [0]: 1

   SOCKS5 203.0.113.10:1080 as 'yourname'
   Checking the server before anything is started...
   OK -- reachable, and the credentials were accepted.
   CONNECT shim on 127.0.0.1:18080 -> 203.0.113.10:1080
```

The check happens **before** Growtopia.exe is closed and before the hosts file
is touched. If it fails, nothing on the machine has been changed and you can
just fix the config and start again.

---

## When it will not start

The proxy names which of the four it is, because they need four different
fixes.

**"the server did not answer"** — nothing is listening. In order: is the
server up and the daemon running; **has your own address changed?** If the
port is restricted to a list of addresses and yours has moved, this looks
exactly like an outage and is not one; then check `host` and `port`.

**"the server refuses username/password auth"** — it answered, so the network
path is fine. Server-side configuration.

**"the server rejected the credentials"** — network path fine. Fix `user` /
`pass`, or the account on the server.

**"the reply was not valid SOCKS5"** — something is listening on that port,
but it is not a SOCKS5 server. Usually the wrong port.

And one that appears later, after the server list has been fetched:

**"ASSOCIATE failed — the server refused the request"** — the server works but
will not relay UDP. Enable UDP on it; the game session cannot go without it.

**"loginurl is an address, not a name"** — the hosts file redirects names, so
an address cannot be taken over. Common on private servers. Set
`login_route = 0`.

**"Login relay: could not listen on 127.0.0.2:443"** — something else holds
that address. Pick another loopback address with `login_ip = 127.0.0.3`.

---

## Reading the log

Every request the proxy's own HTTP server completes is logged, including the
ones no handler matched:

```
[HTTPD] www.growtopia2.com POST /growtopia/server_data.php -> 200
```

That line is worth more than it looks. Without it, "the request never reached
us" and "it reached us and we answered 404" look identical from the log — and
they are unrelated problems. A `404` there means the game asked this proxy for
something it does not serve; no line at all means the request never arrived,
which is a hosts-file, DNS-cache or TLS problem instead.

A working SOCKS5 start looks like this:

```
[HTTP] Request: /growtopia/server_data.php
[HTTP] server_data.php fetch routed through 203.0.113.10:1080
[HTTP] Login page login.example.com -> 127.0.0.2:443 -> (socks5) -> login.example.com:443
[HTTP] SOCKS5 UDP association up via 203.0.113.10:45863
[HTTP] ENet -> 127.0.0.1:17045 -> (socks5) -> 198.51.100.7:17045
[HTTP] Fetched the api www.growtopia2.com
[HTTPD] www.growtopia2.com POST /growtopia/server_data.php -> 200
```

If the login page then fails with a 404 **from this proxy**, the `[HTTPD]`
line names the host and path it asked for, which says immediately whether the
login host is resolving here when it should not be. A stale hosts entry or a
cached DNS answer from an earlier run does exactly that; `ipconfig /flushdns`
clears the second one.

---

## Things worth knowing

**Your session arrives from the proxy's address.** Whatever reputation that
address has is now attached to your play. An address shared with other people,
or one that has been used for bulk automated connections, is not a neutral
place to appear from. This is not theoretical: a server address that had been
used for repeated automated connection attempts stopped being answered by the
game, while an ordinary home address was still fine.

**It fails closed, everywhere.** If the route cannot be established the proxy
stops. It never continues directly while you believe you are routed, because
acting on that belief is worse than not starting.

**It adds no traffic of its own.** The relay is one datagram in, one datagram
out, with a 10-byte header added and stripped. It does not change how much you
send or how often — so it neither helps nor hurts on that front, and it is not
a substitute for the delays in anything that automates play.

**Your password stays in one file.** It is never compiled in, never logged,
and never printed — not even in an error message. Keep `socks5.cfg` out of
git; the repository's `.gitignore` already excludes it.

---

## Three details that bite, if you are reading the code

**The local UDP socket binds the game server's own port number.** That looks
arbitrary. It is not: Growtopia's ENet fork encodes the destination port into
the packet header, and the receiver compares it against its own port. A
mismatch is dropped silently, on both sides. A relay listening on a port of its
own choosing gives you a connection that never completes and no error anywhere.

**The source port facing the relay never changes.** An ENet peer is pinned to
the address its CONNECT arrived from and ignores packets from anywhere else. A
general-purpose UDP forwarder that opens a fresh association per burst gets a
new source port each time, and the game server stops answering part-way in.
Holding one association for the whole session is what avoids that.

**The hosts entry for the login host is written by the relay, not by the
config.** `editHosts` asks the relay which host it is listening for, and gets
an empty string when it is not running. So the file can never name a host
nothing is answering for — which would fail worse than not redirecting at all,
because the login page would then not load and nothing would say why — and the
entry clears itself on the way out.
