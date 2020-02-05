#!/usr/bin/env python

from gradelib import *

r = Runner(save("jos.out"),
           stop_breakpoint("readline"))

@test(0, "running JOS")
def test_jos():
    r.run_qemu()

@test(20, "Physical page allocator", parent=test_jos)
def test_check_page_alloc():
    r.match(r"check_page_alloc\(\) succeeded!")

@test(20, "Page management", parent=test_jos)
def test_check_page():
    r.match(r"check_page\(\) succeeded!")

@test(20, "Kernel page directory", parent=test_jos)
def test_check_kern_pgdir():
    r.match(r"check_kern_pgdir\(\) succeeded!")

@test(10, "Page management 2", parent=test_jos)
def test_check_page_installed_pgdir():
    r.match(r"check_page_installed_pgdir\(\) succeeded!")

run_tests()
