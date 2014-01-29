
CFLAGS += -fPIC -O3 -Wall -Werror --std=c++11
LFLAGS += -shared

LUAPKG            = lua5.2
INSTALL           = install -D
INSTALL_DEST_CPP  = `$(LUAPKG) -e "print(package.cpath:match('(.-)%/%?.so'))"`
INSTALL_DEST_LUA  = `$(LUAPKG) -e "print(package.path:match('(.-)%/%?.lua'))"`

CFLAGS += `pkg-config $(LUAPKG) --cflags`
LIBS   += `pkg-config $(LUAPKG) --libs`

SOURCES = src/main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: core.so

core.o: src/main.cpp
	$(CC) -o core.o -c $(CFLAGS) $<
#./src/main.cpp

core.so: core.o
	$(CC) -o $@ $(LFLAGS) $(LIBS) $<
	
clean:
	$(RM) core.o core.so ./*~ ./**/*~


install-lua: src/pty.lua
	$(INSTALL) src/pty.lua $(INSTALL_DEST_LUA)/pty.lua
	
install-so: core.so
	$(INSTALL) $< $(INSTALL_DEST_CPP)/pty/$<
	
install: install-so install-lua
