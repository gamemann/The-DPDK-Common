BUILDDIR=objs
SRCDIR=src

DPDKCOMMONSRC := dpdk_common.c
DPDKCOMMONOBJ := dpdk_common.o

PKGCONF ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as forwarding uses rte_ethdev_set_ptype API.
CFLAGS += -DALLOW_EXPERIMENTAL_API
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)
LDFLAGS_STATIC = $(shell $(PKGCONF) --static --libs libdpdk)

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

all: static shared
.PHONY: all

makebuilddir:
	mkdir -p $(BUILDDIR)/static
	mkdir -p $(BUILDDIR)/shared

static: $(SRCDIR)/$(DPDKCOMMONSRC) Makefile $(PC_FILE) | makebuilddir
	$(CC) -c $(CFLAGS) $(SRCDIR)/$(DPDKCOMMONSRC) -o $(BUILDDIR)/static/$(DPDKCOMMONOBJ) $(LDFLAGS) $(LDFLAGS_STATIC)

shared: $(SRCDIR)/$(DPDKCOMMONSRC) Makefile $(PC_FILE) | makebuilddir
	$(CC) -c $(CFLAGS) $(SRCDIR)/$(DPDKCOMMONSRC) -o $(BUILDDIR)/shared/$(DPDKCOMMONOBJ) $(LDFLAGS) $(LDFLAGS_SHARED)

.PHONY: clean
clean:
	rm -f $(BUILDDIR)/static/$(DPDKCOMMONOBJ)
	rm -f $(BUILDDIR)/shared/$(DPDKCOMMONOBJ)