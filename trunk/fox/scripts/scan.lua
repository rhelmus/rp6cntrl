local ret = makescript()

local function isvalidscandata(scanset)
    local maxerr = 2
    local errfound = 0
    for _, v in ipairs(scanset) do
        if v < 20 or v < grid:getcellsize() or v > 150 then
            errfound = errfound + 1
            if errfound > maxerr then
                return false
            end
        end
    end
    
    return true
end

function init()
    grid = nav.newgrid()
    grid:setsize(10, 10)
    grid:setrobot(grid[0][0])
    grid:setrobotangle(180)
end

function initclient()
    grid:initclient()
end

function run()
    local prevhit
	for servopos = 0, 180, 10 do
		robot.setservo(servopos)
		local wait = gettimems() + 500
		while wait > gettimems() do
		    coroutine.yield()
		end
		
		local stime = gettimems() + 1000
		local scandata = { }
		while stime > gettimems() do
			if wait < gettimems() then
				wait = gettimems() + 50
				local angle = robot.servoangle(servopos)
				scandata[angle] = scandata[angle] or { }
				table.insert(scandata[angle], robot.sensors.sharpir())
			end
			coroutine.yield()
		end
		
        -- NOTE: not ipairs because scanarray indices have 'gaps' or equal 0 and are
        -- therefore not real lua arrays
        for sangle, disttab in pairs(scandata) do               
            -- Distance in valid range?
            if isvalidscandata(disttab) then
            	print("prevhit:", prevhit)
                local sdist = math.median(disttab)
                print(string.format("av scan[%d] = %d", sangle, sdist))
                
                local robotpos = grid:getvec(grid:getrobot())
                local scanpos = robot.sensoroffset()
                scanpos:rotate(math.wrapangle(-grid:getrobotangle()))
                scanpos = scanpos + robotpos
                
                local hit = nav.newvector(0, -sdist)
                hit:rotate(math.wrapangle(sangle + grid:getrobotangle()))
                hit = hit + scanpos
                
                print("botpos:", robotpos, grid:getrobotangle())
                print("scanpos:", scanpos)
                print("hit:", hit)
                
                local hitcell, expanded, exl, exu = grid:safegetcell(hit)
                print("hitcell:", hitcell)
                
                if hit:x() < 0 then
                    hit:setx(grid:getcellsize() - math.fmod(-hit:x(), grid:getcellsize()))
                end
                if hit:y() < 0 then
                    hit:sety(grid:getcellsize() - math.fmod(-hit:y(), grid:getcellsize()))
                end
                
                sendmsg("hit", hit:x(), hit:y())
                
                if prevhit then
                    if exl > 0 then
                        prevhit:setx(prevhit:x() + (exl * grid:getcellsize()))
                    end
                    if exu > 0 then
                        prevhit:sety(prevhit:y() + (exu * grid:getcellsize()))
                    end
                    sendmsg("connecthit", prevhit:x(), prevhit:y(), hit:x(), hit:y())
                end
                
                -- For now discard results from current cell
--[[                if hitcell ~= grid:getrobot() then
                    grid:addobstacle(hitcell)                   
                end]]
                
                prevhit = hit
            else
                prevhit = nil
            end
        end
        coroutine.yield()
    end
end

return ret

