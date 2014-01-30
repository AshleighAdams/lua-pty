

//#include <pty.h>
//int openpty(int *amaster, int *aslave, char *name,						const struct
//termios *termp,						const struct winsize *winp);
//pid_t forkpty(int *amaster, char *name,							const struct termios
//*termp,							const struct winsize *winp);
//#include <utmp.h>
//int login_tty(int fd);
//Link with -lutil.

#include <lua5.2/lua.hpp>

// Posix
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
//#include <util.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pty.h>
#include <termios.h>

// STD
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>

int lua_return(lua_State* L, void* nil, const std::string& reason)
{
	lua_pushnil(L);
	lua_pushstring(L, reason.c_str());
	return 2;
}

int lua_return(lua_State* L, void* nil1, void* nil2, const std::string& reason)
{
	lua_pushnil(L);
	lua_pushnil(L);
	lua_pushstring(L, reason.c_str());
	return 2;
}

int pty_forkpty(lua_State* L)
{
	std::string program = lua_tostring(L, 1);
	if(!lua_istable(L, 2)) { lua_pushstring(L, "argument #2: table expected"); lua_error(L); }
	
	std::string cwd;
	if(!lua_isnil(L, 3))
	 	cwd = lua_tostring(L, 3);
	if(!lua_istable(L, 4)) { lua_pushstring(L, "argument #4: table expected"); lua_error(L); }
	
	int fd = -1;
	char name[40];
	winsize ws;
	ws.ws_row = 24;
	ws.ws_col = 80;
	ws.ws_xpixel = 0;
	ws.ws_ypixel = 0;
	pid_t pid = forkpty(&fd, name, nullptr, &ws);
		
	switch(pid)
	{
	case -1:
		return lua_return(L, nullptr, "failed to fork");
	case 0: // inside the new process
	{
		// cwd
		if(cwd != "")
			chdir(cwd.c_str());
	
		// args
		const int                args_count = lua_rawlen(L, 2);
		char*                    args_ptr[args_count + 2];
		std::vector<std::string> args;
		
		
		lua_pushnil(L);
		while (lua_next(L, 2) != 0)
		{
			args.emplace_back(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		// lua_pop(L, 1); // FIXME: should this be here?
		
		for(int i = 0; i < args_count; i++)
			args_ptr[i + 1] = (char*)args[i].c_str();
			
		args_ptr[0]               = (char*)program.c_str();
		args_ptr[args_count + 1]  = nullptr;
		
		// env
		const int                env_count = lua_rawlen(L, 4);
		char*                    env_ptr[env_count + 1];
		std::vector<std::string> env;
		
		lua_pushnil(L);
		while (lua_next(L, 4) != 0)
		{
			env.emplace_back(lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		// lua_pop(L, 1); // FIXME: should this be here?
		
		for(int i = 0; i < env_count; i++)
			env_ptr[i] = (char*)env[i].c_str();
		env_ptr[env_count] = nullptr;
		
		execvpe(args_ptr[0], (char* const*)args_ptr, (char* const*)env_ptr);
		std::cerr << "lua-pty: error: execvpe failed\n";
		exit(1);
		break;
	}
	default:
		break;
	}
	
	/*int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
		flags = 0;
		
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return lua_return(L, nullptr, "failed to set none-blocking");
	*/
	lua_newtable(L);
	
	lua_pushstring(L, "fd");
	lua_pushnumber(L, fd);
	lua_settable(L, -3);
	
	lua_pushstring(L, "pid");
	lua_pushnumber(L, pid);
	lua_settable(L, -3);
	
	lua_pushstring(L, "pty");
	lua_pushstring(L, name);
	lua_settable(L, -3);
	
	return 1;
}

int pty_setsize(lua_State* L)
{
	int fd = lua_tonumber(L, 1);
	int col = lua_tonumber(L, 2);
	int row = lua_tonumber(L, 3);
	
	winsize ws;
	ws.ws_col = col;
	ws.ws_row = row;
	ws.ws_xpixel = 0;
	ws.ws_ypixel = 0;
	
	if(ioctl(fd, TIOCSWINSZ, &ws) == -1)
		return lua_return(L, nullptr, "ioctl(2) failed");
	
	lua_pushboolean(L, true);
	return 1;
}

int pty_getsize(lua_State* L)
{
	int fd = lua_tonumber(L, 1);
	
	winsize ws;
	
	if(ioctl(fd, TIOCGWINSZ, &ws) == -1)
		return lua_return(L, nullptr, nullptr, "ioctl(2) failed");
	
	lua_pushnumber(L, ws.ws_col);
	lua_pushnumber(L, ws.ws_row);
	return 2;
}

int pty_getproc(lua_State* L)
{
	int fd = lua_tonumber(L, 1);
	
	pid_t pgrp = tcgetpgrp(fd);
	if (pgrp == -1)
		return lua_return(L, nullptr, "failed to locate pid from fd");
	
	std::ifstream ifs("/proc/" + std::to_string(pgrp) + "/cmdline");
	
	if(!ifs.is_open())
		return lua_return(L, nullptr, "failed to open /proc/" + std::to_string(pgrp) + "/cmdline");
	
	std::string contents;
	ifs >> contents;
	ifs.close();
	
	lua_pushstring(L, contents.c_str());
	return 1;
}

// = = = = liolib.c
typedef luaL_Stream LStream;
#define tolstream(L)        ((LStream *)luaL_checkudata(L, 1, LUA_FILEHANDLE))
static int io_fclose (lua_State *L) {
  LStream *p = tolstream(L);
  int res = fclose(p->f);
  return luaL_fileresult(L, (res == 0), NULL);
}
// = = = =

int pty_openfd(lua_State* L)
{
	int fd = lua_tonumber(L, 1);
	
	LStream *p = (LStream *)lua_newuserdata(L, sizeof(LStream));
	p->closef = NULL;  /* mark file handle as 'closed' */
	luaL_setmetatable(L, LUA_FILEHANDLE);
	
	p->f = fdopen(fd, "wr+");
	p->closef = &io_fclose;
	return (p->f == NULL) ? luaL_fileresult(L, 0, "fd") : 1;
}

int pty_readbytes(lua_State* L)
{
	int fd = lua_tonumber(L, 1);
	int bytes = lua_tonumber(L, 2);
	
	char buf[bytes];
	size_t red = read(fd, &buf, bytes);
	
	lua_pushlstring(L, buf, red);
	return 1;
}

int pty_bufferedbytes(lua_State* L)
{
	int fd = lua_tonumber(L, 1);
	int pending;
	
	if(ioctl(fd, FIONREAD, &pending) == -1)
		lua_pushnumber(L, 0);
	else
		lua_pushnumber(L, pending);
	return 1; // 1 return value
}

termios saved_attributes;
void reset_input_mode()
{
	tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

int pty_nocanon(lua_State* L)
{
	termios tattr;

	/* Make sure stdin is a terminal. */
	if (!isatty (STDIN_FILENO))
		return lua_return(L, nullptr, "not a terminal");

	/* Save the terminal attributes so we can restore them later. */
	tcgetattr (STDIN_FILENO, &saved_attributes);
	atexit (reset_input_mode);

	/* Set the funny terminal modes. */
	tcgetattr (STDIN_FILENO, &tattr);
	tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
	tattr.c_cc[VMIN] = 1;
	tattr.c_cc[VTIME] = 0;
	tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
	
	lua_pushboolean(L, true);
	return 1;
}

extern char **environ;
int pty_getenvs(lua_State* L)
{
	lua_newtable(L);

	int i = 1;
	for (char **env = environ; *env; ++env, i++)
	{
		lua_pushnumber(L, i);
		lua_pushstring(L, *env);
		lua_settable(L, -3);
	}
	
	return 1;
}

static const luaL_Reg R[] =
{
	{"forkpty",       pty_forkpty       },
	{"setsize",       pty_setsize       },
	{"getsize",       pty_getsize       },
	{"getproc",       pty_getproc       },
	{"openfd",        pty_openfd        },
	{"pendingbytes",  pty_bufferedbytes },
	{"nocanon",       pty_nocanon       },
	{"getenvs",       pty_getenvs       },
	{ NULL, NULL }
};

extern "C"
{
	LUALIB_API int luaopen_pty_core(lua_State *L)
	{
		luaL_newlib(L, R);
		return 1;
	}
}

// http://thebends.googlecode.com/svn/trunk/misc/pty-example.c
// https://github.com/chjj/pty.js/blob/master/src/unix/pty.cc
