CC      ?= xcrun -sdk macosx clang
CPP		?= clang -E
CFLAGS  ?= -mmacosx-version-min=10.7

ALL := libroothide.dylib libvroot.h libvroot.dylib libvrootapi.dylib updatelink symredirect jbrand jbroot rootfs

ifneq (,$(findstring iphoneos,$(CC) $(CFLAGS)))
CFLAGS += -arch arm64 -arch arm64e
endif

LDFLAGS += -L./

libroothide.dylib: jbroot.c jbroot.cpp jbroot.m cache.c common.c
	$(CC) -fobjc-arc $(CFLAGS) $(LDFLAGS) -lstdc++ -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libroothide.dylib -o $@ $^
	xcrun tapi stubify $@

libvroot.h: vroot.h
	$(CPP) $< > $@

libvroot.dylib: libroothide.dylib vroot.c vroot_mktemp.c vroot.cpp vroot_rootfs.c common.c
	$(CC) $(CFLAGS) $(LDFLAGS) -lstdc++ -lroothide -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libvroot.dylib -o $@ $^
	xcrun tapi stubify $@

libvrootapi.dylib: libvroot.dylib vrootapi.c
	$(CC) $(CFLAGS) $(LDFLAGS) -lvroot -dynamiclib -install_name @loader_path/.jbroot/usr/lib/libvrootapi.dylib -o $@ $^

updatelink: updatelink.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -lroothide -o $@ $^

symredirect: symredirect.cpp
	$(CC) $(CFLAGS) -std=c++11 $(LDFLAGS) -lstdc++ -o $@ $^

jbrand: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -lroothide -o $@ $^

jbroot: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -lroothide -o $@ $^

rootfs: cmdtool.c libroothide.dylib
	$(CC) $(CFLAGS) $(LDFLAGS) -lroothide -o $@ $^


all: $(ALL)
	mkdir devkit
	cp ./libroothide.h ./devkit/
	cp ./libvroot.h	./devkit/
	cp ./symredirect ./devkit/
	cp ./*.tbd ./devkit/
	zip -r devkit.zip ./devkit

clean:
	rm -rf $(ALL) *.tbd
	rm -rf ./devkit.zip
	rm -rf ./devkit

.PHONY: all clean
