local ret = makescript()

local simmove = false
local currenttask = nil
local cellsize = 30 -- in cm. Bot is about 20 cm long
local gridsize = { }
local startcell, goalcell = { }, { }
local pathengine = nil
local currentpath, currentcell = nil, nil
local currentangle, targetangle = 0, 0

local function average(set)
    local sum = 0
    local n = #set
    
    for i=1, n do
        sum = sum + set[i]
    end
    
    return sum / n
end

local function median(set)
    local ret
    
    table.sort(set)
    
    if math.fmod(#set, 2) > 0 then
        local i = math.floor(#set / 2)
        ret = (set[i] + set[i+1]) / 2
    else
        ret = set[#set / 2]
    end
    
    io.write("median: ")
    for _, v in ipairs(set) do
        io.write(string.format("%d, ", v))
    end
    print("\n== ", ret)
    
    return ret
end

local function setcell(c, x, y)
    c.x = x
    c.y = y
end

local function movecell(c, dx, dy)
    c.x = c.x + dx
    c.y = c.y + dy
end

local function setgrid(w, h)
    gridsize.w = w
    gridsize.h = h
    pathengine:setgrid(w, h)
end

local function expandgrid(left, up, right, down)
    gridsize.w = gridsize.w + left + right
    gridsize.h = gridsize.h + up + down
    
    if left > 0 then
        movecell(startcell, left, 0)
        movecell(goalcell, left, 0)
        movecell(currentcell, left, 0)
    end

    if up > 0 then
        movecell(startcell, 0, up)
        movecell(goalcell, 0, up)
        movecell(currentcell, 0, up)
    end

    pathengine:expandgrid(left, up, right, down)
    sendmsg("gridexpanded", left, up, right, down)
end

local function disttocells(dist)
    return math.floor(dist / cellsize + 0.5) -- Round
end

local function wrapangle(a)
    while a < 0 do
        a = a + 360
    end
    
    while a >= 360 do
        a = a - 360
    end
    
    return a
end

local function servopostoangle(pos)
    -- Servo ranges from 0 (270 deg) to 180 (90 deg)
    -- Angles are converted to clockwise
    return wrapangle(270 + pos)
end

local function settask(task)
    if currenttask and currenttask.finish then
        currenttask:finish()
    end
    
    currenttask = task
    
    if task and task.init then
        task:init()
    end
    
    if not task then
        print("Cleared task")
        currentpath = nil
    end
    
    sendmsg("navigating", (task ~= nil))
end

-- Declare tasks
local taskInitPath, taskMoveToNextNode, taskIRScan

-- Tasks
taskInitPath =
{
    run = function()
        pathengine:init(startcell.x, startcell.y, goalcell.x, goalcell.y)
        local stat
        stat, currentpath = pathengine:calc()
        if stat then
            print("Calculated path!")
            currentcell = table.remove(currentpath, 1) -- Remove first cell (startcell)
            sendmsg("path", currentpath)
            return true, taskIRScan
        else
            print("WARNING: Failed to calculate path!")
            return true, nil
        end
    end
}

taskMoveToNextNode =
{
    init = function(self)
        local nextcell = table.remove(currentpath, 1)
        
        if currentcell.x > nextcell.x then
            targetangle = 270
        elseif currentcell.x < nextcell.x then
            targetangle = 90
        elseif currentcell.y > nextcell.y then
            targetangle = 0
        else
            targetangle = 180
        end
        
        currentcell = nextcell
        
        if targetangle ~= currentangle then
            self.status = "startrotate"
        else
            self.status = "startmove"
        end
    end,
    
    run = function(self)
        if self.status == "startrotate" then
            local a = wrapangle(targetangle - currentangle)
            print(string.format("Rotating %d degrees (%d->%d)", a, currentangle, targetangle))
            
            if not simmove then
                shortrotate(a)
            end
            
            self.status = "rotating"
            currentangle = targetangle
        elseif self.status == "rotating" then
            if getstate().movecomplete or simmove then
                self.status = "startmove"
                getstate().movecomplete = false
                sendmsg("rotation", currentangle)
            end
        elseif self.status == "startmove" then
            print("Moving to next cell")
            if not simmove then
                move(cellsize * 10) -- *10: to mm
            end
            self.status = "moving"
        elseif self.status == "moving" then
            if getstate().movecomplete or simmove then
                sendmsg("robotcell", currentcell.x, currentcell.y)
                getstate().movecomplete = false
                
                -- Done?
                if #currentpath == 0 then
                    print("Finished path!")
                    return true, nil
                else
                    return true, taskIRScan
                end
            end
        end
        
        return false
    end     
}

taskIRScan =
{
    isvalidscandata = function(self, scanset)
        local maxerr = 2
        local errfound = 0
        for _, v in ipairs(scanset) do
            if v < 20 or v < cellsize or v > 150 then
                errfound = errfound + 1
                if errfound > maxerr then
                    return false
                end
            end
        end
        
        return true
    end,
    
    init = function(self)
        self.status = "initdelay"
        self.wait = gettimems() + 500 -- cool down a bit from movement (UNDONE)
        self.scanarray = { }
    end,
        
    run = function(self)
        if self.status == "initdelay" then
            if self.wait < gettimems() then
                self.servopos = 0
                self.status = "setturret"
            end
        elseif self.status == "setturret" then
            setservo(self.servopos)
            -- UNDONE: Verify/tweak
            if self.servopos == 0 then -- Wait longer for first
                self.wait = gettimems() + 1000
            else
                self.wait = gettimems() + 300
            end
            self.status = "wait"
        elseif self.status == "wait" then
            if self.wait < gettimems() then
                self.status = "scan"
                self.scantime = gettimems() + 550 -- ~3-5 scans (30-50 ms) UNDONE
                self.scandelay = 0
            end
        elseif self.status == "scan" then
            if self.scantime < gettimems() then
                self.servopos = self.servopos + 25 -- UNDONE
                if self.servopos > 180 then
                    self.status = "procresults"
                else
                    setservo(self.servopos)
                    self.status = "setturret"
                end
            elseif self.scandelay < gettimems() then
                local angle = servopostoangle(self.servopos)
                self.scandelay = gettimems() + 50 -- UNDONE
                self.scanarray[angle] = self.scanarray[angle] or { }
                table.insert(self.scanarray[angle], getsharpir())
                print(string.format("scan[%d] = %d", angle,
                                    self.scanarray[angle][#self.scanarray[angle]]))
            end
        elseif self.status == "procresults" then
            local refreshpath = false
            
            -- NOTE: not ipairs because scanarray indices have 'gaps' or equal 0 and are
            -- therefore not real lua arrays
            for sangle, disttab in pairs(self.scanarray) do               
                -- Distance in valid range?
                if self:isvalidscandata(disttab) then
                    local sdist = median(disttab) -- UNDONE: More statistics?
                    print(string.format("av scan[%d] = %d", sangle, sdist))

                    local x, y
                    local realangle = wrapangle(currentangle + sangle)
                    if realangle == 0 then -- Straight up
                        x = currentcell.x
                        y = currentcell.y - disttocells(sdist)
                    elseif realangle == 90 then -- Straight right
                        x = currentcell.x + disttocells(sdist)
                        y = currentcell.y
                    elseif realangle == 180 then -- Straight down
                        x = currentcell.x
                        y = currentcell.y + disttocells(sdist)
                    elseif realangle == 270 then -- Straight left
                        x = currentcell.x - disttocells(sdist)
                        y = currentcell.y
                    else
                        local function getcells(angle)
                            local rad = math.rad(angle)
                            local adj = math.cos(rad) * sdist
                            local opp = math.sin(rad) * sdist
                            print("getcells:", adj, opp)
                            return disttocells(adj), disttocells(opp)
                        end
                        
                        if realangle < 90 then
                            local adjcells, oppcells = getcells(realangle)
                            x = currentcell.x + oppcells
                            y = currentcell.y - adjcells
                        elseif realangle < 180 then
                            local adjcells, oppcells = getcells(realangle - 90)
                            x = currentcell.x + adjcells
                            y = currentcell.y + oppcells
                        elseif realangle < 270 then
                            local adjcells, oppcells = getcells(realangle - 180)
                            x = currentcell.x - oppcells
                            y = currentcell.y + adjcells
                        elseif realangle < 360 then
                            local adjcells, oppcells = getcells(realangle - 270)
                            x = currentcell.x - adjcells
                            y = currentcell.y - oppcells
                        end                       
                    end
                    
                    print("av cell:", x, y)
                    print("realangle:", realangle)
                    
                    local exl, exu, exr, exd = 0, 0, 0, 0
                    if x < 0 then
                        exl = math.abs(x)
                    elseif x > gridsize.w then
                        exr = x - gridsize.w
                    end
                    
                    if y < 0 then
                        exu = math.abs(y)
                    elseif y > gridsize.h then
                        exd = y - gridsize.h
                    end
                                        
                    if exl > 0 or exu > 0 or exr > 0 or exd > 0 then
                        print("expand:", exl, exu, exr, exd)

                        expandgrid(exl, exu, exr, exd)
                        refreshpath = true
                        
                        if x < 0 then
                            x = 0
                        end
                        if y < 0 then
                            y = 0
                        end
                    end
                    
                    pathengine:setobstacle(x, y)
                    sendmsg("obstacle", x, y)
                    print("obstacle:", x, y)
                    
                    if not refreshpath then
                        -- Check if cell is in current path list
                        for _, v in ipairs(currentpath) do
                            if v.x == x and v.y == y then
                                refreshpath = true
                                break
                            end
                        end
                    end
                end
            end
            
            if refreshpath then
            	startcell = currentcell
                return true, taskInitPath
            else
                return true, taskMoveToNextNode
            end
        end

        return false
    end
}


-- Module functions
function init()
    pathengine = newpathengine()
    setgrid(10, 10, false)
    setcell(startcell, 0, 0)
    setcell(goalcell, 9, 9)
end

function initclient()
    sendmsg("enablepathclient", true)
    sendmsg("grid", gridsize.w, gridsize.h)
    sendmsg("start", startcell.x, startcell.y)
    sendmsg("goal", goalcell.x, goalcell.y)
    sendmsg("navigating", (currenttask ~= nil))
    
    if currentpath then
        sendmsg("path", currentpath)
    end
end

function handlecmd(cmd, ...)
    print("received cmd:", cmd, "args:", ...)
    
    local args = { ... }
    
    if cmd == "start" then
        if not currenttask then
            currentangle = 180 -- By default bot is facing downwards
            settask(taskInitPath)
        end
    elseif cmd == "abort" then
        settask(nil)
    elseif cmd == "setstart" then
        setcell(startcell, tonumber(args[1]), tonumber(args[2]))
    elseif cmd == "setgoal" then
        setcell(goalcell, tonumber(args[1]), tonumber(args[2]))
    elseif cmd == "setgrid" then
        setgrid(tonumber(args[1]), tonumber(args[2]))
    elseif cmd == "simmove" then
        simmove = (args[1] == "1")
    end
end

function run()
    while true do
        if currenttask then
            local done, nexttask = currenttask:run()
            if done then
                settask(nexttask)
            end
        end
        coroutine.yield()
    end
end

function finish()
    sendmsg("enablepathclient", false)
end

return ret

