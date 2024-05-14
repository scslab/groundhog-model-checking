#
# Copyright 2023 Geoffrey Ramseyer
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

CLOCAL_AMFLAGS = -I m4

SUBDIRS = $(PKGCONFIG_SUBDIRS)

CLEANFILES = *~ .*~

include $(srcdir)/lib/utility/Makefile.am.fragment

X_FILES = \
	xdr/storage.x \
	xdr/types.x

XH_FILES = $(X_FILES:.x=.h)

#building x files
$(XH_FILES) : $(XDRC)

SUFFIXES = .x .h

.x.h:
	$(XDRC)  -hh -o $@ $<

CONFIG_SRCS = \
	config/yield_config.cc

CRYPTO_SRCS = \
	crypto/crypto_utils.cc

HASH_SET_SRCS = \
	hash_set/atomic_set.cc 

OBJECT_SRCS = \
	object/comparators.cc \
	object/make_delta.cc \
	object/object_defaults.cc \
	object/revertable_object.cc

THREADLOCAL_SRCS = \
	threadlocal/threadlocal_context.cc

SRCS = \
	$(CONFIG_SRCS) \
	$(CRYPTO_SRCS) \
	$(HASH_SET_SRCS) \
	$(OBJECT_SRCS) \
	$(THREADLOCAL_SRCS) \
	$(utility_CCS_NOLDMB)

libptimc/build/libptimc.a: libptimc/libptimc.c libptimc/libimc/libimc_linux.c libptimc/libimc/master.c libptimc/libimc/worker.c
	cd libptimc && make

exhaust-libptimc/build/libptimc.a: exhaust-libptimc/libptimc.c exhaust-libptimc/libimc/libimc_linux.c exhaust-libptimc/libimc/master.c exhaust-libptimc/libimc/worker.c
	cd exhaust-libptimc && make


AM_CPPFLAGS = \
	$(xdrpp_CFLAGS) \
	$(libsodium_CFLAGS) \
	$(utility_CFLAGS) \
	-I./exhaust-libptimc/libimc/ \
	-I./exhaust-libptimc

LDADD = \
	$(xdrpp_LIBS) \
	$(libsodium_LIBS) \
	$(utility_LIBS) \
	./exhaust-libptimc/build/libptimc.a

$(SRCS:.cc=.o) : $(XH_FILES)

bin_PROGRAMS = \
	test_uint128_simple \
	test_uint128_buggy \
	test_hs_simple \
	test_hs_ins_erase \
	test_hs_tombstone_bug \
	test_hs_tombstone_bug_shorter \
	test_rbo_basic_empty \
	test_rbo_basic_empty_sametc \
	test_rbo_basic_empty_sametc2

test_uint128_simple_SOURCES = main/test_uint128_simple.cc $(SRCS)
test_uint128_buggy_SOURCES = main/test_uint128_buggy.cc $(SRCS)
test_hs_simple_SOURCES = main/test_hs_simple.cc $(SRCS)
test_hs_ins_erase_SOURCES = main/test_hs_ins_erase.cc $(SRCS)
test_hs_tombstone_bug_SOURCES = main/test_hs_tombstone_bug.cc $(SRCS)
test_hs_tombstone_bug_shorter_SOURCES = main/test_hs_tombstone_bug_shorter.cc $(SRCS)
test_rbo_basic_empty_SOURCES = main/test_rbo_basic_empty.cc $(SRCS)
test_rbo_basic_empty_sametc_SOURCES = main/test_rbo_basic_empty_sametc.cc $(SRCS)
test_rbo_basic_empty_sametc2_SOURCES = main/test_rbo_basic_empty_sametc2.cc $(SRCS)


clean-local:
	$(RM) -r libptimc/build/

