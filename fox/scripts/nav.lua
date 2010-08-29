local ret = makescript()

local grid
local simmove = false
local currenttask = nil
local currentpath = nil
local targetangle = 0

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
        local stat
        stat, currentpath = grid:calcpath()
        if stat then
            print("Calculated path!")
             -- Remove start cell from path list
            grid:setrobot(table.remove(currentpath, 1))
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
        local currentcell = grid:getrobot()
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
        
        grid:setrobot(nextcell)
        
        if targetangle ~= currentangle then
            self.status = "startrotate"
        else
            self.status = "startmove"
        end
    end,
    
    run = function(self)
        if self.status == "startrotate" then
            local curangle = grid:getrobotangle()
            local a = math.wrapangle(targetangle - curangle)
            print(string.format("Rotating %d degrees (%d->%d)", a, curangle, targetangle))
            
            if not simmove then
                robot.shortrotate(a)
            end
            
            self.status = "rotating"
            
            -- Use a delay before checking if movement is completed to prevent
            -- testing old data
            self.wait = gettimems() + 500
        elseif self.status == "rotating" then
            if self.wait < gettimems() and (robot.motor.movecomplete() or simmove) then
                grid:setrobotangle(targetangle)
                self.status = "startmove"
                self.wait = gettimems() + 500
            end
        elseif self.status == "startmove" then
            print("Moving to next cell")
            if not simmove then
                robot.move(cellsize * 10) -- *10: to mm
            end
            self.status = "moving"
        elseif self.status == "moving" then
            if self.wait < gettimems() and (robot.motor.movecomplete() or simmove) then
                -- UNDONE: Need this?
                --sendmsg("robotcell", currentcell.x, currentcell.y)
                
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
            robot.setservo(self.servopos)
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
                    robot.setservo(self.servopos)
                    self.status = "setturret"
                end
            elseif self.scandelay < gettimems() then
                local angle = robot.servoangle(self.servopos)
                self.scandelay = gettimems() + 50 -- UNDONE
                self.scanarray[angle] = self.scanarray[angle] or { }
                table.insert(self.scanarray[angle], robot.sensors.sharpir())
                print(string.format("scan[%d] = %d", angle,
                                    self.scanarray[angle][#self.scanarray[angle]]))
            end
        elseif self.status == "procresults" then
            return true, taskMoveToNextNode -- UNDONE
            --[[
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
            --]]
        end

        return false
    end
}


-- Module functions
function init()
    grid = nav.newgrid()
    grid:setsize(10, 10)
end

function initclient()
    grid:initclient()
    sendmsg("navigating", (currenttask ~= nil))
    
    if currentpath then
        sendmsg("path", currentpath)
    end
end

function handlecmd(cmd, ...)
    print("received cmd:", cmd, "args:", ...)
    
    if grid:handlecmd(cmd, ...) then
        return
    end
       
    if cmd == "start" then
        if not currenttask then
            grid:setrobotangle(180) -- By default bot is facing downwards
            settask(taskInitPath)
        end
    elseif cmd == "abort" then
        settask(nil)
    elseif cmd == "simmove" then
        simmove = (select(1, ...) == "1")
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
