# Use the native MSYS2 UCRT64/PDCursesMod recipe when building on Windows.
# Explicit ROGUE_BUILD=unix/windows is useful for source packaging and tests.
ifeq ($(strip $(ROGUE_BUILD)),)
ifeq ($(MSYSTEM),UCRT64)
ROGUE_BUILD := windows
else ifeq ($(OS),Windows_NT)
ROGUE_BUILD := windows
else
ROGUE_BUILD := unix
endif
endif
ifeq ($(ROGUE_BUILD),windows)
include Makefile.win
else ifeq ($(ROGUE_BUILD),unix)
###############################################################################
#
# Makefile for rogue
#
# Rogue: Exploring the Dungeons of Doom
# Copyright (C) 1980-1983, 1985, 1999 Michael Toy, Ken Arnold and Glenn Wichman
# All rights reserved.
#       
# See the file LICENSE.TXT for full copyright and licensing information.
#
###############################################################################
        
###############################################################################
# Site configuration occurs beneath this comment.
# The checked-in Makefile can be used directly.  The optional ./configure
# helper writes config.mk, which is included below when present.
# Override variables on the make command line when local adjustments are needed.
###############################################################################

DISTNAME = rogue_dos_tribute_jec091
PROG ?=

-include config.mk

PROGRAM ?= $(if $(PROG),$(PROG),rogue)
FILEDIR ?=

ifneq ($(strip $(FILEDIR)),)
SAVEFILE ?= $(FILEDIR)/$(PROGRAM).save
SCOREFILE ?= $(FILEDIR)/$(PROGRAM).scr
LOCKFILE ?= $(FILEDIR)/$(PROGRAM).lck
else
SAVEFILE ?= $(PROGRAM).save
SCOREFILE ?= $(PROGRAM).scr
LOCKFILE ?= $(PROGRAM).lck
endif
MANPAGE_EN ?= build/man/en/$(PROGRAM).6
MANPAGE_JA ?= build/man/ja/$(PROGRAM).6
MANPAGES = $(MANPAGE_EN) $(MANPAGE_JA)
MANPAGE_EN_TEMPLATE ?= rogue.6.en.in
MANPAGE_JA_TEMPLATE ?= rogue.6.ja.in

O = o

CC ?= cc
CFLAGS ?= -g -O2
CPPFLAGS ?=
DEFAULT_LANGUAGE ?= 0
LANGUAGE_CPPFLAGS = -DDEFAULT_LANGUAGE=$(DEFAULT_LANGUAGE)
LDFLAGS ?=

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
CURSES_CFLAGS ?=
CURSES_LIBS ?= -lcurses
else
CURSES_CFLAGS ?= $(shell pkg-config --cflags ncursesw 2>/dev/null)
CURSES_LIBS ?= $(shell pkg-config --libs ncursesw 2>/dev/null)
ifeq ($(strip $(CURSES_LIBS)),)
CURSES_LIBS = -lncursesw
endif
endif
LIBS ?= $(CURSES_LIBS)

RM ?= rm -f

CLEAN_PROGRAMS = $(PROGRAM) $(PROGRAM).exe rogue rogue.exe
CLEAN_ARCHIVES = $(PROGRAM).tar $(PROGRAM).tar.gz $(PROGRAM).zip rogue.tar rogue.tar.gz rogue.zip
ifneq ($(strip $(PROG)),)
CLEAN_PROGRAMS += $(PROG) $(PROG).exe
CLEAN_ARCHIVES += $(PROG).tar $(PROG).tar.gz $(PROG).zip
endif

# Default save, score, and lock files follow PROGRAM unless overridden.
# Set FILEDIR to place all three files in the same directory.
# Individual SAVEFILE, SCOREFILE, and LOCKFILE settings still take precedence.
# PROG is accepted as a short alias used by the other Rogue ports.
# If both PROGRAM and PROG are specified, PROGRAM takes precedence.
# For example, make PROGRAM=rdt or make PROG=rdt builds rdt
# and uses rdt.save, rdt.scr, and rdt.lck by default.
# make PROGRAM=rdt FILEDIR=/var/games/rogue uses
# /var/games/rogue/rdt.save, rdt.scr, and rdt.lck.

# Keep the built-in flags even when the user passes CPPFLAGS on the
# command line.  Use EXTRA_CPPFLAGS for local additions such as -DMASTER.
USER_CPPFLAGS := $(CPPFLAGS)
override CPPFLAGS := -DHAVE_CONFIG_H \
                     -DDEFAULT_SAVEFILE=\"$(SAVEFILE)\" \
                     -DSCOREFILE=\"$(SCOREFILE)\" \
                     -DLOCKFILE=\"$(LOCKFILE)\" \
                     $(LANGUAGE_CPPFLAGS) \
                     $(CURSES_CFLAGS) $(USER_CPPFLAGS) $(EXTRA_CPPFLAGS)
ifeq ($(MASTER),1)
override CPPFLAGS += -DMASTER
endif

DISTFILE = $(DISTNAME)

INSTALL ?= install
MKDIR ?= mkdir -p
RMDIR ?= rmdir

DESTDIR ?=

prefix ?= /usr/local
exec_prefix ?= $(prefix)
datarootdir ?= $(prefix)/share
datadir ?= $(datarootdir)
bindir ?= $(exec_prefix)/bin
mandir ?= $(datarootdir)/man
docdir ?= $(datarootdir)/doc/$(PROGRAM)
man6dir ?= $(mandir)/man6
# By default, install Japanese manual pages in the common Linux location
# and in the locale-specific locations used by macOS's standard man(1).
# ./configure may replace this with a platform-specific subset.
JAMAN6DIRS ?= $(mandir)/ja/man6 $(mandir)/ja_JP.UTF-8/man6 $(mandir)/ja.UTF-8/man6

###############################################################################
# Site configuration occurs above this comment
# It should not be necessary to change anything below this comment
############################################################################### 

# Keep one canonical source manifest; object names and source distributions
# are derived from this list so they cannot silently get out of sync.
# Both build systems import the same portable source/header list.
include sources.mk
HDRS     = $(ROGUE_COMMON_HEADERS)
CFILES   = $(ROGUE_COMMON_CFILES)
OBJS     = $(CFILES:.c=.$(O))
DOCS     = README.md README_mac_linux_ja.md README_mac_linux_en.md rogue_ja.txt rogue_en.txt \
           rogue_ja.html rogue_en.html $(MANPAGES)
AFILES   = README.md README_mac_linux_ja.md README_mac_linux_en.md \
           README_windows_ja.md README_windows_en.md \
           rogue_ja.txt rogue_en.txt rogue_ja.html rogue_en.html \
           Makefile Makefile.win sources.mk configure \
           rogue.6.en.in rogue.6.ja.in .github/workflows/build.yml \
           windows/rogue_windows.c windows/rogue_windows.h \
           windows/rogue_windows_console.c windows/rogue_windows_console.h \
           windows/rogue-icon.rc windows/rogue.ico
MISC     = LICENSE.TXT
.PHONY: all japanese english original master wizard clean distclean stddocs dist dist.src install uninstall reinstall

all: $(PROGRAM) $(MANPAGES)

# Use the same recipe for root and jec/ sources.  Always pass -o so
# subdirectory objects are generated where Make expects them.
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

japanese:
	$(MAKE) clean
	$(MAKE) all DEFAULT_LANGUAGE=1 EXTRA_CPPFLAGS="$(EXTRA_CPPFLAGS)"

english:
	$(MAKE) clean
	$(MAKE) all DEFAULT_LANGUAGE=2 EXTRA_CPPFLAGS="$(EXTRA_CPPFLAGS)"

# Rogue DOS Tribute already uses DOS-style defaults; unlike other Rogue variants,
# "original" changes only the initial display language to English.  A
# command-line DEFAULT_LANGUAGE override is honored, as on Windows.
original:
	$(MAKE) clean
	$(MAKE) all DEFAULT_LANGUAGE=$(if $(filter command line,$(origin DEFAULT_LANGUAGE)),$(DEFAULT_LANGUAGE),2) EXTRA_CPPFLAGS="$(EXTRA_CPPFLAGS)"

master:
	$(MAKE) clean
	$(MAKE) all MASTER=1

# Backward-compatible target name; the compile-time macro is still MASTER.
wizard: master

$(PROGRAM): $(HDRS) $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(MANPAGE_EN): $(MANPAGE_EN_TEMPLATE)
	$(MKDIR) $(dir $@)
	sed -e 's|@PROGRAM@|$(PROGRAM)|g' \
	    -e 's|@SAVEFILE@|$(SAVEFILE)|g' \
	    -e 's|@SCOREFILE@|$(SCOREFILE)|g' \
	    $(MANPAGE_EN_TEMPLATE) > $@

$(MANPAGE_JA): $(MANPAGE_JA_TEMPLATE)
	$(MKDIR) $(dir $@)
	sed -e 's|@PROGRAM@|$(PROGRAM)|g' \
	    -e 's|@SAVEFILE@|$(SAVEFILE)|g' \
	    -e 's|@SCOREFILE@|$(SCOREFILE)|g' \
	    $(MANPAGE_JA_TEMPLATE) > $@
    
# Build cleanup must never remove save files, scores, or lock files.
# They may be in use by a running game or stored outside the source tree.
clean:
	$(RM) $(OBJS)
	$(RM) core a.exe a.out a.exe.stackdump $(CLEAN_PROGRAMS)
	$(RM) $(CLEAN_ARCHIVES)
	$(RM) $(MANPAGES) $(PROGRAM).6 rogue.6
	$(RM) -r build/man

# Remove files generated by ./configure.  clean intentionally keeps config.mk.
distclean: clean
	$(RM) config.mk config.log
    

stddocs: $(MANPAGES)
	@echo "$(MANPAGE_EN) generated from $(MANPAGE_EN_TEMPLATE)."
	@echo "$(MANPAGE_JA) generated from $(MANPAGE_JA_TEMPLATE)."

# Source packaging only copies listed source and documentation files;
# it must not clean an existing build or touch player data.
# Stage source files in a unique temporary directory rather than deleting
# $(DISTNAME) in the working tree: that name might be a real user directory.
# Both macOS (BSD tar) and Linux (GNU tar) accept tar -C and -czf.
dist.src:
	@set -eu; \
	    stage=$$(mktemp -d "$${TMPDIR:-/tmp}/rogue-dos-tribute-src.XXXXXXXX"); \
	    trap 'rm -rf "$$stage"' 0; \
	    trap 'exit 1' 1 2 3 15; \
	    $(MKDIR) "$$stage/$(DISTNAME)"; \
	    for file in $(CFILES) $(HDRS) $(MISC) $(AFILES); do \
	        case "$$file" in \
	            */*) $(MKDIR) "$$stage/$(DISTNAME)/$${file%/*}" ;; \
	        esac; \
	        cp "$$file" "$$stage/$(DISTNAME)/$$file"; \
	    done; \
	    chmod 0755 "$$stage/$(DISTNAME)/configure"; \
	    tar -czf "$(CURDIR)/$(DISTNAME)-src.tar.gz" -C "$$stage" "$(DISTNAME)"

# Run cleanup before starting the build, even if make -j is requested.
# The clean target preserves all save/score/lock files.
dist:
	$(MAKE) clean
	$(MAKE) $(PROGRAM) $(MANPAGES)
	tar cf $(DISTFILE).tar $(PROGRAM) LICENSE.TXT $(DOCS)
	gzip -f $(DISTFILE).tar

install: $(PROGRAM) $(MANPAGES)
	$(MKDIR) $(DESTDIR)$(bindir)
	$(MKDIR) $(DESTDIR)$(man6dir)
	$(MKDIR) $(DESTDIR)$(docdir)
ifneq ($(strip $(FILEDIR)),)
	$(MKDIR) $(DESTDIR)$(FILEDIR)
endif
	$(INSTALL) -m 0755 $(PROGRAM) $(DESTDIR)$(bindir)/$(PROGRAM)
	$(INSTALL) -m 0644 $(MANPAGE_EN) $(DESTDIR)$(man6dir)/$(PROGRAM).6
	@for d in $(JAMAN6DIRS); do \
	    $(MKDIR) $(DESTDIR)$$d; \
	    $(INSTALL) -m 0644 $(MANPAGE_JA) $(DESTDIR)$$d/$(PROGRAM).6; \
	done
	$(INSTALL) -m 0644 README_mac_linux_ja.md $(DESTDIR)$(docdir)/README_mac_linux_ja.md
	$(INSTALL) -m 0644 README_mac_linux_en.md $(DESTDIR)$(docdir)/README_mac_linux_en.md
	$(INSTALL) -m 0644 rogue_ja.txt $(DESTDIR)$(docdir)/rogue_ja.txt
	$(INSTALL) -m 0644 rogue_en.txt $(DESTDIR)$(docdir)/rogue_en.txt
	$(INSTALL) -m 0644 rogue_ja.html $(DESTDIR)$(docdir)/rogue_ja.html
	$(INSTALL) -m 0644 rogue_en.html $(DESTDIR)$(docdir)/rogue_en.html
	$(INSTALL) -m 0644 LICENSE.TXT $(DESTDIR)$(docdir)/LICENSE.TXT

uninstall:	
	-$(RM) $(DESTDIR)$(bindir)/$(PROGRAM)
	-$(RM) $(DESTDIR)$(man6dir)/$(PROGRAM).6
	@for d in $(JAMAN6DIRS); do \
	    $(RM) $(DESTDIR)$$d/$(PROGRAM).6; \
	    $(RMDIR) $(DESTDIR)$$d 2>/dev/null || true; \
	    parent=$${d%/man6}; \
	    if [ "$$parent" != "$$d" ]; then \
	        $(RMDIR) $(DESTDIR)$$parent 2>/dev/null || true; \
	    fi; \
	done
	-$(RM) $(DESTDIR)$(docdir)/README_mac_linux_ja.md
	-$(RM) $(DESTDIR)$(docdir)/README_mac_linux_en.md
	-$(RM) $(DESTDIR)$(docdir)/rogue_ja.txt
	-$(RM) $(DESTDIR)$(docdir)/rogue_en.txt
	-$(RM) $(DESTDIR)$(docdir)/rogue_ja.html
	-$(RM) $(DESTDIR)$(docdir)/rogue_en.html
	-$(RM) $(DESTDIR)$(docdir)/LICENSE.TXT
	-$(RMDIR) $(DESTDIR)$(docdir)
    
reinstall: uninstall install

else
$(error Unsupported ROGUE_BUILD=$(ROGUE_BUILD); use unix or windows)
endif
