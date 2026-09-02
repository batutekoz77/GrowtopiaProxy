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
| The login page | TCP | **no** — see below |

The game client opens the login page itself, to the `loginurl` address. That
request never reaches this process, and the hosts file does not redirect it,
so it goes out from your own connection.

**This means the login and the game session arrive from two different
addresses.** That is a real difference from playing normally and it is visible
from the other side. Nothing in this feature changes it. Decide whether you
want that before turning it on.

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

## Two details that bite, if you are reading the code

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
