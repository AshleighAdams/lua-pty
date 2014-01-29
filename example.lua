#!/usr/bin/env lua
local pty = require("pty")

local tty = pty.new()
print(tty:name(), tty:size())

tty:write("ls\n")

-- turn off canonical input (no buffering, instant)
pty.core.nocanon();

while true do
	if pty.stdin.canread() then
		io.stderr:write("cin > tty: pre-pending: " .. pty.stdin.pending() .. "\n")
		tty:write(io.stdin:read(pty.stdin.pending()))
		tty.file:flush()
		io.stderr:write("cin > tty: post-pending: " .. pty.stdin.pending() .. "\n")
	end
	if tty:canread() then
		io.stderr:write("tty > cout: pre-pending: " .. tty:pending() .. "\n")
		io.stdout:write(tty:read(tty:pending()))
		io.stdout:flush()
		io.stderr:write("tty > cout: post-pending: " .. tty:pending() .. "\n")
	end
end
