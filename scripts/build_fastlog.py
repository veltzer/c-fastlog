#!/usr/bin/env python

""" Build the fastlog shared library and its test programs, reproducing the
Makefile: compile every src/**/*.c with -fpic, link them into
out/lib/libfastlog.so, then compile each test/*.c against that library. The
object files go to a temporary directory, so only the declared outputs land in
the tree. File arguments are ignored -- the source layout below defines the
build. """

import glob
import os
import subprocess
import sys
import tempfile

CC = "gcc"
SRCDIR = "src"
OUT = "out"
LIBNAME = "fastlog"
BASE_FLAGS = ["-O2", "-fpic", "-Wall", "-Wextra", "-Werror", "-pedantic", "-std=gnu99"]
CFLAGS = [*BASE_FLAGS, "-I" + SRCDIR, "-Itest"]
LIB = os.path.join(OUT, "lib", "lib" + LIBNAME + ".so")


def run(cmd: list[str]) -> None:
    """ Run a command, exiting the process on the first failure. """
    ret = subprocess.call(cmd)
    if ret != 0:
        sys.exit(ret)


def build_library(objdir: str) -> None:
    """ Compile every library source into objdir and link them into the shared
    object. """
    objs = []
    for src in sorted(glob.glob(os.path.join(SRCDIR, "**", "*.c"), recursive=True)):
        obj = os.path.join(objdir, os.path.splitext(src)[0] + ".o")
        os.makedirs(os.path.dirname(obj), exist_ok=True)
        run([CC, "-c", *CFLAGS, "-o", obj, src])
        objs.append(obj)
    os.makedirs(os.path.dirname(LIB), exist_ok=True)
    run([CC, "-shared", "-fpic", "-o", LIB, *objs])


def build_tests() -> None:
    """ Compile each test program against the shared library. """
    binld = ["-L" + os.path.join(OUT, "lib"), "-l" + LIBNAME, "-lpthread"]
    os.makedirs(os.path.join(OUT, "bin"), exist_ok=True)
    for src in sorted(glob.glob(os.path.join("test", "*.c"))):
        exe = os.path.join(OUT, "bin", os.path.splitext(os.path.basename(src))[0])
        run([CC, *CFLAGS, "-o", exe, src, *binld])


def main() -> None:
    """ main entry point """
    with tempfile.TemporaryDirectory() as objdir:
        build_library(objdir)
    build_tests()


if __name__ == "__main__":
    main()
