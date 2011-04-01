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
        
        if targetangle ~= grid:getrobotangle() then
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
                robot.move(grid:getcellsize() * 10) -- *10: to mm
            end
            self.status = "moving"
            self.wait = gettimems() + 500
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
            if v < 20 or v < grid:getcellsize() or v > 150 then
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
            local refreshpath = false
            
            -- NOTE: not ipairs because scanarray indices have 'gaps' or equal 0 and are
            -- therefore not real lua arrays
            for sangle, disttab in pairs(self.scanarray) do               
                -- Distance in valid range?
                if self:isvalidscandata(disttab) then
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
                                       
                    local hitcell, expanded = grid:safegetcell(hit)
                    print("hitcell:", hitcell)
                    
                    local hx, hy = hit:x(), hit:y()
                    if hx < 0 then
                        hx = math.fmod(-hx, grid:getcellsize())
                    end
                    if hy < 0 then
                        hy = math.fmod(-hy, grid:getcellsize())
                    end
                    
                    sendmsg("hit", hx, hy)
                    
                    -- For now discard results from current cell
                    if hitcell ~= grid:getrobot() then
                        refreshpath = refreshpath or expanded
                        grid:addobstacle(hitcell)
                        
                        if not refreshpath then
                            -- Check if cell is in current path list
                            for _, v in ipairs(currentpath) do
                                if v == hitcell then
                                    refreshpath = true
                                    print("hitcell found")
                                    break
                                end
                            end
                        end
                    end
                end
            end
            
            if refreshpath then
                grid:setpathstart(grid:getrobot())
                print("new start path:", grid:getrobot(), grid:getpathgoal())
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
	setserialdelay("sharpir", 50)
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

