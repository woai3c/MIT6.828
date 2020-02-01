#!/usr/bin/env python

import re
from gradelib import *

r = Runner(save("jos.out"),
           stop_breakpoint("readline"))

@test(0, "running JOS")
def test_jos():
    r.run_qemu()

@test(20, parent=test_jos)
def test_printf():
    r.match("6828 decimal is 15254 octal!")

BACKTRACE_RE = r"^ *ebp +f01[0-9a-z]{5} +eip +f0100[0-9a-z]{3} +args +([0-9a-z]+)"

@test(10, parent=test_jos)
def test_backtrace_count():
    matches = re.findall(BACKTRACE_RE, r.qemu.output, re.MULTILINE)
    assert_equal(len(matches), 8)

@test(10, parent=test_jos)
def test_backtrace_arguments():
    matches = re.findall(BACKTRACE_RE, r.qemu.output, re.MULTILINE)
    assert_equal("\n".join(matches[:7]),
                 "\n".join("%08x" % n for n in [0,0,1,2,3,4,5]))

@test(5, parent=test_jos)
def test_backtrace_symbols():
    matches = re.findall(r"kern/init.c:[0-9]+: +([^+]*)\+", r.qemu.output)
    assert_equal("\n".join(matches[:7]),
                 "\n".join(["test_backtrace"] * 6 + ["i386_init"]))

@test(5, parent=test_jos)
def test_backtrace_lines():
    matches = re.findall(r"([^ ]*init.c:([0-9]+):) +test_backtrace\+", r.qemu.output)
    assert matches, "No line numbers"
    if any(int(m[1]) < 5 or int(m[1]) > 50 for m in matches):
        assert_equal("\n".join(m[0] for m in matches),
                     "Line numbers between 5 and 50")

run_tests()
