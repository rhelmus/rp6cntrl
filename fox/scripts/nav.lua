local ret = makescript()

local currenttask = nil
local gridsize = { }
local startcell, goalcell = { }, { }
local pathengine = nil
local currentpath, currentcell = nil, nil
local currentangle, targetangle = 0, 0

local function setgrid(w, h)
    gridsize.w = w
    gridsize.h = h
    pathengine:setgrid(w, h)
end

local function setcell(c, x, y)
    c.x = x
    c.y = y
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
            sendmsg("path", currentpath)
            currentcell = table.remove(currentpath, 1) -- Remove first cell (startcell)
            currentangle = 0
            return true, taskMoveToNextNode
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
            targetangle = 180
        else
            targetangle = 0
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
            local a = targetangle - currentangle
            if a < 0 then
                a = a + 360
            end
            print(string.format("Rotating %d degrees (%d->%d)", a, currentangle, targetangle))
            shortrotate(a)
            self.status = "rotating"
            currentangle = targetangle
        elseif self.status == "rotating" then
            if true or getstate().movecomplete then
                self.status = "startmove"
                getstate().movecomplete = false
                sendmsg("rotation", currentangle)
            end
        elseif self.status == "startmove" then
            print("Moving 300 mm")
            move(300) -- UNDONE: Grid size
            self.status = "moving"
        elseif self.status == "moving" then
            if true or getstate().movecomplete then
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
    init = function(self)
        self.status = "setturret"
        self.targetangle = 0
        self.scanarray = { }
    end,
    
    run = function(self)
        if self.status == "setturret" then
            setservo(self.targetangle)
            -- UNDONE: Verify/tweak
            if self.targetangle == 0 then -- Wait longer for first
                self.wait = gettimems() + 500
            else
                self.wait = gettimems() + 100
            end
            self.status = "wait"
        elseif self.status == "wait" then
            if self.wait < gettimems() then
                self.status = "scan"
                self.scantime = gettimems() + 150 -- ~3-5 scans (30-50 ms)
                self.scandelay = 0
            end
        elseif self.status == "scan" then
            if self.scantime < gettimems() then
                self.targetangle = self.targetangle + 25 -- UNDONE
                if self.targetangle > 180 then
                    return true, taskMoveToNextNode
                end
                setservo(self.targetangle)
                self.status = "setturret"
            elseif self.scandelay < gettimems() then
                self.scandelay = gettimems() + 50 -- UNDONE
                self.scanarray[self.targetangle] = self.scanarray[self.targetangle] or { }
                table.insert(self.scanarray[self.targetangle], getsharpir())
                print(string.format("scan[%d] = %d", self.targetangle,
                                    self.scanarray[self.targetangle][#self.scanarray[self.targetangle]]))
            end
        end

        return false
        --[[
    -- UNDONE
        if not timeout then
            timeout = gettimems()
            return false
        elseif (gettimems() - timeout) >= 400 then
            timeout = nil
            return true, taskMoveToNextNode
        else
            return false
        end
        --]]
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
            settask(taskInitPath)
        end
    elseif cmd == "abort" then
        settask(nil)
    elseif cmd == "setstart" then
        setcell(startcell, args[1], args[2])
    elseif cmd == "setgoal" then
        setcell(goalcell, args[1], args[2])
    elseif cmd == "setgrid" then
        setgrid(args[1], args[2])
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

