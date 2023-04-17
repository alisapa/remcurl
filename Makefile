CURL_CFLAGS=$(shell curl-config --cflags)
CURL_LDFLAGS=$(shell curl-config --libs)
CJSON_CFLAGS=
CJSON_LDFLAGS=-lcjson
# Use these to pass extra CFLAGS/LDFLAGS for debugging
EXTRA_CFLAGS=
EXTRA_LDFLAGS=

CFLAGS=$(CURL_CFLAGS) $(CJSON_CFLAGS) $(EXTRA_CFLAGS)
LDFLAGS=$(CURL_LDFLAGS) $(CJSON_LDFLAGS) $(EXTRA_LDFLAGS)

OBJDIR=obj
SRCDIR=src

_DEPS=download_funcs.h
_OBJS=download_funcs.o main.o
_SRCS=download_funcs.c main.c
DEPS=$(patsubst %,$(SRCDIR)/%,$(_DEPS))
OBJS=$(patsubst %,$(OBJDIR)/%,$(_OBJS))

INSTALL_PREFIX=/usr/local
INSTALL_BIN=$(INSTALL_PREFIX)/bin
INSTALL_MAN=$(INSTALL_PREFIX)/share/man

remcurl: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	@mkdir -p $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o
	rm -f remcurl

.PHONY: install
install:
	mkdir -p $(INSTALL_BIN)
	cp remcurl $(INSTALL_BIN)/remcurl
	mkdir -p $(INSTALL_MAN)
	cp man/remcurl.1 $(INSTALL_MAN)/remcurl
.PHONY: uninstall
uninstall:
	rm -f $(INSTALL_BIN)/remcurl
	rm -f $(INSTALL_MAN)/remcurl.1
