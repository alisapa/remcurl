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

remcurl: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o
	rm -f remcurl
