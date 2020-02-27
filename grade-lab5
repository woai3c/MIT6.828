#!/usr/bin/env python

from gradelib import *

r = Runner(save("jos.out"),
           stop_breakpoint("readline"))

def matchtest(parent, name, *args, **kw):
    def do_test():
        r.match(*args, **kw)
    test(5, name, parent=parent)(do_test)

@test(5, "internal FS tests [fs/test.c]")
def test_fs():
    r.user_test("hello")
matchtest(test_fs, "fs i/o",
          "FS can do I/O")
matchtest(test_fs, "check_bc",
          "block cache is good")
matchtest(test_fs, "check_super",
          "superblock is good")
matchtest(test_fs, "check_bitmap",
          "bitmap is good")
matchtest(test_fs, "alloc_block",
          "alloc_block is good")
matchtest(test_fs, "file_open",
          "file_open is good")
matchtest(test_fs, "file_get_block",
          "file_get_block is good")
matchtest(test_fs, "file_flush/file_truncate/file rewrite",
          "file_flush is good",
          "file_truncate is good",
          "file rewrite is good")

@test(10, "testfile")
def test_testfile():
    r.user_test("testfile")
matchtest(test_testfile, "serve_open/file_stat/file_close",
          "serve_open is good",
          "file_stat is good",
          "file_close is good",
          "stale fileid is good")
matchtest(test_testfile, "file_read",
          "file_read is good")
matchtest(test_testfile, "file_write",
          "file_write is good")
matchtest(test_testfile, "file_read after file_write",
          "file_read after file_write is good")
matchtest(test_testfile, "open",
          "open is good")
matchtest(test_testfile, "large file",
          "large file is good")

@test(10, "spawn via spawnhello")
def test_spawn():
    r.user_test("spawnhello")
    r.match('i am parent environment 00001001',
            'hello, world',
            'i am environment 00001002',
            'No runnable environments in the system!')

@test(5, "Protection I/O space")
def test_faultio():
    r.user_test("spawnfaultio")
    r.match('TRAP')

@test(10, "PTE_SHARE [testpteshare]")
def test_pte_share():
    r.user_test("testpteshare")
    r.match('fork handles PTE_SHARE right',
            'spawn handles PTE_SHARE right')

@test(5, "PTE_SHARE [testfdsharing]")
def test_fd_share():
    r.user_test("testfdsharing")
    r.match('read in child succeeded',
            'read in parent succeeded')

@test(10, "start the shell [icode]")
def test_icode():
    r.user_test("icode")
    r.match('icode: read /motd',
            'This is /motd, the message of the day.',
            'icode: spawn /init',
            'init: running',
            'init: data seems okay',
            'icode: exiting',
            'init: bss seems okay',
            "init: args: 'init' 'initarg1' 'initarg2'",
            'init: running sh',
            '\$ ')

@test(15)
def test_testshell():
    r.user_test("testshell", timeout=60)
    r.match("shell ran correctly")

def gen_primes(n):
    rest = range(2, n)
    while rest:
        yield rest[0]
        rest = [n for n in rest if n % rest[0]]

@test(10)
def test_primespipe():
    r.user_test("primespipe", stop_on_line("[0-9]{4}$"), timeout=120)
    primes = set(gen_primes(1000))
    nonprimes = set(range(1000)) - primes
    r.match(no=["%d$" % np for np in nonprimes],
            *["%d$" % p for p in primes])

run_tests()
