local pty = {}
pty.core = require("pty.core")

local tty = {}

function pty.new(cmd, args, cols, rows)
	cmd = cmd or "/bin/bash"
	args = args or {}
	local ret = pty.core.forkpty(cmd, args, nil, pty.core.getenvs()) -- exec, args, cwd, env
	
	if cols ~= nil and rows ~= nil then
		assert(pty.core.setsize(ret.fd, cols, rows))
	end
	
	ret.file = pty.core.openfd(ret.fd)
	-- turn off buffering, otherwise it just seemingly "hangs" for no reason
	ret.file:setvbuf("no")
	--ret.file = io.open(string.format("/proc/self/fd/%d", ret.fd), "r+")
	
	return setmetatable(ret, tty.meta)
end

pty.stdin = {}
function pty.stdin.canread()
	return pty.stdin.pending() ~= 0
end

function pty.stdin.pending()
	return pty.core.pendingbytes(0) -- 0 == stdin
end

function tty:size()
	return assert(pty.core.getsize(self.fd))
end

function tty:setsize(cols, rows)
	assert(pty.core.setsize(self.fd, cols, rows))
end

function tty:name()
	return assert(self.pty)
end

function tty:read(...)
	return self.file:read(...)
end

function tty:canread()
	return self:pending() ~= 0
end

function tty:pending()
	return pty.core.pendingbytes(self.fd)
end

function tty:write(...)
	return self.file:write(...)
end

local function tty_tostring(tty)
	return string.format("tty: %s", tty:name())
end

tty.meta = {__index = tty, __tostring = tty_tostring}
return pty
