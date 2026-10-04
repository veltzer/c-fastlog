# TOFIX

Findings from a code scan on 2026-10-04.

## High

- `src/fastlog.c:149` - the `error:` label in `fastlog_init` returns `0`, so a failed `malloc` or `mlock` reports success to the caller. Return `-1` (or the errno) on that path.
- `src/fastlog.c:119` - when `conf == NULL` (the path every test uses), `fastlog_init` writes the buffer pointer into the stack-local `config` and returns, so the buffer is leaked and no state survives the call; there is no global/static state for `fastlog_log` or `fastlog_close` to use. Keep the runtime state in a file-static (or caller-owned) struct.
- `src/fastlog.h:54` - `fastlog_close()` is declared without a prototype, while `src/fastlog.c:176` defines it as `fastlog_close(fastlog_config* conf)`. The tests call it with no argument (`test/fastlog_test_basic.c:8`, `test/fastlog_test_crash.c:34`, `test/fastlog_test_speed.c:142`), so `free(conf->buffer)` runs on a garbage pointer (undefined behaviour). Give the header a real prototype (`int fastlog_close(void);`) and make the definition match it.
- `src/fastlog.c:196` - `fastlog_log` is a no-op: everything is behind `#ifdef DO_WRITE` (never defined, line 11), and that code would not compile anyway because it references an undeclared `conf`. `fastlog_worker` (line 53) is entirely commented out too. So the library logs nothing, and `test/fastlog_test_speed.c` times an empty function. Implement the ring-buffer write and worker, or state plainly in the README/description that the library is a non-functional prototype.

## Medium

- `src/fastlog.h:31` - `fastlog_config_set_mlock`/`get_mlock`, `set/get_msgnum`, `set/get_msgsize` (lines 31-38), `fastlog_sendwake` (line 81) and `fastlog_sync` (line 88) are declared in the public header but never defined, so any caller hits a link error. Implement them or remove the declarations.
- `src/fastlog.c:156` - `pthread_attr_init`/`pthread_create` (line 164) return an error code and do not set `errno`, so `perror` prints an unrelated message; use `strerror(res)`. The return values of `pthread_attr_setinheritsched`/`setschedpolicy`/`setschedparam` (lines 159-163) are ignored, `pthread_attr_destroy` is never called, and `conf->stop=false` is set after the thread already started (line 169), which races with the worker. `fastlog_thread_config` is private to the .c file, so the non-static `fastlog_thread_*` functions cannot be called by users at all.
- `config/project.lua:23` - `LICENSE_TYPE = "GPLV3"`, but `LICENSE` is MIT and `COPYRIGHT` says LGPL-2.1. Pick one license and make all three agree (delete `COPYRIGHT` if MIT is the answer).
- `rsconstruct.toml:32` - `[processor.ruff]` and `[processor.mypy]` (line 36) list `src` (C only) and `config` (Lua only) in `src_dirs`; the only Python is in `scripts`. Use `src_dirs = ["scripts"]`.
- `rsconstruct.toml:26` - `[processor.tera]` lists the same three Lua files in both `dep_auto` (line 20) and `dep_inputs`; the comment on line 17 says `dep_inputs` is only for repos still on `config/*.py`. Drop the `dep_inputs` block.
- `rsconstruct.toml:52` - the test programs are built but never run, so CI cannot catch the crash in `fastlog_close` above. Run `out/bin/fastlog_test_basic` as part of the build once it works.

## Low

- `scripts/build_fastlog.py:17` - `-Wno-dangling-pointer` is no longer needed: `src/fastlog.c` and all three `test/*.c` compile cleanly with `-Wall -Werror` without it. Remove it so the warning can catch real bugs.
- `pyproject.toml:10` - `pytest` is in the dev group but there are no Python tests; drop it (and `uv lock`).
- `external/syslog-async-0.2.tar.gz` - vendored tarball that nothing builds or references (only a URL in `doc/DESIGN.txt:62`); delete it.
- `support/doxygen.cfg` - not referenced by `rsconstruct.toml` or any script; wire it into the build or delete it.
- `doc/TODO.txt:7` - stale items refer to retired tooling (templar line 7, pdmt/mako line 25, GitHub wiki line 24); prune them.
- `README.md:6` - the generated README shows only the one-line description; `DESCRIPTION_LONG` in `config/project.lua:3` (and its pointer to `doc/DESIGN.txt`) never reaches it. Add a `tera.snippets/main.md.tera` with the long description and usage.
