#
# Copyright 2024 Geoffrey Ramseyer
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

AC_INIT([groundhog-model-checking], [0])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

OPT_FLAGS="-march=native -O3 -flto=auto"
DEBUG_FLAGS="-ggdb -O0 -march=native"

CXXFLAGS="-std=gnu++2b $OPT_FLAGS $CXXFLAGS"
CXX="g++"

AC_PROG_CXX
AC_PROG_CC
AC_LANG(C++)
AX_APPEND_COMPILE_FLAGS([-pthread])
AS_IF([test -z "${WFLAGS+set}"], AX_APPEND_COMPILE_FLAGS([-Wall], WFLAGS))


PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([libsodium], [libsodium])
PKG_CHECK_MODULES([tbb], [tbb])
CXXFLAGS="$CXXFLAGS $WFLAGS"

export CXXFLAGS
export CXX

AX_PKGCONFIG_SUBDIR(lib/xdrpp)
AC_MSG_CHECKING(for xdrc)
AS_IF([test -n "$XDRC"],,
	    [test -n "$xdrpp_INTERNAL" -a x"$cross_compiling" != xyes],
	    [XDRC='$(top_builddir)/lib/xdrpp/xdrc/xdrc$(EXEEXT)'],
	    [AC_PATH_PROG(XDRC, [xdrc])])
if test -z "$XDRC"; then
   AC_MSG_ERROR(Cannot find xdrc)
fi

AC_MSG_RESULT($XDRC)
AC_SUBST(XDRC)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT

