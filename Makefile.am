#
# Copyright (c) 2015-2020 Hirochika Asai <asai@jar.jp>
# All rights reserved.
#

bin_PROGRAMS = palmtrie_test_basic palmtrie_test_acl palmtrie_eval_lpm palmtrie_eval_acl

EXTRA_DIST = tests/linx-rib.20141217.0000-p46.txt

lib_LTLIBRARIES = libpalmtrie.la
libpalmtrie_la_SOURCES = palmtrie.c palmtrie.h sl.c tpt.c mtpt.c popmtpt.c

palmtrie_test_basic_SOURCES = tests/basic.c
palmtrie_test_basic_LDFLAGS = -static $(top_builddir)/libpalmtrie.la
palmtrie_test_basic_DEPENDENCIES = libpalmtrie.la

palmtrie_test_acl_SOURCES = tests/acl.c
palmtrie_test_acl_LDFLAGS = -static $(top_builddir)/libpalmtrie.la
palmtrie_test_acl_DEPENDENCIES = libpalmtrie.la

palmtrie_eval_lpm_SOURCES = tests/eval_lpm.c
palmtrie_eval_lpm_LDFLAGS = -static $(top_builddir)/libpalmtrie.la
palmtrie_eval_lpm_DEPENDENCIES = libpalmtrie.la

palmtrie_eval_acl_SOURCES = tests/eval_acl.c
palmtrie_eval_acl_LDFLAGS = -static $(top_builddir)/libpalmtrie.la
palmtrie_eval_acl_DEPENDENCIES = libpalmtrie.la

CLEANFILES = *~

test: all
	@echo "Testing all..."
	$(top_builddir)/palmtrie_test_basic

microperf: all
	@echo "Measuring performance..."
	$(top_builddir)/palmtrie_test_basic microperf

perf: all
	@echo "Testing LPM..."
	$(top_builddir)/palmtrie_test_basic microperf
	@echo "Testing ACL..."
	$(top_builddir)/palmtrie_test_acl microperf rossperf1 rossperf2 rossperf3

