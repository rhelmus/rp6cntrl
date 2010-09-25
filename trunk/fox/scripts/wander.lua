local ret = makescript()

local currenttask = nil

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
local driveTask, collisionTask, scanTask

driveTask =
{   
    init = function(self)
        self.updatedelay = 0
        robot.setservo(90)
        robot.motor.setspeed(80, 80)
    end,
    
    run = function(self)
        if self.updatedelay > gettimems() then
            return
        end
        
        self.updatedelay = gettimems() + 100
        
        if robot.sensors.bumperleft() or robot.sensors.bumperright() then
            return true, collisionTask
        elseif robot.sensors.acsleft() or robot.sensors.acsright() then
            return true, scanTask
        else
            local dist = robot.sensors.sharpir()
            if dist > 20 and dist < 50 then
                return true, scanTask
            end
        end
        
        return false
    end
}

collisionTask =
{
    init = function(self)
        robot.move(15, 60, "bwd")
        self.delay = gettimems() + 500
    end,
    
    run = function(self)
        if self.delay < gettimems() then
            if robot.motor.movecomplete() then
                return true, scanTask
            end
            self.delay = gettimems() + 50
        end
        return false
    end
}

scanTask =
{   
    init = function(self)
        robot.motor.stop()
        self.delay = gettimems() + 100
        self.state = "initrotate"
    end,
            
    run = function(self)
        if self.delay < gettimems() then
            if self.state == "initrotate" then
                -- UNDONE
                robot.rotate(90)
                self.state = "rotate"
            elseif self.state == "rotate" then
                if robot.motor.movecomplete() then
                    return true, driveTask
                end
            end
            self.delay = gettimems() + 50
        end
        
        return false
    end
}

function init()
    robot.setacs("med")
    settask(driveTask)
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
    robot.motor.stop()
    robot.setacs("off")
end

return ret
