# AdGuard Ad Blocking on Routers — Technical Research

> **Purpose:** This document informs the ESP32-S3 firmware's AdGuard testing logic.  
> It covers how  router ad blocking works (built-in and AdGuard Home), what DNS
> responses are returned for blocked domains, a step-by-step testing protocol,
> Python code examples, and configuration notes for popular  models.

---

## Table of Contents

1. [Router Built-in Ad Blocking (AiProtection / Trend Micro)](#1-asus-router-built-in-ad-blocking-aiprotection--trend-micro)
2. [AdGuard Home on Routers](#2-adguard-home-on-asus-routers)
3. [DNS Responses for Blocked Domains](#3-dns-responses-for-blocked-domains)
4. [Testing Protocol Design](#4-testing-protocol-design)
5. [Common Ad / Tracking Domains](#5-common-ad--tracking-domains)
6. [Router Configuration Notes](#6-router-configuration-notes)
7. [ESP32-S3 Firmware Integration Notes](#7-esp32-s3-firmware-integration-notes)

---

## 1. Router Built-in Ad Blocking (AiProtection / Trend Micro)

### Overview

's **AiProtection** is a network-level security suite powered by **Trend Micro**.
It is available on most mid-range and high-end  routers running WRT firmware
(not available in Access Point mode, and disabled when WAN aggregation / load-balance
dual-WAN is active).

AiProtection is split into two main sections:

- **Network Protection** — Malicious Sites Blocking, Two-Way IPS, Infected Device
  Prevention, Advertisement Blocking, Tracker Blocking.
- **Parental Controls** — (not covered here).

### Advertisement Blocking

The **Advertisement Blocking** feature (added to newer firmware versions) blocks
common advertising sources at the DNS level. When enabled, DNS queries for known
ad-serving domains are intercepted and the router returns a blocked response instead
of the real IP address.

Key characteristics:

- **Powered by Trend Micro cloud databases** — the router checks queried domains
  against Trend Micro's threat intelligence. This means the router contacts Trend
  Micro servers (e.g., `ntd-asus-2014b-en.fbs20.trendmicro.com`) to verify domains.
- **DNS-level blocking** — the block happens at DNS resolution time, before any
  connection is established.
- **Cloud-dependent** — blocking lists are not stored locally; they are queried
  from Trend Micro's servers in real-time or cached for a period.
- **Privacy trade-off** — enabling AiProtection requires agreeing to share DNS
  query information with Trend Micro.

### Tracker Blocking

Added alongside Advertisement Blocking in recent firmware, **Tracker Blocking**
detects and blocks tracking requests from websites, advertisers, and third-party
analytics platforms. It works the same way as Advertisement Blocking — DNS-level
interception — but uses a different Trend Micro category list.

### DNS Response Behavior for AiProtection Blocks

 AiProtection (via Trend Micro) does **not** return a standard sinkhole IP like
`0.0.0.0`. Based on community analysis and testing, the behavior is:

| Response Type | Behavior |
|---|---|
| **HTTP/HTTPS traffic to blocked domain** | The router intercepts the connection and returns a block page (HTTP) or resets the connection (HTTPS). |
| **DNS query** | The router may still resolve the domain to its real IP, but then block the subsequent TCP connection at the firewall/IPS layer. Alternatively, in some firmware versions, it returns NXDOMAIN or a null route. |
| **Block page** | For HTTP, a Trend Micro block page is served. For HTTPS, the connection is refused/reset. |

> **Important for ESP32-S3 testing:** AiProtection's blocking may be **connection-level**
> (IPS/firewall) rather than purely DNS-level. This means a DNS query to the router
> may return the real IP address, but the actual HTTP connection to that IP will be
> blocked. This is a critical distinction from AdGuard Home, which blocks purely at
> the DNS level.

### How to Verify AiProtection is Active

1. Log into the  web GUI (`http://www.asusrouter.com` or the router's LAN IP).
2. Navigate to **AiProtection → Network Protection**.
3. Confirm AiProtection is **Enabled**.
4. Check that **Advertisement Blocking** and/or **Tracker Blocking** are toggled on.
5. The "Successfully Protected Events" counter will increment when blocks occur.

---

## 2. AdGuard Home on Routers

AdGuard Home is an open-source, self-hosted DNS sinkhole that blocks ads and
trackers at the DNS level. There are three ways to get AdGuard on an  router:

### 2.1 Pre-installed AdGuard DNS (Wi-Fi 7 / BE-Series Routers)

 has partnered with AdGuard to pre-install **AdGuard DNS** (the cloud-based
service, not AdGuard Home) on all Wi-Fi 7-compatible  routers.

**Supported models include:**
- RT-BE96U, RT-BE90U, RT-BE82U, RT-BE58U, RT-BE55, RT-BE50, RT-BE3600HP
- RT-AXE7800 (and other RT-AXE series)

**How it works:**
- AdGuard DNS is configured as an upstream DNS server on the router.
- DNS queries are forwarded to AdGuard's cloud DNS servers
  (`dns.adguard-dns.com` / IPs: `94.140.14.14`, `94.140.15.15`).
- Blocked domains receive `0.0.0.0` from the AdGuard DNS cloud.

**Configuration:**
- Web GUI → **WAN → DNS Server** → set DNS to AdGuard DNS, or
- Web GUI → **LAN → DHCP Server → DNS and WINS Server Setting** → specify AdGuard DNS IP.

### 2.2 AdGuard Home on AI Board (GT-BE19000AI and similar)

The  **GT-BE19000AI** router includes an "AI Board" (a built-in compute module
that can run containerized services).  provides an official AdGuard Home
installation guide for this platform.

**Installation steps (from  official FAQ #1055942):**
1. Log in to the AI Board web interface (`https://www.asusrouter.com:8443`).
2. Find the **AdGuard Home** section and click **Install**.
3. Note the AI Board hostname (e.g., `aiboard-anpu.local`).
4. Open `http://<aiboard-hostname>:3000` to access the AdGuard Home setup wizard.
5. Set admin interface port to **3000**.
6. Select DNS listen interface: **eth0** (typically `192.168.50.x`).
7. Create admin username and password.
8. Note the AdGuard Home IP address.
9. On the main router, go to **LAN → DHCP Server → DNS and WINS Server Setting**
   and set the DNS server to the AdGuard Home IP.

### 2.3 AdGuard Home via Asuswrt-Merlin + Entware (ARM Routers)

For users running **Asuswrt-Merlin** custom firmware, AdGuard Home can be installed
on ARM-based  routers using **Entware** (a package manager for embedded Linux).

**Requirements:**
- ARM-based  router (not bridges or access points)
- Asuswrt-Merlin firmware
- JFFS partition enabled
- Entware installed (via `amtm` — Asuswrt-Merlin Terminal Menu)

**Installation:**
```bash
# Install Entware via amtm
amtm

# Install AdGuard Home
amtm → oe (Entware) → install AdGuardHome
# or via the jumpsmm7 installer:
# https://github.com/jumpsmm7/Asuswrt-Merlin-AdGuardHome-Installer
```

**Port configuration:**
- AdGuard Home DNS listens on **port 53** (standard DNS) by default.
- Since the router's own `dnsmasq` also uses port 53, AdGuard Home is typically
  configured to listen on an alternate port (e.g., **5353**) and the router's
  `dnsmasq` is configured to forward all DNS queries to AdGuard Home, or
  `dnsmasq` is reconfigured to use a different port.
- Alternatively, `dnsmasq` can be set to use AdGuard Home as its upstream DNS,
  effectively chaining: client → dnsmasq (port 53) → AdGuard Home (port 5353)
  → upstream DNS.

**AdGuard Home web UI:** Default port **3000**.

### 2.4 AdGuard Home DNS Sinkhole Behavior

AdGuard Home is a true **DNS sinkhole**. When it blocks a domain:

1. The client sends a DNS query to AdGuard Home (directly or via router forwarding).
2. AdGuard Home checks the domain against its blocklists.
3. If blocked, AdGuard Home returns a sinkhole response instead of forwarding
   the query to upstream DNS.
4. The client receives the sinkhole response and (ideally) does not connect.

---

## 3. DNS Responses for Blocked Domains

This is the most critical section for the ESP32-S3 firmware. The firmware must
detect whether a domain is blocked by examining the DNS response.

### 3.1 Response Types Overview

| Blocking Mode | DNS RCODE | Answer Section | A Record Value | AAAA Record Value | Used By |
|---|---|---|---|---|---|
| **NULL / Null IP (default)** | NOERROR (0) | Present | `0.0.0.0` | `::` | AdGuard Home (default), Pi-hole (default) |
| **NXDOMAIN** | NXDOMAIN (3) | Empty | N/A | N/A | AdGuard Home (option), Pi-hole (option), CleanBrowsing, Quad9 |
| **REFUSED** | REFUSED (5) | Empty | N/A | N/A | AdGuard Home (option) |
| **NODATA** | NOERROR (0) | Empty | N/A | N/A | Pi-hole (option) |
| **Custom IP** | NOERROR (0) | Present | Configured IP (e.g., `192.168.1.42`) | Configured IPv6 | AdGuard Home (custom), Pi-hole (IP mode) |
| **Sinkhole IP (public)** | NOERROR (0) | Present | Specific sinkhole IP | Specific sinkhole IPv6 | Some enterprise DNS filters |

### 3.2 Detailed Descriptions

#### NULL / Null IP (0.0.0.0) — AdGuard Home & Pi-hole Default

This is the **most common** blocking mode. The DNS response has:
- **RCODE:** NOERROR (0)
- **Answer section:** Contains an A record with value `0.0.0.0`
- **AAAA:** Contains an AAAA record with value `::` (the IPv6 unspecified address)

```
;; QUESTION SECTION:
;doubleclick.net.        IN  A

;; ANSWER SECTION:
doubleclick.net.  300  IN  A   0.0.0.0
doubleclick.net.  300  IN  AAAA ::
```

**Why 0.0.0.0?** It is the "unspecified address" per RFC 3513. Clients that
receive `0.0.0.0` should not attempt to establish a TCP connection, because
`0.0.0.0` is not a routable address. In practice, most operating systems and
browsers immediately fail the connection.

**Advantages:**
- Clients do not attempt to connect, reducing network traffic.
- No HTTPS timeout issues (connection is immediately refused).
- No need for a web server to serve a block page.

**Disadvantages:**
- Some poorly-behaved clients may still try to connect to `0.0.0.0`, causing
  errors or timeouts.

#### NXDOMAIN

The DNS response indicates the domain does not exist:
- **RCODE:** NXDOMAIN (3)
- **Answer section:** Empty (no records returned)

```
;; QUESTION SECTION:
;doubleclick.net.        IN  A

;; (no answer section — header shows status: NXDOMAIN)
```

**Advantages:**
- Clean signal that the domain "does not exist."
- Some clients cache NXDOMAIN aggressively, reducing repeated queries.

**Disadvantages:**
- Some clients may fall back to alternative DNS servers when they receive
  NXDOMAIN, potentially bypassing the blocker.
- Clients may retry more frequently compared to NULL blocking.

#### REFUSED

The DNS server explicitly refuses to resolve the query:
- **RCODE:** REFUSED (5)
- **Answer section:** Empty

```
;; QUESTION SECTION:
;doubleclick.net.        IN  A

;; (no answer section — header shows status: REFUSED)
```

**Advantages:**
- Clearly indicates the query was refused (not that the domain doesn't exist).
- Less likely to trigger fallback to alternative DNS servers compared to NXDOMAIN.

#### NODATA

The domain exists but has no record of the requested type:
- **RCODE:** NOERROR (0)
- **Answer section:** Empty (no records)

```
;; QUESTION SECTION:
;doubleclick.net.        IN  A

;; (no answer section — header shows status: NOERROR)
```

**Advantages:**
- Clients accept this more gracefully than NXDOMAIN.
- Indicates the domain is valid but has no A record.

#### Custom IP (Sinkhole)

The DNS server returns a specific IP address (usually the address of a block
page server):
- **RCODE:** NOERROR (0)
- **Answer section:** A record with a configured IP (e.g., `192.168.1.42`)

**Used by:**
- AdGuard Home "Custom IP" blocking mode (configurable in Settings → DNS Settings).
- Pi-hole "IP" blocking mode (returns the Pi-hole's own IP).
- Some enterprise DNS filters that redirect to a block page.

### 3.3 Common Sinkhole IPs to Detect

| IP Address | Meaning | Source |
|---|---|---|
| `0.0.0.0` | NULL route / unspecified address | AdGuard Home (default), Pi-hole (default) |
| `::` | IPv6 unspecified address | AdGuard Home (default), Pi-hole (default) |
| `127.0.0.1` | Loopback (some ISP-level blockers use this) | Some ISP DNS resolvers |
| `0.0.0.1` | Some sinkhole implementations | Various |
| `192.168.x.x` (local) | Pi-hole / AdGuard Home own IP (IP mode) | Pi-hole IP mode, AdGuard custom IP |
| `10.x.x.x` (local) | Local sinkhole | Custom configurations |
| `255.255.255.255` | Broadcast (some blockers) | Rare |

### 3.4 Detection Logic Summary

To determine if a domain is blocked, check in this order:

1. **RCODE is NXDOMAIN (3)** → Blocked (NXDOMAIN mode)
2. **RCODE is REFUSED (5)** → Blocked (REFUSED mode)
3. **RCODE is NOERROR (0) but answer section is empty** → Blocked (NODATA mode)
4. **RCODE is NOERROR (0) and A record is `0.0.0.0`** → Blocked (NULL mode)
5. **RCODE is NOERROR (0) and AAAA record is `::`** → Blocked (NULL mode, IPv6)
6. **A record is a known sinkhole IP** (see table above) → Blocked (Custom IP mode)
7. **A record is a private/local IP** that is not the expected upstream → Possibly blocked (Custom IP mode)
8. **RCODE is NOERROR (0) and A record is a public, routable IP** → **Not blocked**

---

## 4. Testing Protocol Design

### 4.1 Objective

Test whether the  router (acting as DNS resolver or forwarding to AdGuard
Home) blocks a specific domain by sending a DNS query directly to the router
and analyzing the response.

### 4.2 Prerequisites

- Router IP address (typically `192.168.1.1` or `192.168.50.1` for ).
- Network connectivity to the router (same LAN).
- DNS query tool: `dig`, `nslookup`, or Python with `dnspython` / raw UDP.

### 4.3 Step-by-Step Protocol

#### Step 1: Identify the Router's DNS IP

The  router typically listens on port 53 at its LAN IP. Common defaults:
- `192.168.1.1` (default for most  routers)
- `192.168.50.1` (some newer models / AiMesh nodes)
- `192.168.2.1` (some configurations)

If AdGuard Home is installed, the DNS server may be the AdGuard Home IP instead
of the router IP (e.g., `192.168.50.x` where x is the AI Board / AdGuard Home
address).

#### Step 2: Send a DNS Query for a Known Good Domain (Baseline)

Query a domain that should **never** be blocked (e.g., `example.com`).

```bash
dig @192.168.1.1 example.com A +short
```

**Expected result:** A valid public IP address (e.g., `93.184.216.34`).

If this fails, the router is not responding to DNS queries on port 53, or
network connectivity is broken. Troubleshoot before continuing.

#### Step 3: Send a DNS Query for a Known Ad Domain (Test)

Query a domain that should be blocked (e.g., `doubleclick.net`).

```bash
dig @192.168.1.1 doubleclick.net A +short
dig @192.168.1.1 doubleclick.net A
```

#### Step 4: Analyze the Response

Compare the response against the detection logic in Section 3.4:

- If `0.0.0.0` is returned → **Blocked** (NULL mode)
- If NXDOMAIN status → **Blocked** (NXDOMAIN mode)
- If REFUSED status → **Blocked** (REFUSED mode)
- If empty answer with NOERROR → **Blocked** (NODATA mode)
- If a real public IP → **Not blocked** (or the domain is not in the blocklist)

#### Step 5: Test AAAA (IPv6) Records

```bash
dig @192.168.1.1 doubleclick.net AAAA +short
```

- If `::` is returned → **Blocked** (NULL mode, IPv6)
- If NXDOMAIN / REFUSED / empty → **Blocked**
- If a real IPv6 address → **Not blocked** (IPv6 path)

#### Step 6: Test Multiple Ad Domains

Repeat Steps 3-5 with multiple domains from the ad domains list (Section 5).
A single domain may not be in the blocklist; testing multiple increases
confidence.

#### Step 7: Verify with a Non-Ad Domain (Negative Control)

Query a domain that should definitely not be blocked:

```bash
dig @192.168.1.1 wikipedia.org A +short
```

This should return a valid public IP. If it returns `0.0.0.0` or NXDOMAIN,
something is wrong (over-blocking or DNS misconfiguration).

### 4.4 Edge Cases

#### CNAME Chains (CNAME Cloaking)

Some ad domains use CNAME records to disguise their tracking domains. AdGuard
Home has a "Blocked by CNAME or IP" feature that detects when a CNAME in the
response chain points to a blocked domain.

**Testing:** Query a domain that is known to CNAME-chain to a tracker:
```bash
dig @192.168.1.1 someblog.example.com A
```
If the CNAME chain includes a blocked domain, AdGuard Home will block the entire
response. The response will appear as a blocked response (0.0.0.0 / NXDOMAIN).

**Firmware handling:** The ESP32-S3 should follow CNAME chains in the DNS
response and check if any CNAME target is in the blocked-domain list.

#### DNS over HTTPS (DoH) / DNS over TLS (DoT)

If a client uses DoH or DoT, it bypasses the router's DNS entirely (queries go
directly to a DoH/DoT server like `dns.google` or `cloudflare-dns.com` over
HTTPS/TLS). The router cannot intercept these queries at the DNS level.

** routers** can enable DNS-over-TLS (DoT) in the firmware, but this affects
the router's own upstream queries, not client-to-router queries.

**Testing implication:** The ESP32-S3 sends a plain UDP DNS query to the router
on port 53, so DoH/DoT is not a concern for the test itself. However, note that
DoH-using clients on the network would not be protected by AdGuard Home.

#### IPv6 (AAAA Records)

Some networks use IPv6. The router may or may not block IPv6 DNS queries.

**Testing:**
- Query AAAA records in addition to A records.
- A blocked AAAA response may return `::` (NULL mode) or NXDOMAIN.
- If the network has no IPv6, AAAA queries may return NODATA regardless.

#### Caching

DNS responses are cached by the router and by the client. If a domain was
queried before AdGuard Home was enabled, the cached response may be the real IP.

**Mitigation:** Use a random subdomain or append a random query ID, or flush the
DNS cache before testing. The ESP32-S3 can use a fresh random transaction ID
for each query (standard DNS behavior) to avoid cache-collision issues, though
the DNS cache will still return cached results for the same domain name.

#### Port 53 vs Alternate Ports

If AdGuard Home is running on an alternate port (e.g., 5353) behind dnsmasq
forwarding, querying port 53 on the router will still work (dnsmasq forwards to
AdGuard Home). If AdGuard Home is running directly on port 53 (AI Board setup),
querying port 53 goes directly to AdGuard Home.

### 4.5 Python Testing Code

#### 4.5.1 Using dnspython (Full-featured)

```python
#!/usr/bin/env python3
"""
Test whether a DNS resolver (e.g.,  router with AdGuard Home) blocks a domain.
Uses dnspython library.
"""

import dns.resolver
import dns.rcode
import dns.flags
import sys
import ipaddress

# Known sinkhole IPs that indicate blocking
SINKHOLE_IPS = {
    "0.0.0.0",        # NULL route (AdGuard Home / Pi-hole default)
    "::",             # IPv6 unspecified
    "127.0.0.1",      # Loopback (some ISP blockers)
    "0.0.0.1",        # Some sinkhole implementations
}

# Private IP ranges (may indicate custom IP blocking mode)
PRIVATE_RANGES = [
    ipaddress.ip_network("10.0.0.0/8"),
    ipaddress.ip_network("172.16.0.0/12"),
    ipaddress.ip_network("192.168.0.0/16"),
    ipaddress.ip_network("169.254.0.0/16"),
]


def is_sinkhole_ip(ip_str: str) -> bool:
    """Check if an IP is a known sinkhole or private address."""
    if ip_str in SINKHOLE_IPS:
        return True
    try:
        ip = ipaddress.ip_address(ip_str)
        for net in PRIVATE_RANGES:
            if ip in net:
                return True
    except ValueError:
        return True  # Unparseable IP → treat as suspicious
    return False


def test_domain(domain: str, dns_server: str = "192.168.1.1", 
                record_type: str = "A", timeout: float = 5.0) -> dict:
    """
    Test if a domain is blocked by the DNS resolver.
    
    Returns a dict with:
        - 'domain': the queried domain
        - 'blocked': True/False/None (None = error)
        - 'block_mode': description of how it was blocked
        - 'rcode': DNS response code name
        - 'answer': list of answer strings
        - 'raw_response': the full dns.message object
    """
    resolver = dns.resolver.Resolver()
    resolver.nameservers = [dns_server]
    resolver.timeout = timeout
    resolver.lifetime = timeout
    
    rdtype = dns.rdatatype.A if record_type == "A" else dns.rdatatype.AAAA
    
    result = {
        "domain": domain,
        "record_type": record_type,
        "blocked": None,
        "block_mode": None,
        "rcode": None,
        "answer": [],
        "error": None,
    }
    
    try:
        response = resolver.resolve(domain, rdtype)
        result["rcode"] = dns.rcode.to_text(response.response.rcode())
        
        # Extract answer records
        answers = []
        for rrset in response.response.answer:
            for rdata in rrset:
                ip_str = str(rdata)
                answers.append(ip_str)
        result["answer"] = answers
        
        # Determine if blocked
        if not answers:
            # NOERROR with no answer → NODATA mode
            result["blocked"] = True
            result["block_mode"] = "NODATA"
        else:
            # Check if any answer is a sinkhole IP
            all_sinkhole = all(is_sinkhole_ip(ip) for ip in answers)
            if all_sinkhole:
                if "0.0.0.0" in answers or "::" in answers:
                    result["blocked"] = True
                    result["block_mode"] = "NULL_IP"
                else:
                    result["blocked"] = True
                    result["block_mode"] = "CUSTOM_SINKHOLE_IP"
            else:
                # Check if ANY answer is a sinkhole (mixed response)
                if any(is_sinkhole_ip(ip) for ip in answers):
                    result["blocked"] = True
                    result["block_mode"] = "PARTIAL_NULL"
                else:
                    result["blocked"] = False
                    result["block_mode"] = "NOT_BLOCKED"
                    
    except dns.resolver.NXDOMAIN:
        result["blocked"] = True
        result["block_mode"] = "NXDOMAIN"
        result["rcode"] = "NXDOMAIN"
    except dns.resolver.NoNameservers:
        # Could be REFUSED
        result["blocked"] = True
        result["block_mode"] = "REFUSED_OR_NO_NAMESERVERS"
        result["rcode"] = "REFUSED"
    except dns.resolver.LifetimeTimeout:
        result["blocked"] = None
        result["error"] = "Timeout"
    except dns.resolver.NoAnswer:
        # NOERROR but no answer section
        result["blocked"] = True
        result["block_mode"] = "NODATA"
        result["rcode"] = "NOERROR"
    except Exception as e:
        result["blocked"] = None
        result["error"] = str(e)
    
    return result


def test_domain_both_records(domain: str, dns_server: str = "192.168.1.1") -> dict:
    """Test both A and AAAA records for a domain."""
    result_a = test_domain(domain, dns_server, "A")
    result_aaaa = test_domain(domain, dns_server, "AAAA")
    
    return {
        "domain": domain,
        "A": result_a,
        "AAAA": result_aaaa,
        "blocked": result_a["blocked"] or result_aaaa["blocked"],
    }


# --- Example Usage ---
if __name__ == "__main__":
    router_ip = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.1"
    
    # Known good domain (baseline)
    print("=== Baseline Test ===")
    baseline = test_domain("example.com", router_ip)
    print(f"example.com: blocked={baseline['blocked']}, "
          f"mode={baseline['block_mode']}, answer={baseline['answer']}")
    
    # Known ad domain
    print("\n=== Ad Domain Test ===")
    test = test_domain("doubleclick.net", router_ip)
    print(f"doubleclick.net: blocked={test['blocked']}, "
          f"mode={test['block_mode']}, answer={test['answer']}")
    
    # Full A + AAAA test
    print("\n=== Full A + AAAA Test ===")
    full = test_domain_both_records("googlesyndication.com", router_ip)
    print(f"googlesyndication.com: blocked={full['blocked']}")
    print(f"  A:    blocked={full['A']['blocked']}, mode={full['A']['block_mode']}, "
          f"answer={full['A']['answer']}")
    print(f"  AAAA: blocked={full['AAAA']['blocked']}, mode={full['AAAA']['block_mode']}, "
          f"answer={full['AAAA']['answer']}")
```

#### 4.5.2 Raw UDP DNS Query (No External Libraries — ESP32-Friendly)

This is a minimal implementation suitable for porting to ESP32-S3 firmware (C/C++).
It constructs a raw DNS query packet over UDP and parses the response.

```python
#!/usr/bin/env python3
"""
Minimal raw DNS query over UDP — no external libraries.
Suitable as a reference for ESP32-S3 firmware implementation.
"""

import socket
import struct
import random

SINKHOLE_IPS = {"0.0.0.0", "127.0.0.1"}


def build_dns_query(domain: str, qtype: int = 1) -> bytes:
    """
    Build a raw DNS query packet.
    qtype: 1=A, 28=AAAA, 5=CNAME
    """
    # Transaction ID (random 16-bit)
    txn_id = random.randint(0, 0xFFFF)
    
    # Flags: RD=1 (recursion desired), standard query
    flags = 0x0100
    
    # Counts: 1 question, 0 answer, 0 authority, 0 additional
    header = struct.pack(">HHHHHH", txn_id, flags, 1, 0, 0, 0)
    
    # Question section: encode domain as DNS labels
    qname = b""
    for label in domain.rstrip(".").split("."):
        qname += struct.pack("B", len(label)) + label.encode("ascii")
    qname += b"\x00"  # null terminator
    
    # Question: qname + qtype + qclass (IN=1)
    question = qname + struct.pack(">HH", qtype, 1)
    
    return header + question


def parse_dns_response(data: bytes) -> dict:
    """
    Parse a raw DNS response packet.
    Returns dict with rcode, answers, etc.
    """
    if len(data) < 12:
        return {"error": "Response too short"}
    
    # Parse header
    txn_id, flags, qdcount, ancount, nscount, arcount = struct.unpack(
        ">HHHHHH", data[:12]
    )
    
    rcode = flags & 0x0F
    rcode_names = {0: "NOERROR", 1: "FORMERR", 2: "SERVFAIL", 
                   3: "NXDOMAIN", 5: "REFUSED"}
    
    result = {
        "txn_id": txn_id,
        "flags": flags,
        "rcode": rcode,
        "rcode_name": rcode_names.get(rcode, f"UNKNOWN({rcode})"),
        "answer_count": ancount,
        "answers": [],
    }
    
    # Skip question section
    offset = 12
    for _ in range(qdcount):
        offset = skip_name(data, offset)
        offset += 4  # qtype + qclass
    
    # Parse answer section
    for _ in range(ancount):
        # Parse name (may use compression)
        name, offset = parse_name(data, offset)
        # Parse type, class, TTL, rdlength
        rtype, rclass, ttl, rdlength = struct.unpack(
            ">HHIH", data[offset:offset + 10]
        )
        offset += 10
        rdata = data[offset:offset + rdlength]
        offset += rdlength
        
        answer = {"name": name, "type": rtype, "ttl": ttl, "rdata": rdata}
        
        if rtype == 1:  # A record
            if len(rdata) == 4:
                ip = ".".join(str(b) for b in rdata)
                answer["ip"] = ip
                result["answers"].append(ip)
        elif rtype == 28:  # AAAA record
            if len(rdata) == 16:
                ip = ":".join(f"{rdata[i]:02x}{rdata[i+1]:02x}" 
                              for i in range(0, 16, 2))
                answer["ip"] = ip
                result["answers"].append(ip)
        elif rtype == 5:  # CNAME
            cname, _ = parse_name(data, offset - rdlength)
            answer["cname"] = cname
        
    return result


def skip_name(data: bytes, offset: int) -> int:
    """Skip a DNS name field, handling compression pointers."""
    while offset < len(data):
        length = data[offset]
        if length == 0:
            return offset + 1
        if (length & 0xC0) == 0xC0:
            return offset + 2  # compression pointer
        offset += length + 1
    return offset


def parse_name(data: bytes, offset: int) -> tuple:
    """Parse a DNS name, handling compression pointers."""
    labels = []
    jumped = False
    original_offset = offset
    
    while offset < len(data):
        length = data[offset]
        if length == 0:
            offset += 1
            break
        if (length & 0xC0) == 0xC0:
            pointer = struct.unpack(">H", data[offset:offset + 2])[0] & 0x3FFF
            if not jumped:
                original_offset = offset + 2
            offset = pointer
            jumped = True
            continue
        offset += 1
        labels.append(data[offset:offset + length].decode("ascii", errors="replace"))
        offset += length
    
    name = ".".join(labels)
    return name, (original_offset if jumped else offset)


def query_dns(domain: str, dns_server: str = "192.168.1.1", 
              port: int = 53, qtype: int = 1, timeout: float = 5.0) -> dict:
    """
    Send a raw DNS query and return the parsed response.
    qtype: 1=A, 28=AAAA
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(timeout)
    
    query = build_dns_query(domain, qtype)
    
    try:
        sock.sendto(query, (dns_server, port))
        data, _ = sock.recvfrom(4096)
        return parse_dns_response(data)
    except socket.timeout:
        return {"error": "timeout", "rcode": -1, "answers": []}
    finally:
        sock.close()


def is_blocked(domain: str, dns_server: str = "192.168.1.1") -> dict:
    """
    Test if a domain is blocked by the DNS resolver.
    Returns dict with 'blocked', 'mode', and 'details'.
    """
    result = {"domain": domain, "blocked": False, "mode": None, "details": {}}
    
    # Test A record
    a_response = query_dns(domain, dns_server, qtype=1)
    result["details"]["A"] = a_response
    
    rcode = a_response.get("rcode", -1)
    answers = a_response.get("answers", [])
    
    if rcode == 3:  # NXDOMAIN
        result["blocked"] = True
        result["mode"] = "NXDOMAIN"
    elif rcode == 5:  # REFUSED
        result["blocked"] = True
        result["mode"] = "REFUSED"
    elif rcode == 0 and not answers:  # NOERROR with no answer
        result["blocked"] = True
        result["mode"] = "NODATA"
    elif rcode == 0 and answers:
        if all(ip in SINKHOLE_IPS for ip in answers):
            result["blocked"] = True
            result["mode"] = "NULL_IP"
        elif any(ip in SINKHOLE_IPS for ip in answers):
            result["blocked"] = True
            result["mode"] = "PARTIAL_SINKHOLE"
        else:
            result["blocked"] = False
            result["mode"] = "NOT_BLOCKED"
    elif rcode == -1:
        result["blocked"] = None
        result["mode"] = "ERROR"
    
    return result


# --- Example Usage ---
if __name__ == "__main__":
    import sys
    
    router_ip = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.1"
    
    print(f"Testing DNS resolver at {router_ip}\n")
    
    # Baseline
    result = is_blocked("example.com", router_ip)
    print(f"example.com:         blocked={result['blocked']}, mode={result['mode']}")
    
    # Ad domains
    test_domains = [
        "doubleclick.net",
        "googlesyndication.com",
        "googleads.g.doubleclick.net",
        "adservice.google.com",
        "connect.facebook.net",
        "scorecardresearch.com",
    ]
    
    for domain in test_domains:
        result = is_blocked(domain, router_ip)
        print(f"{domain:30s} blocked={result['blocked']}, mode={result['mode']}")
```

---

## 5. Common Ad / Tracking Domains

The following domains are well-known ad-serving, tracking, and analytics domains
commonly found in DNS blocklists. A JSON version with categorization is in
`ad-domains-list.json`.

### Google Ads
- `doubleclick.net`
- `googlesyndication.com`
- `googleads.g.doubleclick.net`
- `adservice.google.com`
- `adservice.google.com`
- `googleadservices.com`
- `googletagservices.com`
- `partner.googleadservices.com`
- `pagead2.googlesyndication.com`
- `adwords.google.com`
- `ads.google.com`

### Facebook / Meta
- `connect.facebook.net`
- `connect.facebook.com`
- `graph.facebook.com`
- `pixel.facebook.com`
- `an.facebook.com`

### Analytics
- `google-analytics.com`
- `ssl.google-analytics.com`
- `scorecardresearch.com`
- `quantserve.com`
- `comscore.com`
- `hotjar.com`
- `mixpanel.com`
- `segment.io`
- `amplitude.com`
- `newrelic.com`
- `chartbeat.com`
- `quantcount.com`

### Trackers
- `criteo.com`
- `criteo.net`
- `advertising.com`
- `eyeota.net`
- `tapad.com`
- `bluekai.com`
- `demdex.net`
- `omtrdc.net`
- `evidon.com`
- `krxd.net`

### Social Media Trackers
- `platform.twitter.com`
- `analytics.twitter.com`
- `t.co`
- `ads.linkedin.com`
- `ads.pinterest.com`
- `ads.tiktok.com`
- `analytics.tiktok.com`
- `business.tiktok.com`

### Ad Networks
- `adnxs.com`
- `adcolony.com`
- `applovin.com`
- `chartboost.com`
- `inmobi.com`
- `mopub.com`
- `unityads.unity3d.com`
- `vungle.com`
- `media.net`
- `outbrain.com`
- `taboola.com`
- `revcontent.com`

### Microsoft / Bing Ads
- `adsymptotic.com`
- `ads.microsoft.com`
- `bat.bing.com`
- `clarity.ms`

### Amazon Ads
- `amazon-adsystem.com`
- `aax.amazon-adsystem.com`

---

## 6. Router Configuration Notes

### 6.1 Router Models with Ad Blocking Support

#### Built-in AiProtection (Advertisement Blocking + Tracker Blocking)

Available on most mid-range and high-end  routers with WRT firmware.
Not available on entry-level models (e.g., RT-AX53U, RT-AX3000 in some regions).

**Models known to support AiProtection:**
- RT-AX series: RT-AX88U, RT-AX86U, RT-AX86S, RT-AX82U, RT-AX58U, RT-AX56U,
  RT-AX3000 (varies by region), RT-AX55, RT-AX68U, RT-AX89X, RT-AX92U
- RT-AXE series: RT-AXE7800, RT-AXE95Q
- RT-AX Pro / RT-AX86U Pro
- GT series: GT-AXE16000, GT-AX11000, GT-AX6000
- RT-BE series (Wi-Fi 7): RT-BE96U, RT-BE90U, RT-BE82U, RT-BE58U, RT-BE55,
  RT-BE50, RT-BE3600HP

> Note: Model availability and AiProtection support vary by region and firmware
> version. Check the  product page for each model.

#### Pre-installed AdGuard DNS (Wi-Fi 7 / BE-Series)

All Wi-Fi 7-compatible  routers have AdGuard DNS pre-installed as a
configurable option:
- RT-BE96U, RT-BE90U, RT-BE82U, RT-BE58U, RT-BE55, RT-BE50, RT-BE3600HP
- RT-AXE7800

**Configuration:**
1. Web GUI → **WAN → DNS Server**
2. Set "Connect to DNS Server automatically" to **No**
3. Enter AdGuard DNS IPs: `94.140.14.14` and `94.140.15.15`
4. Or use the AdGuard DNS toggle if available in firmware

#### AdGuard Home on AI Board

- **GT-BE19000AI** (and future AI Router models with AI Board)

**Configuration:**
1. Access AI Board web interface (`https://www.asusrouter.com:8443`)
2. Install AdGuard Home from the AI Board dashboard
3. Configure via `http://<aiboard-hostname>:3000`
4. Set DNS listen interface to **eth0** (192.168.50.x subnet)
5. Set router DHCP DNS to the AdGuard Home IP

#### AdGuard Home via Asuswrt-Merlin + Entware

**Supported models (ARM-based, running Asuswrt-Merlin):**
- RT-AC68U, RT-AC86U, RT-AC88U, RT-AC3100, RT-AC3200
- RT-AX56U, RT-AX58U, RT-AX88U, RT-AX86U, RT-AX86S
- GT-AC2900, GT-AC5300, GT-AX11000, GT-AX6000
- (Full list: https://asuswrt-merlin.net/)

> Note: MIPS-based routers (older RT-AC66U, RT-N66U) are **not** supported by
> Entware ARM packages and cannot run AdGuard Home.

### 6.2 Port Summary

| Component | Port | Protocol | Purpose |
|---|---|---|---|
| Router DNS (dnsmasq) | 53 | UDP/TCP | Standard DNS for LAN clients |
| AdGuard Home DNS | 53 or 5353 | UDP/TCP | DNS sinkhole (configurable) |
| AdGuard Home Web UI | 3000 | TCP | Admin dashboard |
|  Web GUI | 80/8443 | TCP | Router management |
| DNS over TLS (DoT) | 853 | TCP/TLS | Encrypted upstream DNS (router to upstream) |

### 6.3 AdGuard Home Configuration (Blocking Mode)

In AdGuard Home web UI → **Settings → DNS settings → Blocking mode**:

| Option | Description |
|---|---|
| **Default** | Returns `0.0.0.0` for A, `::` for AAAA (NULL blocking) |
| **REFUSED** | Returns DNS REFUSED (rcode 5) |
| **NXDOMAIN** | Returns NXDOMAIN (rcode 3) |
| **Null IP** | Same as Default — returns `0.0.0.0` / `::` |
| **Custom IP** | Returns a user-specified IP address |

The default blocking mode in AdGuard Home is **Null IP** (`0.0.0.0` / `::`).

### 6.4 Router DNS Configuration for AdGuard Home

To ensure all LAN clients use AdGuard Home:

1. **LAN → DHCP Server → DNS and WINS Server Setting**
   - Set "DNS Server 1" to the AdGuard Home IP address.
   - Leave "DNS Server 2" empty or set to the same address.

2. **Force DNS redirection (optional, for Asuswrt-Merlin):**
   - Use the `DNSFilter` feature: **Parental Controls → DNSFilter**
   - Set "Global Filter Mode" to "Router"
   - This forces all port 53 traffic through the router's DNS, preventing
     clients from bypassing AdGuard Home by using custom DNS servers.

3. **dnsmasq forwarding (for Entware-based AdGuard Home):**
   - Configure dnsmasq to forward DNS to AdGuard Home:
   ```bash
   # In /jffs/configs/dnsmasq.conf.add:
   server=127.0.0.1#5353
   no-resolv
   ```
   - This makes dnsmasq (port 53) forward all queries to AdGuard Home (port 5353).

---

## 7. ESP32-S3 Firmware Integration Notes

### 7.1 Key Considerations for Firmware Implementation

1. **DNS Query:** The ESP32-S3 should send a raw UDP DNS query to the router IP
   on port 53. Use the raw UDP socket implementation (Section 4.5.2) as reference.

2. **Query Both A and AAAA:** Test both record types for completeness.

3. **Detection Logic Priority:**
   ```
   1. Check rcode: NXDOMAIN(3) → blocked
   2. Check rcode: REFUSED(5) → blocked
   3. Check rcode: SERVFAIL(2) → upstream error (not necessarily blocked)
   4. Check rcode: NOERROR(0):
      a. No answer records → NODATA → blocked
      b. A record == 0.0.0.0 → NULL_IP → blocked
      c. AAAA record == :: → NULL_IP → blocked
      d. A record in private range (10.x, 172.16-31.x, 192.168.x) → CUSTOM_IP → blocked
      e. A record is public routable IP → NOT blocked
   ```

4. **Timeout Handling:** Use a 3-5 second timeout. If the router doesn't
   respond, it may indicate the DNS service is down or blocking the query
   silently. Retry 2-3 times.

5. **Multiple Domain Testing:** Test multiple domains (from the ad-domains-list.json)
   and calculate a blocking percentage. A router with AdGuard Home should block
   >90% of domains in the list.

6. **Baseline Check:** Always first query `example.com` or `wikipedia.org` to
   confirm the router is responding to DNS at all.

7. **AiProtection Caveat:**  AiProtection may block at the connection level
   (IPS/firewall) rather than DNS level. A DNS query may return the real IP,
   but the HTTP connection will be blocked. To detect this, after getting a
   "not blocked" DNS response, attempt an HTTP connection to the resolved IP
   and check if the connection is refused/reset. However, this is beyond pure
   DNS testing.

8. **CNAME Following:** When parsing DNS responses, follow CNAME chains. If
   the response contains CNAME records pointing to blocked domains, AdGuard
   Home would have blocked the entire response. The firmware should check
   CNAME targets against the known blocked-domain patterns.

9. **DNS Transaction ID:** Use a random 16-bit transaction ID for each query
   and verify it matches in the response. This prevents response spoofing and
   cache poisoning issues.

10. **EDNS (OPT record):** Some DNS servers use EDNS0 (extended DNS). The
    firmware can optionally include an EDNS OPT record in the query, but it's
    not required for basic A/AAAA queries.

### 7.2 Firmware Test Flow (Pseudocode)

```
function test_ad_blocking(router_ip):
    // Step 1: Baseline
    baseline = dns_query(router_ip, 53, "example.com", TYPE_A)
    if baseline.error or baseline.timeout:
        return ERROR("Router not responding to DNS")
    if baseline.blocked:
        return ERROR("Router is over-blocking (example.com is blocked)")
    
    // Step 2: Test ad domains
    blocked_count = 0
    total = 0
    for domain in AD_DOMAINS_LIST:
        result_a = dns_query(router_ip, 53, domain, TYPE_A)
        result_aaaa = dns_query(router_ip, 53, domain, TYPE_AAAA)
        
        if result_a.blocked or result_aaaa.blocked:
            blocked_count++
        total++
        
        delay(100ms)  // avoid flooding
    
    // Step 3: Report
    blocking_percentage = blocked_count / total * 100
    if blocking_percentage > 80:
        return AD_BLOCKING_ACTIVE
    elif blocking_percentage > 20:
        return PARTIAL_BLOCKING
    else:
        return NO_BLOCKING
```

### 7.3 Sinkhole IP Detection Table (Firmware-Ready)

```c
// Known sinkhole IPs (4 bytes each, network byte order)
static const uint8_t SINKHOLE_IPS[][4] = {
    {0, 0, 0, 0},      // 0.0.0.0 - NULL (AdGuard/Pi-hole default)
    {127, 0, 0, 1},    // 127.0.0.1 - loopback
    {0, 0, 0, 1},      // 0.0.0.1 - some implementations
};

// IPv6 unspecified address (::)
static const uint8_t SINKHOLE_IPV6[16] = {0};  // all zeros = ::

// Private IP ranges (first octet check for quick filter)
static const uint8_t PRIVATE_PREFIXES[] = {10, 172, 192};

bool is_sinkhole_ipv4(const uint8_t ip[4]) {
    // Check against known sinkhole IPs
    for (int i = 0; i < sizeof(SINKHOLE_IPS)/4; i++) {
        if (memcmp(ip, SINKHOLE_IPS[i], 4) == 0) return true;
    }
    // Check private ranges
    if (ip[0] == 10) return true;
    if (ip[0] == 172 && (ip[1] >= 16 && ip[1] <= 31)) return true;
    if (ip[0] == 192 && ip[1] == 168) return true;
    return false;
}

bool is_sinkhole_ipv6(const uint8_t ip[16]) {
    // Check for :: (all zeros)
    if (memcmp(ip, SINKHOLE_IPV6, 16) == 0) return true;
    // Check for ::1 (loopback)
    uint8_t loopback[16] = {0};
    loopback[15] = 1;
    if (memcmp(ip, loopback, 16) == 0) return true;
    // Check for fc00::/7 (ULA - unique local addresses)
    if ((ip[0] & 0xFE) == 0xFC) return true;
    // Check for fe80::/10 (link-local)
    if (ip[0] == 0xFE && (ip[1] & 0xC0) == 0x80) return true;
    return false;
}
```

---

## References

-  AiProtection FAQ: https://www.asus.com/support/faq/1008719/
-  AdGuard Home Installation Guide: https://www.asus.com/support/faq/1055942/
-  AdGuard DNS Setup: https://www.asus.com/support/faq/1051213/
- AdGuard DNS pre-installed in  Wi-Fi 7 routers: https://adguard.com/en/blog/adguard-dns-is-now-pre-installed-in-all-asus-wi-fi-7-routers.html
- AdGuard Home FAQ: https://adguard-dns.io/kb/adguard-home/faq/
- Pi-hole Blocking Modes: https://docs.pi-hole.net/ftldns/blockingmode/
- Comparing DNS Blocking Methods (ADAM Networks): https://support.adamnet.works/t/comparing-dns-blocking-methods/1245
- Asuswrt-Merlin AdGuard Home Installer: https://github.com/jumpsmm7/Asuswrt-Merlin-AdGuardHome-Installer
- SNB Forums AdGuard Home Setup: https://www.snbforums.com/threads/adguard-home-dns-sinkhole-setup.91466/
- SNB Forums Comprehensive AdGuard Home Tutorial: https://www.snbforums.com/threads/one-comprehensive-adguard-home-installation-tutorial-amtm.90684/
- RFC 3513 (IPv6 Addressing Architecture, unspecified address): https://tools.ietf.org/html/rfc3513
- RFC 8914 (Extended DNS Errors): https://tools.ietf.org/html/rfc8914

---

*Document generated for the ESP32-S3 Pi-hole project. Last updated: 2026-08-16.*
