include config

CFLAGS += -fPIC -O3 -Wall -Werror --std=c++11
LFLAGS += -shared
LIBS   += -lutil

LUAPKG            = lua5.2
INSTALL           = install -D

CFLAGS += `pkg-config $(LUAPKG) --cflags`
LIBS   += `pkg-config $(LUAPKG) --libs`

SOURCES = src/main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: core.so pty.lua

test:
	echo $(DESTDIR)

pty.lua:
	luac -p src/$@

core.o: src/main.cpp
	$(CXX) -o core.o -c $(CFLAGS) $<
#./src/main.cpp

core.so: core.o
	$(CXX) -o $@ $(LFLAGS) $(LIBS) $<
	
clean:
	$(RM) core.o core.so ./*~ ./**/*~
	$(RM) -r debian/lua-pty/
	$(RM) debian/debhelper.log debian/lua-pty.debhelper.log debian/lua-pty.substvars debian/files


install-lua: src/pty.lua
	$(INSTALL) src/pty.lua $(LUA_DIR)/pty.lua
	
install-so: core.so
	$(INSTALL) $< $(LUA_LIBDIR)/pty/core.so
	
install: install-so install-lua

deb: pty.lua core.so
	@echo generating deb
	./gitlog-to-changelog > debian/changelog
	dpkg-buildpackage -rfakeroot
	

