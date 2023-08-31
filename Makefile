
ARCHS = arm64 arm64e
TARGET := iphone:clang:latest:15.0
#THEOS_PACKAGE_SCHEME=rootless

THEOS_PLATFORM_DEB_COMPRESSION_TYPE = gzip

include $(THEOS)/makefiles/common.mk

LIBRARY_NAME = libroothide libvroot libvrootapi

libroothide_FILES = jbroot.c jbroot.cpp jbroot.m cache.c common.c
libroothide_LDFLAGS += -install_name @loader_path/.jbroot/usr/lib/libroothide.dylib
libroothide_INSTALL_PATH = /usr/lib

libvroot_FILES = vroot.c vroot_mktemp.c vroot.cpp vroot_rootfs.c common.c
libvroot_CFLAGS += -I./
libvroot_LDFLAGS += -install_name @loader_path/.jbroot/usr/lib/libvroot.dylib -L$(THEOS_OBJ_DIR) -lroothide
libvroot_INSTALL_PATH = /usr/lib

libvrootapi_FILES = vrootapi.c
libvrootapi_LDFLAGS += -install_name @loader_path/.jbroot/usr/lib/libvrootapi.dylib -L$(THEOS_OBJ_DIR) -lvroot
libvrootapi_INSTALL_PATH = /usr/lib
libvrootapi_USE_MODULES = no
include $(THEOS_MAKE_PATH)/library.mk


TOOL_NAME = updatelink symredirect

updatelink_FILES = updatelink.c
updatelink_CFLAGS += -I./
updatelink_LDFLAGS += -L$(THEOS_OBJ_DIR) -lroothide
updatelink_INSTALL_PATH = /usr/libexec
updatelink_CODESIGN_FLAGS = -Sentitlements.plist

symredirect_FILES = symredirect.cpp
symredirect_CFLAGS += -I./ -std=c++11
symredirect_INSTALL_PATH = /usr/bin
symredirect_CODESIGN_FLAGS = -Sentitlements.plist


include $(THEOS_MAKE_PATH)/tool.mk


after-libvroot-all::
	$(CPP) vroot.h > libvroot.h

clean::
	rm -rf ./packages/*

after-install::
	install.exec 'installed'
