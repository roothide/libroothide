CC      ?= xcrun -sdk iphoneos clang -arch arm64
CPP		?= clang -E
CFLAGS  ?= -miphoneos-version-min=15.0 -isysroot $(shell xcrun --sdk iphoneos --show-sdk-path)

ALL := roothideinit.dylib libroothide.dylib libvroot.h libvroot.dylib libvrootapi.dylib updatelink symredirect jbrand jbroot rootfs

LDFLAGS += -L./
CFLAGS +=  -fvisibility=hidden 
CCFLAGS += -fvisibility=hidden

roothideinit.dylib: init.c 
	$(CC) -arch arm64 -arch arm64e $(CFLAGS) $(LDFLAGS) -dynamiclib -install_name @loader_path/.jbroot/usr/lib/roothideinit.dylib -o $@ $^

libroothide.dylib: roothideinit.dylib jbroot.c jbroot.cpp jbroot.m cache.c common.c
	$(CC) -fobjc-arc -arch arm64 -arch arm64e $(CFLAGS) $(LDFLAGS) -lstdc++ -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libroothide.dylib -o $@ $^
	xcrun tapi stubify $@

libvroot.h: vroot.h
	$(CPP) $< > $@

libvroot.dylib: libroothide.dylib vroot.c vroot_mktemp.c vroot.cpp vroot_rootfs.c vroot_exec.c vroot_dlfcn.c common.c
	$(CC) -arch arm64 -arch arm64e $(CFLAGS) $(LDFLAGS) -lstdc++ -lroothide -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libvroot.dylib -o $@ $^
	xcrun tapi stubify $@

libvrootapi.dylib: libvroot.dylib vrootapi.c
	$(CC) -arch arm64 -arch arm64e $(CFLAGS) $(LDFLAGS) -lvroot -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libvrootapi.dylib -o $@ $^
	xcrun tapi stubify $@

updatelink: updatelink.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -lroothide -o $@ $^

symredirect: symredirect.cpp
	xcrun -sdk macosx clang -mmacosx-version-min=10.7 -std=c++11 -lstdc++ -o $@ $^

jbrand: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -lroothide -o $@ $^

jbroot: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -lroothide -o $@ $^

rootfs: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -lroothide -o $@ $^


all: $(ALL)
	mkdir devkit
	mkdir devkit/roothide
	cp ./roothide-xcode.h ./devkit/roothide.h
	cp ./roothide.h ./devkit/roothide/
	cp ./stub.h ./devkit/roothide/
	cp ./libroothide.tbd ./devkit/roothide/
	cp ./module.modulemap ./devkit/roothide/
	zip -r devkit.zip ./devkit

clean:
	rm -rf $(ALL) *.tbd
	rm -rf ./devkit.zip
	rm -rf ./devkit

.PHONY: all clean
