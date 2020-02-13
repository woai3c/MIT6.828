#!/usr/bin/env python

import re
from gradelib import *

r = Runner(save("jos.out"),
           stop_breakpoint("readline"))

def E(s, trim=False):
    """Expand $En in s to the environment ID of the n'th user
    environment."""

    tmpl = "%x" if trim else "%08x"
    return re.sub(r"\$E([0-9]+)",
                  lambda m: tmpl % (0x1000 + int(m.group(1))-1), s)

@test(5)
def test_dumbfork():
    r.user_test("dumbfork")
    r.match(E(".00000000. new env $E1"),
            E(".$E1. new env $E2"),
            "0: I am the parent.",
            "9: I am the parent.",
            "0: I am the child.",
            "9: I am the child.",
            "19: I am the child.",
            E(".$E1. exiting gracefully"),
            E(".$E1. free env $E1"),
            E(".$E2. exiting gracefully"),
            E(".$E2. free env $E2"))

end_part("A")

@test(5)
def test_faultread():
    r.user_test("faultread")
    r.match(E(".$E1. user fault va 00000000 ip 008....."),
            "TRAP frame at 0xf....... from CPU .",
            "  trap 0x0000000e Page Fault",
            "  err  0x00000004.*",
            E(".$E1. free env $E1"),
            no=["I read ........ from location 0."])

@test(5)
def test_faultwrite():
    r.user_test("faultwrite")
    r.match(E(".$E1. user fault va 00000000 ip 008....."),
            "TRAP frame at 0xf....... from CPU .",
            "  trap 0x0000000e Page Fault",
            "  err  0x00000006.*",
            E(".$E1. free env $E1"))

@test(5)
def test_faultdie():
    r.user_test("faultdie")
    r.match("i faulted at va deadbeef, err 6",
            E(".$E1. exiting gracefully"),
            E(".$E1. free env $E1"))

@test(5)
def test_faultregs():
    r.user_test("faultregs")
    r.match("Registers in UTrapframe OK",
            "Registers after page-fault OK",
            no=["Registers in UTrapframe MISMATCH",
                "Registers after page-fault MISMATCH"])

@test(5)
def test_faultalloc():
    r.user_test("faultalloc")
    r.match("fault deadbeef",
            "this string was faulted in at deadbeef",
            "fault cafebffe",
            "fault cafec000",
            "this string was faulted in at cafebffe",
            E(".$E1. exiting gracefully"),
            E(".$E1. free env $E1"))

@test(5)
def test_faultallocbad():
    r.user_test("faultallocbad")
    r.match(E(".$E1. user_mem_check assertion failure for va deadbeef"),
            E(".$E1. free env $E1"))

@test(5)
def test_faultnostack():
    r.user_test("faultnostack")
    r.match(E(".$E1. user_mem_check assertion failure for va eebfff.."),
            E(".$E1. free env $E1"))

@test(5)
def test_faultbadhandler():
    r.user_test("faultbadhandler")
    r.match(E(".$E1. user_mem_check assertion failure for va (deadb|eebfe)..."),
            E(".$E1. free env $E1"))

@test(5)
def test_faultevilhandler():
    r.user_test("faultevilhandler")
    r.match(E(".$E1. user_mem_check assertion failure for va (f0100|eebfe)..."),
            E(".$E1. free env $E1"))

@test(5)
def test_forktree():
    r.user_test("forktree")
    r.match("....: I am .0.",
            "....: I am .1.",
            "....: I am .000.",
            "....: I am .100.",
            "....: I am .110.",
            "....: I am .111.",
            "....: I am .011.",
            "....: I am .001.",
            E(".$E1. exiting gracefully"),
            E(".$E2. exiting gracefully"),
            ".0000200.. exiting gracefully",
            ".0000200.. free env 0000200.")

end_part("B")

@test(5)
def test_spin():
    r.user_test("spin")
    r.match(E(".00000000. new env $E1"),
            "I am the parent.  Forking the child...",
            E(".$E1. new env $E2"),
            "I am the parent.  Running the child...",
            "I am the child.  Spinning...",
            "I am the parent.  Killing the child...",
            E(".$E1. destroying $E2"),
            E(".$E1. free env $E2"),
            E(".$E1. exiting gracefully"),
            E(".$E1. free env $E1"))

@test(5)
def test_stresssched():
    r.user_test("stresssched", make_args=["CPUS=4"])
    r.match(".000010... stresssched on CPU 0",
            ".000010... stresssched on CPU 1",
            ".000010... stresssched on CPU 2",
            ".000010... stresssched on CPU 3",
            no=[".*ran on two CPUs at once"])

@test(5)
def test_sendpage():
    r.user_test("sendpage", make_args=["CPUS=2"])
    r.match(".00000000. new env 00001000",
            E(".00000000. new env $E1"),
            E(".$E1. new env $E2"),
            E("$E1 got message: hello child environment! how are you?", trim=True),
            E("child received correct message", trim=True),
            E("$E2 got message: hello parent environment! I'm good", trim=True),
            E("parent received correct message", trim=True),
            E(".$E1. exiting gracefully"),
            E(".$E1. free env $E1"),
            E(".$E2. exiting gracefully"),
            E(".$E2. free env $E2"))

@test(5)
def test_pingpong():
    r.user_test("pingpong", make_args=["CPUS=4"])
    r.match(E(".00000000. new env $E1"),
            E(".$E1. new env $E2"),
            E("send 0 from $E1 to $E2", trim=True),
            E("$E2 got 0 from $E1", trim=True),
            E("$E1 got 1 from $E2", trim=True),
            E("$E2 got 8 from $E1", trim=True),
            E("$E1 got 9 from $E2", trim=True),
            E("$E2 got 10 from $E1", trim=True),
            E(".$E1. exiting gracefully"),
            E(".$E1. free env $E1"),
            E(".$E2. exiting gracefully"),
            E(".$E2. free env $E2"))

@test(5)
def test_primes():
    r.user_test("primes", stop_on_line("CPU .: 1877"), stop_on_line(".*panic"),
                make_args=["CPUS=4"], timeout=60)
    r.match(E(".00000000. new env $E1"),
            E(".$E1. new env $E2"),
            E("CPU .: 2 .$E2. new env $E3"),
            E("CPU .: 3 .$E3. new env $E4"),
            E("CPU .: 5 .$E4. new env $E5"),
            E("CPU .: 7 .$E5. new env $E6"),
            E("CPU .: 11 .$E6. new env $E7"),
            E("CPU .: 1877 .$E289. new env $E290"))

end_part("C")

run_tests()
