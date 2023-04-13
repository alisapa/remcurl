CURL_CFLAGS=$(shell curl-config --cflags)
CURL_LDFLAGS=$(shell curl-config --libs)
CJSON_CFLAGS=
CJSON_LDFLAGS=-lcjson

CFLAGS=$(CURL_CFLAGS) $(CJSON_CFLAGS)
LDFLAGS=$(CURL_LDFLAGS) $(CJSON_LDFLAGS)

OBJDIR=obj
SRCDIR=src

_DEPS=download_funcs.h
_OBJS=download_funcs.o main.o
_SRCS=download_funcs.c main.c
DEPS=$(patsubst %,$(SRCDIR)/%,$(_DEPS))
OBJS=$(patsubst %,$(OBJDIR)/%,$(_OBJS))

rem: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(OBJDIR)/*.o
	rm -f rem
