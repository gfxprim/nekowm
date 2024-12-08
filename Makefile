CFLAGS?=-W -Wall -Wextra -O2 -ggdb
CFLAGS+=-std=gnu99 $(shell gfxprim-config --cflags)
BIN=nekowm
BIN_LOGIN=nekowm-login
#TODO: Move text_fit to core to avoid linking against widgets
$(BIN): LDLIBS=-lcrypt -lgfxprim $(shell gfxprim-config --libs-backends) -lgfxprim-widgets
$(BIN_LOGIN): LDLIBS=-lcrypt -lgfxprim $(shell gfxprim-config --libs-backends) -lgfxprim-widgets
SOURCES=$(wildcard *.c)
DEP=$(SOURCES:.c=.dep)
OBJ=$(SOURCES:.c=.o)

all: $(BIN) $(BIN_LOGIN) $(DEP)

%.dep: %.c
	$(CC) $(CFLAGS) -M $< -o $@

$(BIN): $(filter-out nekowm-login.o,$(OBJ))
$(BIN_LOGIN): login.o

-include $(DEP)

install:
	install -d $(DESTDIR)/usr/bin/
	install $(BIN) -t $(DESTDIR)/usr/bin/
	install $(BIN_LOGIN) -t $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/usr/share/applications/
	install -m 644 $(BIN).desktop -t $(DESTDIR)/usr/share/applications/
	install -d $(DESTDIR)/usr/share/$(BIN)/
	install -m 644 $(BIN).png -t $(DESTDIR)/usr/share/$(BIN)/
	install -d $(DESTDIR)/etc/systemd/system/
	install -m 644 $(BIN).service -t $(DESTDIR)/etc/systemd/system/

clean:
	rm -f $(BIN) *.dep *.o

