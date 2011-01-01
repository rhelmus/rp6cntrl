--[[
IDEAS:
    * Scan while drive (if possible)
        - Quick 4 scans (2 on both sides)
        - 90 and -90: Adjust speed ratio
        - Other: Randomly turn left/right if looks favourable
        - Maybe: wall following (needs close 90 degree scan --> only
          if not planning to change direction)
    * Light sensors: avoid darkness
    * ACS
        - Alternate power
        - Act differently on left/right if power is not low
    * LEDs, bleeps...
        - Bleeping while going backwards
    * Change speed depending on scans
    * Button: suspend movement
--]]

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

local function isvalidscandata(scanset)
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
end

local function getbestangle(scanarray, mindist)
    local freeangles = { } -- Angles where no hit is found
    local furthest = { } -- Angles at which no close hit was found
    mindist = mindist or 0
    
    for angle, scans in pairs(scanarray) do
        if not isvalidscandata(scans) then
            table.insert(freeangles, angle)
        else
            local avdist = math.median(scans)
            if avdist >= mindist and
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
end
                       
-- Declare tasks
local driveTask, collisionTask, scanTask

driveTask =
{   
    drivespeed = 90,
    slowspeed = 70,
    
    init = function(self)
        robot.setservo(90)
        robot.motor.stop()
        self.acsfreetime = 0
        self.sharpirclosetime = 0
        self.scandelay = 0
        self.servopos = 90
        self.scandir = "right"
        self.moveslow, self.checkslow = false, true
        
        self.state = "initmotor"
        self.delay = gettimems() + 750
        print("Started drive task")
    end,
    
    run = function(self)
        if self.delay > gettimems() then
            return false
        end
       
        self.delay = gettimems() + 100
        
        if not robot.sensors.acsleft() and not robot.sensors.acsright() then
            self.acsfreetime = gettimems()
        end

        if self.turntime < gettimems() then
            robot.motor.setspeed(self.drivespeed, self.drivespeed)
            self.turntime = 0
        end
        
        if robot.sensors.bumperleft() or robot.sensors.bumperright() then
            return true, collisionTask
        elseif self.acsfreetime < (gettimems() - 300) then
            return true, scanTask
        elseif self.state == "initmotor" then
            robot.motor.setspeed(self.drivespeed, self.drivespeed)
            robot.motor.setdir("fwd")
            self.state = "idledrive"                
        elseif self.state == "idledrive" then
            local dist = robot.sensors.sharpir()
            if dist > 20 and dist < 40 then
                return true, scanTask
            end
            
            if dist < 100 and dist > 20 then
                self.sharpirclosetime = gettimems()
            end
            
            if self.scandelay < gettimems() then
                self.state = "scan"
                self.scanarray = { }
                
                local dspos
                if self.servopos > 90 then
                    self.scandir = "left"
                    dspos = 180 - self.servopos
                    self.servopos = 180
                else
                    self.scandir = "right"
                    dspos = self.servopos
                    self.servopos = 0
                end
                
                self.scandelay = gettimems() + (dspos / robot.servospeed())
                self.scantime = self.scandelay + 200
                robot.setservo(self.servopos)
            end
        elseif self.state == "scan" then
            if self.scandelay < gettimems() then
                local curscanangle = robot.servoangle(self.servopos)
                self.scanarray[curscanangle] = self.scanarray[curscanangle] or { }
                table.insert(self.scanarray[curscanangle], robot.sensors.sharpir())
                
                if self.scantime < gettimems() then
                    local newspos
                    if self.scandir == "left" then
                        newspos = self.servopos - 30
                    else
                        newspos = self.servopos + 30
                    end
                    
                    if newspos < 0 or newspos > 180 then
                        -- Process results
                        self.state = "idledrive" -- May change below
                        
                        -- Something in front?
                        if isvalidscandata(self.scanarray[0]) then
                            local avdist = math.median(self.scanarray[0])
                            if avdist < 120 then                               
                                -- Check for better directions
                                local newangle = getbestangle(self.scanarray)
                                local absangle = newangle
                                
                                if absangle > 180 then
                                    -- Go left
                                    absangle = 360 - absangle
                                    robot.motor.setspeed(0, self.slowspeed)
                                    print(string.format("Going %d deg to left", absangle))
                                else
                                    -- Go right
                                    robot.motor.setspeed(self.slowspeed, 0)
                                    print(string.format("Going %d deg to right", absangle))
                                end
                                
                                self.turntime = gettimems() +
                                                robot.driverotatetime(self.slowspeed,
                                                                      absangle)
                            end
                        end
                        
                        -- UNDONE: Base delay on presence of obstacles, correct for turning
                        self.scandelay = gettimems() + math.random(1000, 3000)
                        self.checkslow = true
                    else                      
                        if isvalidscandata(self.scanarray[curscanangle]) then
                            local frontscan = (curscanangle > 350 or curscanangle < 10)
                            local avdist = math.median(self.scanarray[curscanangle])
                            
                            if frontscan and avdist < 40 then
                                return true, scanTask
                            end
                            
                            if self.checkslow and self.turntime == 0 and
                               (curscanangle >= 330 or curscanangle <= 30) then
                                local goslow = false
                            
                                if frontscan and avdist < 80 then
                                    goslow = true
                                elseif avdist < 25 then
                                    goslow = true
                                end
                                    
                                if goslow ~= self.moveslow then
                                    if goslow then
                                        robot.motor.setspeed(self.slowspeed, self.slowspeed)
                                        self.checkslow = false -- Only check once each scan
                                    else
                                        robot.motor.setspeed(self.drivespeed, self.drivespeed)
                                    end
                                    self.moveslow = goslow
                                end
                            else
                                -- UNDONE: Wall following
                            end
                        end
                        
                        self.scandelay = gettimems() + (30 / robot.servospeed())
                        self.scantime = self.scandelay + 200
                        robot.setservo(newspos)
                        self.servopos = newspos
                    end
                end
            end
        end        
        return false
    end,

    finish = function(self)
        if self.servopos ~= 90 then
            robot.setservo(90)
        end
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
                local targetangle = getbestangle(self.scanarray, 100)
                
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
    robot.setacs("low")
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
