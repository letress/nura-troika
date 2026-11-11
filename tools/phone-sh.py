#!/usr/bin/env python3
"""Spustí příkaz v shellu testovacího initramfs na telefonu přes telnet (USB síť).

    tools/phone-sh.py 'dmesg | tail'

Telefon musí běžet z testovacího image s USB (172.16.42.1). Jen čte výstup;
co příkaz na telefonu udělá, je na volajícím.
"""
import socket, sys, time
IAC, DONT, DO, WONT, WILL, SB, SE = 255, 254, 253, 252, 251, 250, 240
def run(cmd, host='172.16.42.1', port=23, timeout=20):
    s = socket.create_connection((host, port), timeout=5)
    s.settimeout(1.0)
    out = bytearray()
    def pump(until=None, t=timeout):
        end = time.time() + t
        while time.time() < end:
            try:
                d = s.recv(65536)
            except socket.timeout:
                if until is None: return
                continue
            if not d: return
            i = 0
            while i < len(d):
                b = d[i]
                if b == IAC and i + 1 < len(d):
                    c = d[i + 1]
                    if c in (DO, DONT, WILL, WONT) and i + 2 < len(d):
                        o = d[i + 2]
                        s.sendall(bytes([IAC, WONT if c == DO else DONT, o]) if c in (DO, WILL) else b'')
                        i += 3; continue
                    if c == SB:
                        j = d.find(bytes([IAC, SE]), i)
                        i = (j + 2) if j >= 0 else len(d); continue
                    i += 2; continue
                out.append(b); i += 1
            if until and until in out: return
    pump(None, 2)
    out.clear()
    s.sendall(("stty -echo 2>/dev/null; echo __BEG''IN__; %s; echo __E''ND__\n" % cmd).encode())
    pump(b'__END__\r', timeout)
    s.close()
    t = out.decode('utf-8', 'replace').replace('\r', '')
    a = t.rfind('__BEGIN__\n'); b = t.rfind('__END__')
    return t[a + 10:b] if a >= 0 and b > a else t
if __name__ == '__main__':
    sys.stdout.write(run(' '.join(sys.argv[1:])))
