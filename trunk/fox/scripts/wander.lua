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
        robot.setservo(90)
        robot.motor.stop()
        self.delay = gettimems() + 750
        self.initmotor = true
        print("Started drive task")
    end,
    
    run = function(self)
        if self.delay > gettimems() then
            return
        end
       
        self.delay = gettimems() + 100

        if self.initmotor then
            robot.motor.setspeed(80, 80)
            robot.motor.setdir("fwd")
            self.initmotor = false
        elseif robot.sensors.bumperleft() or robot.sensors.bumperright() then
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
        robot.move(300, 60, "bwd")
        self.delay = gettimems() + 500
        print("Started collision task")
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
    isvalidscandata = function(self, scanset)
        local maxerr = 2
        local errfound = 0
        for _, v in ipairs(scanset) do
            if v < 20 or v > 150 then
                errfound = errfound + 1
                if errfound > maxerr then
                    return false
                end
            end
        end
        
        return true
    end,
    
    getbestangle = function(self)
        local freeangles = { } -- Angles were no hit is found
        local furthest = { } -- Angles at which no close hit was found
        for angle, scans in pairs(self.scanarray) do
            if not self:isvalidscandata(scans) then
                table.insert(freeangles, angle)
            else
                local avdist = math.median(scans)
                if avdist >= 100 and
                   (not furthest.angle or avdist > furthest.dist) then
                    furthest.angle = angle
                    furthest.dist = avdist
                end
            end
        end
        
        if #freeangles > 0 then -- Best case, some place without obstacles
            -- Pick a random one
            -- UNDONE: Score
            return freeangles[math.random(#freeangles)]
        elseif furthest.angle then
            return furthest.angle
        end
        
        -- return nil
    end,
    
    init = function(self)
        robot.motor.stop()
        self.delay = gettimems() + 100
        self.state = "initscan"
        print("Started scan task")
    end,
            
    run = function(self)
        if self.delay < gettimems() then
            if self.state == "initscan" then
                robot.setservo(0)
                self.servopos = 0
                self.delay = gettimems() + 1500
                self.scantime = self.delay + 500
                self.scandelay = 0
                self.state = "scan"
                self.scanarray = { }
            elseif self.state == "scan" then
                if self.scantime < gettimems() then
                    self.servopos = self.servopos + 30
                    if self.servopos > 180 then
                        self.state = "initrotate"
                        robot.setservo(90)
                        self.delay = gettimems() + 750
                        
                        print("Scan results:")
                        for a, l in pairs(self.scanarray) do
                            for i, d in ipairs(l) do
                                print(string.format("[%d][%d] = %d", a, i, d))
                            end
                        end
                    else
                        robot.setservo(self.servopos)
                        self.delay = gettimems() + 750
                        self.scantime = self.delay + 500
                        self.scandelay = 0
                    end
                elseif self.scandelay < gettimems() then
                    local angle = robot.servoangle(self.servopos)
                    self.scanarray[angle] = self.scanarray[angle] or { }
                    table.insert(self.scanarray[angle], robot.sensors.sharpir())
                    self.scandelay = gettimems() + 50 -- UNDONE
                end
            elseif self.state == "initrotate" then
                local targetangle = self:getbestangle()
                
                if not targetangle then
                    -- Just rotate 180 degrees and hope for the best
                    targetangle = 180
                end
                
                robot.shortrotate(targetangle)
                self.state = "rotate"
                self.delay = gettimems() + 300
            elseif self.state == "rotate" then
                if robot.motor.movecomplete() then
                    return true, driveTask
                end
                -- UNDONE: Check bumpers
                self.delay = gettimems() + 150
            end
        end
        
        return false
    end
}

function init()
    robot.setacs("med")
    settask(driveTask)
    math.randomseed(os.time())
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
