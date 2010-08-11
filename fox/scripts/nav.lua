local ret = makescript()

local startcell, goalcell = { }, { }
local pathengine = nil
local navstatus = "idle"
local currentpath = nil

local function setcell(c, x, y)
	c.x = x
	c.y = y
end

function init()
	pathengine = newpathengine()
	pathengine:setgrid(10, 10)
	setcell(startcell, 0, 0)
	setcell(goalcell, 0, 0)
end

function handlecmd(cmd, ...)
	print("received cmd:", cmd, "args:", ...)
	
	local args = { ... }
	
	if cmd == "start" then
		if navstatus == "idle" then
			navstatus = "init"
		end
	elseif cmd == "setstart" then
		setcell(startcell, args[1], args[2])
	elseif cmd == "setgoal" then
		setcell(goalcell, args[1], args[2])
	elseif cmd == "setgrid" then
		pathengine:setgrid(args[1], args[2], args[3], args[4])
	end
		
end

function run()
	while true do
		if navstatus == "init" then
			pathengine:init(startcell.x, startcell.y, goalcell.x, goalcell.y)
			local stat
			stat, currentpath = pathengine:calc()
			if stat then
				navstatus = "navigating"
			else
				navstatus = "idle"
				print("WARNING: Failed to calculate path!")
			end
		elseif navstatus == "navigating" then
			-- UNDONE
		end
		
		coroutine.yield()
	end
end

return ret

