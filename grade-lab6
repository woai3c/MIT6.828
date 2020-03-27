#!/usr/bin/env python

import os, re, threading, socket, time, shutil, struct, difflib
try:
    from urllib2 import urlopen, HTTPError
except ImportError:
    # Python 3
    from urllib.request import urlopen
    from urllib.error import HTTPError
from gradelib import *

r = Runner(save("jos.out"),
           stop_breakpoint("readline"))

def match_packet_seq(got, expect):
    got, expect = list(got), list(expect)
    s = difflib.SequenceMatcher(None, got, [d for n, d in expect])
    msgs, bad = [], False
    for tag, i1, i2, j1, j2 in s.get_opcodes():
        wrong = []
        if tag in ("delete", "replace"):
            wrong.append([("unexpected packet:", pkt) for pkt in got[i1:i2]])
        if tag in ("insert", "replace"):
            wrong.append([("missing %s:" % n, pkt) for n, pkt in expect[j1:j2]])
        for seq in wrong:
            bad = True
            for msg, pkt in seq[:3]:
                msgs.append(color("red", msg) + "\n" + hexdump(pkt))
            if seq[3:]:
                msgs.append("... plus %d more" % len(seq[3:]))

        if tag == "equal":
            msgs.append(color("green", "got ") + expect[j1][0])
            if j1 + 1 < j2:
                msgs[-1] += " through " + expect[j2 - 1][0]
    assert not bad, "\n".join(msgs)

def save_pcap_on_fail():
    def save_pcap(fail):
        save_path = "qemu.pcap." + get_current_test().__name__[5:]
        if fail and os.path.exists("qemu.pcap"):
            shutil.copyfile("qemu.pcap", save_path)
            print("    Packet capture saved to %s" % save_path)
        elif not fail and os.path.exists(save_path):
            os.unlink(save_path)
            print("    (Old %s failed packet capture deleted)" % save_path)
    get_current_test().on_finish.append(save_pcap)

def read_pcap():
    f = open("qemu.pcap", "rb")
    s = struct.Struct("=IHHiIII")
    hdr = s.unpack(f.read(s.size))
    assert_equal(hdr[0], 0xa1b2c3d4, "Bad pcap magic number")
    assert_equal((hdr[1], hdr[2]), (2, 4), "Bad pcap version number")
    s = struct.Struct("=iiII")
    while True:
        hdr_data = f.read(s.size)
        if not len(hdr_data):
            return
        hdr = s.unpack(hdr_data)
        yield bytes(f.read(hdr[2]))

def ascii_to_bytes(s):
    if bytes == str:
        # Python 2
        return s
    else:
        # Python 3
        return bytes(s, "ascii")

def hexdump(data):
    data = bytearray(data)
    buf = []
    for i in range(0, len(data), 16):
        chunk = data[i:i+16]
        hd = "".join("%02x" % b for b in chunk)
        hd = " ".join(hd[j:j+4] for j in range(0, len(hd), 4))
        ad = "".join(chr(c) if chr(c).isalnum() else "." for c in chunk)
        buf.append("%04x   %-40s  %s" % (i, hd, ad))
    return "\n".join(buf)

echo_port = QEMU.get_gdb_port() + 1
http_port = QEMU.get_gdb_port() + 2

socket.setdefaulttimeout(5)

#
# Basic tests
#

@test(5)
def test_testtime():
    r.user_test("testtime", make_args=["INIT_CFLAGS=-DTEST_NO_NS"])
    r.match(r'starting count down: 5 4 3 2 1 0 ')

@test(5)
def test_pci_attach():
    r.user_test("hello", make_args=["INIT_CFLAGS=-DTEST_NO_NS"])
    r.match(r'PCI function 00:03.0 \(8086:100e\) enabled')

#
# testoutput
#

def test_testoutput_helper(count):
    save_pcap_on_fail()

    maybe_unlink("qemu.pcap")
    r.user_test("net_testoutput",
                make_args=["NET_CFLAGS=-DTESTOUTPUT_COUNT=%d" % count])

    # Check the packet capture
    got = list(read_pcap())
    expect = [("packet %d/%d" % (i+1, count), ascii_to_bytes("Packet %02d" % i))
              for i in range(count)]
    match_packet_seq(got, expect)

@test(15, "testoutput [5 packets]")
def test_testoutput_5():
    test_testoutput_helper(5)

@test(10, "testoutput [100 packets]")
def test_testoutput_100():
    test_testoutput_helper(100)

end_part("A")

#
# testinput
#

def test_testinput_helper(count):
    save_pcap_on_fail()
    maybe_unlink("qemu.pcap")

    def send_packets():
        # Send 'count' UDP packets
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.connect(("127.0.0.1", echo_port))
        for i in range(count):
            sock.send(ascii_to_bytes("Packet %03d" % i))
    send_thread = threading.Thread(target=send_packets)

    # When we see the final packet, we can stop QEMU
    digits = "%03d" % (count-1)
    final = "input: 0030   203%s 3%s3%s" % tuple("%03d" % (count-1))

    r.user_test("net_testinput",
                call_on_line("Waiting for packets",
                              lambda _: send_thread.start()),
                stop_on_line(final))
    if send_thread.isAlive():
        send_thread.join()

    # Parse testinput hexdumps for received packets
    got = []
    for off, dump in re.findall("input: ([0-9a-f]{4})   ([0-9a-f ]*)\n",
                                r.qemu.output):
        if off == "0000" or not got:
            got.append(bytearray())
        dump = dump.replace(" ", "")
        for i in range(0, len(dump), 2):
            got[-1].append(int(dump[i:i+2], 16))
    got = map(bytes, got)

    # Get the packets QEMU actually received, since there's some
    # header information we can't easily guess.
    pcap = list(read_pcap())
    if len(pcap) != count + 2:
        raise RuntimeError("pcap contains only %d packets" % len(pcap))
    # Strip transmitted ARP request
    assert pcap.pop(0)[:6] == b"\xff" * 6, "First packet is not ARP request"
    # The E1000 pads packets out to a 60 byte frame
    pcap = [pkt.ljust(60, b"\0") for pkt in pcap]

    names = ["ARP reply"] + ["packet %d/%d" % (i+1, count)
                             for i in range(count)]
    match_packet_seq(got, zip(names, pcap))

@test(15, "testinput [5 packets]")
def test_testinput_5():
    test_testinput_helper(5)

@test(10, "testinput [100 packets]")
def test_testinput_100():
    test_testinput_helper(100)

#
# Servers
#

@test(15, "tcp echo server [echosrv]")
def test_echosrv():
    def ready(line):
        expect = ascii_to_bytes("%s: network server works" % time.time())
        got = bytearray()
        sock = socket.socket()
        try:
            sock.settimeout(5)
            sock.connect(("127.0.0.1", echo_port))
            sock.sendall(expect)
            while got != expect:
                data = sock.recv(4096)
                if not data:
                    break
                got += data
        except socket.error as e:
            got += "[Socket error: %s]" % e
        finally:
            sock.close()
        assert_equal(got, expect)
        raise TerminateTest

    save_pcap_on_fail()
    r.user_test("echosrv", call_on_line("bound", ready))
    r.match("bound", no=[".*panic"])

@test(0, "web server [httpd]")
def test_httpd():
    pass

def mk_test_httpd(url, expect_code, expect_data):
    fullurl = "http://localhost:%d%s" % (http_port, url)
    def test_httpd_test():
        def ready(line):
            try:
                # This uses the default socket timeout (5 seconds)
                res = urlopen(fullurl)
                got = "(Status 200)\n" + \
                    res.read().decode('utf-8')
            except HTTPError as e:
                got = "(Status %d)" % e.code
            except IOError as e:
                got = "(Error: %s)" % e
            expect = "(Status %d)" % expect_code
            if expect_data:
                expect += "\n" + expect_data
            assert_equal(got, expect)
            raise TerminateTest
        save_pcap_on_fail()
        r.user_test("httpd",
                    call_on_line('Waiting for http connections', ready))
        r.match('Waiting for http connections',
                no=[".*panic"])
    test_httpd_test.__name__ += url.replace("/", "-")
    return test(10, fullurl, parent=test_httpd)(test_httpd_test)
mk_test_httpd("/", 404, "")
mk_test_httpd("/index.html", 200, open("fs/index.html").read())
mk_test_httpd("/random_file.txt", 404, "")

end_part("B")

run_tests()
