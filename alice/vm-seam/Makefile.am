SUBDIRS = alice debian

INCLUDES = -I@top_srcdir@

seamlib_SEAMLIBRARIES = alice.dll

alice_dll_SOURCES = \
	AliceMain.cc

alice_dll_LIBADD = \
	alice/lib/system/libsystem.a \
	alice/lib/utility/libutility.a \
	alice/lib/distribution/libdistribution.a \
	alice/libalicebase.a \
	alice/primitives/libprimitives.a

bin_SCRIPTS = \
	bin/alicerun \
	bin/alicetool \
	bin/aliceremote \
	bin/alice-config

include_HEADERS = \
	Alice.hh

EXTRA_DIST = \
	bin/alicerun.in \
	bin/alicetool.in \
	bin/aliceremote \
	bin/alice-config.in \
	alice/bytecode_disassembler_body.hh \
	bin/windows/Makefile \
	bin/windows/alicex.cc
