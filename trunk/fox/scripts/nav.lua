local ret = makescript()

local currenttask = nil
local startcell, goalcell = { }, { }
local pathengine = nil
local currentpath, currentcell = nil, nil
local currentangle, targetangle = 0, 0

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
    end
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
        
        if currentcell.x < nextcell.x then
            targetangle = 270
        elseif currentcell.x > nextcell.x then
            targetangle = 90
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
            --shortrotate(a)
            self.status = "rotating"
            currentangle = targetangle
        elseif self.status == "rotating" then
            if true or getstate().movecomplete then
                self.status = "startmove"
            end
        elseif self.status == "startmove" then
            print("Moving 300 mm")
            --move(300) -- UNDONE: Grid size
            self.status = "moving"
        elseif self.status == "moving" then
            if true or getstate().movecomplete then
                sendmsg("robotcell", currentcell.x, currentcell.y)
                
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
    run = function(self)
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
    end
}


-- Module functions
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
        if not currenttask then
            settask(taskInitPath)
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
        if currenttask then
            local done, nexttask = currenttask:run()
            if done then
                settask(nexttask)
            end
        end
        coroutine.yield()
    end
end

return ret

