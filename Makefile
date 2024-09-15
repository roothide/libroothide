ifneq ($(shell type xcrun >/dev/null 2>&1 && echo 1),1)
$(error "please use macOS to build the library for arm64e new abi")
endif

ifeq ($(shell type xcrun >/dev/null 2>&1 && echo 1),1)
TAPI ?= xcrun tapi
else ifeq ($(shell type tapi >/dev/null 2>&1 && echo 1),1)
TAPI ?= tapi
else
$(error "tapi not found")
endif


ALL := roothideinit.dylib libroothide.dylib libvroot.h libvroot.dylib libvrootapi.dylib updatelink symredirect jbrand jbroot rootfs

LIB_ARCHS	= -arch arm64 -arch arm64e

CFLAGS    += -fvisibility=hidden
CXXFLAGS  += -fvisibility=hidden
CPPFLAGS  += -fvisibility=hidden

LDFLAGS   += -L./

.DEFAULT_GOAL := all

roothideinit.dylib: init.c common.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIB_ARCHS) -dynamiclib -install_name @loader_path/.jbroot/usr/lib/roothideinit.dylib -o $@ $^

jbroot.cpp.o: jbroot.cpp
	$(CXX) $(CXXFLAGS) $(LIB_ARCHS) -stdlib=libc++ -c $< -o $@

libroothide.dylib: roothideinit.dylib jbroot.cpp.o jbroot.c jbroot.m cache.c common.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIB_ARCHS) -fobjc-arc -lc++ -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libroothide.dylib -o $@ $^
	$(TAPI) stubify $@
	mkdir -p tbdfiles
	mv libroothide.tbd ./tbdfiles/

libvroot.h: vroot.h
	$(CPP) $(CPPFLAGS) -DVROOT_API_ALL $< > $@

vroot.cpp.o: vroot.cpp
	$(CXX) $(CXXFLAGS) $(LIB_ARCHS) -stdlib=libc++ -c $< -o $@

libvroot.dylib: libroothide.dylib vroot.cpp.o vroot.c vroot_mktemp.c vroot_rootfs.c vroot_exec.c vroot_dlfcn.c common.c debug.m
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIB_ARCHS) -fobjc-arc -lc++ -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libvroot.dylib -o $@ $^
	$(TAPI) stubify $@
	mkdir -p tbdfiles
	mv libvroot.tbd ./tbdfiles/

libvrootapi.dylib: libvroot.dylib vrootapi.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LIB_ARCHS) -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libvrootapi.dylib -o $@ $^
	$(TAPI) stubify $@
	mkdir -p tbdfiles
	mv libvrootapi.tbd ./tbdfiles/

updatelink: updatelink.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

symredirect: symredirect.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -std=c++11 -stdlib=libc++ -o $@ $^

jbrand: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

jbroot: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

rootfs: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^


all: $(ALL)
	mkdir devkit
	mkdir devkit/roothide
	cp ./roothide-xcode.h ./devkit/roothide.h
	cp ./roothide.h ./devkit/roothide/
	cp ./stub.h ./devkit/roothide/
	cp ./tbdfiles/libroothide.tbd ./devkit/roothide/
	cp ./module.modulemap ./devkit/roothide/
	zip -r devkit.zip ./devkit

clean:
	rm -f $(ALL) *.tbd *.o ./devkit.zip
	rm -rf ./devkit ./tbdfiles

.PHONY: all clean
