#!/usr/bin/env python

from gradelib import *

r = Runner(save("jos.out"),
           stop_breakpoint("readline"))

@test(10)
def test_divzero():
    r.user_test("divzero")
    r.match('Incoming TRAP frame at 0xefffff..',
            'TRAP frame at 0xf.......',
            '  trap 0x00000000 Divide error',
            '  eip  0x008.....',
            '  ss   0x----0023',
            '.00001000. free env 00001000',
            no=['1/0 is ........!'])

@test(10)
def test_softint():
    r.user_test("softint")
    r.match('Welcome to the JOS kernel monitor!',
            'Incoming TRAP frame at 0xefffffbc',
            'TRAP frame at 0xf.......',
            '  trap 0x0000000d General Protection',
            '  eip  0x008.....',
            '  ss   0x----0023',
            '.00001000. free env 0000100')

@test(10)
def test_badsegment():
    r.user_test("badsegment")
    r.match('Incoming TRAP frame at 0xefffffbc',
            'TRAP frame at 0xf.......',
            '  trap 0x0000000d General Protection',
            '  err  0x00000028',
            '  eip  0x008.....',
            '  ss   0x----0023',
            '.00001000. free env 0000100')

end_part("A")

@test(5)
def test_faultread():
    r.user_test("faultread")
    r.match('.00001000. user fault va 00000000 ip 008.....',
            'Incoming TRAP frame at 0xefffffbc',
            'TRAP frame at 0xf.......',
            '  trap 0x0000000e Page Fault',
            '  err  0x00000004.*',
            '.00001000. free env 0000100',
            no=['I read ........ from location 0!'])

@test(5)
def test_faultreadkernel():
    r.user_test("faultreadkernel")
    r.match('.00001000. user fault va f0100000 ip 008.....',
            'Incoming TRAP frame at 0xefffffbc',
            'TRAP frame at 0xf.......',
            '  trap 0x0000000e Page Fault',
            '  err  0x00000005.*',
            '.00001000. free env 00001000',
            no=['I read ........ from location 0xf0100000!'])

@test(5)
def test_faultwrite():
    r.user_test("faultwrite")
    r.match('.00001000. user fault va 00000000 ip 008.....',
            'Incoming TRAP frame at 0xefffffbc',
            'TRAP frame at 0xf.......',
            '  trap 0x0000000e Page Fault',
            '  err  0x00000006.*',
            '.00001000. free env 0000100')

@test(5)
def test_faultwritekernel():
    r.user_test("faultwritekernel")
    r.match('.00001000. user fault va f0100000 ip 008.....',
            'Incoming TRAP frame at 0xefffffbc',
            'TRAP frame at 0xf.......',
            '  trap 0x0000000e Page Fault',
            '  err  0x00000007.*',
            '.00001000. free env 0000100')

@test(5)
def test_breakpoint():
    r.user_test("breakpoint")
    r.match('Welcome to the JOS kernel monitor!',
            'Incoming TRAP frame at 0xefffffbc',
            'TRAP frame at 0xf.......',
            '  trap 0x00000003 Breakpoint',
            '  eip  0x008.....',
            '  ss   0x----0023',
            no=['.00001000. free env 00001000'])

@test(5)
def test_testbss():
    r.user_test("testbss")
    r.match('Making sure bss works right...',
            'Yes, good.  Now doing a wild write off the end...',
            '.00001000. user fault va 00c..... ip 008.....',
            '.00001000. free env 0000100')

@test(5)
def test_hello():
    r.user_test("hello")
    r.match('.00000000. new env 00001000',
            'hello, world',
            'i am environment 00001000',
            '.00001000. exiting gracefully',
            '.00001000. free env 00001000',
            'Destroyed the only environment - nothing more to do!')

@test(5)
def test_buggyhello():
    r.user_test("buggyhello")
    r.match('.00001000. user_mem_check assertion failure for va 00000001',
            '.00001000. free env 00001000')

@test(5)
def test_buggyhello2():
    r.user_test("buggyhello2")
    r.match('.00001000. user_mem_check assertion failure for va 0....000',
            '.00001000. free env 00001000',
            no=['hello, world'])

@test(5)
def test_evilhello():
    r.user_test("evilhello")
    r.match('.00001000. user_mem_check assertion failure for va f0100...',
            '.00001000. free env 00001000')

end_part("B")

run_tests()
