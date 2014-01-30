#!/usr/bin/env lua
local pty = require("pty")

local tty = pty.new()
print(tty:name(), tty:size())

tty:write("ls\n")

-- turn off canonical input (no buffering, instant)
pty.core.nocanon();

while true do
	if pty.stdin.canread() then
		tty:write(io.stdin:read(pty.stdin.pending()))
		tty.file:flush()
	end
	if tty:canread() then
		io.stdout:write(tty:read(tty:pending()))
		io.stdout:flush()
	end
end
