local ret = makescript()

local state = "wait"
local scandelay, scanlifetime, scans, leftdist, rightdist

local function average(set)
	local sum = 0
	local n = #set
	
	for i=1, n do
		sum = sum + set[i]
	end
	
	return sum / n
end

function handlecmd(cmd, ...)
	if cmd == "start" then
		state = "initrotate"
	elseif cmd == "abort" then
	    state = "waiting"
	end
end

function run()
	while true do
		if state == "initrotate" then
			rotate(360)
			state = "rotating"
			getstate().movecomplete = false
			print("Started rotate")
			beep(140, 130)
		elseif state == "rotating" then
		    if getstate().movecomplete then
		    	state = "initleftscan"
		    end
        elseif state == "initleftscan" or state == "initrightscan" then
            scandelay = gettimems() + 500
            scanlifetime = scandelay + 150 -- ~3 scans
            scans = { }
            if state == "initleftscan" then
                state = "scanleft"
                setservo(45)
            else
                state = "scanright"
                setservo(135)
            end
        elseif state == "scanleft" then
            if scanlifetime < gettimems() then
                state = "initrightscan"
                leftdist = average(scans)
            elseif scandelay < gettimems() then
                table.insert(scans, getsharpir())
                scandelay = gettimems() + 50
            end
        elseif state == "scanright" then
            if scanlifetime < gettimems() then
                state = "dumpresult"
                rightdist = average(scans)
            elseif scandelay < gettimems() then
                table.insert(scans, getsharpir())
                scandelay = gettimems() + 50
            end
        elseif state == "dumpresult" then
            local deg = math.deg(math.atan(rightdist / leftdist)) - 45
            sound(160, 80, 25)
            beep(210, 80)
            print("Finished!")
            print("Left/Right dist:", leftdist, rightdist)
            print("deg:", deg)
            state = "wait"
        end

		coroutine.yield()
	end
end

return ret
