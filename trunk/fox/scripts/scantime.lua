local ret = makescript()

local function cleanscandata(scan)
    local maxerr = #scan.set * 0.25 -- Max 25% 'wrong'
    local errfound = 0
    
    local median = math.median(scan.set)
    
    local i = 1
    while i < #scan.set do
        if scan.set[i] < 20 or scan.set[i] > 150 or
           math.abs(scan.set[i] - median) > 30 then
            table.remove(scan.set, i)
            table.remove(scan.settimes, i)
            errfound = errfound + 1
            if errfound > maxerr then
                return false
            end
        else
            i = i + 1
        end
    end
    
    return true
end

function init()
    math.randomseed(os.time())
end

function run()
    local scanresults = { }
    local oldspos, curspos = 0, 0
    
    robot.setservo(0)
    waityield(1500)
    
    for i = 1, 50 do
        scanresults[i] = { }
        
        local scantime = 3000
        local lifetime = gettimems() + scantime
        local delay = 0
        local dspos = curspos - oldspos -- UNDONE: Make absolute?
        
        scanresults[i][dspos] = { set = { }, settimes = { } }
        
        while lifetime >= gettimems() do
            if delay < gettimems() then
--                 table.insert(scanresults[i][dspos], robot.sensors.sharpir())
                local stime = scantime - (lifetime - gettimems())
                table.insert(scanresults[i][dspos].set, robot.sensors.sharpir())
                table.insert(scanresults[i][dspos].settimes, stime)
                delay = gettimems() + 50
            end
            coroutine.yield()
        end
        
        oldspos = curspos
        curspos = math.random(0, 180)
        while math.abs(curspos - oldspos) < 20 do
            curspos = math.random(0, 180)
        end
        
        robot.setservo(curspos)
    end
    
    local file = io.open("scanresults.csv", "w")
    file:write("dServoPos;time;dist;RSD;accuracy\n")
    for i in ipairs(scanresults) do
        for dspos in pairs(scanresults[i]) do
            local validset = cleanscandata(scanresults[i][dspos])
            if not validset then
                --file:write(string.format("%d invalid\n", dspos))
                print(string.format("[%d][%d] invalid", i, dspos))
            else
                local mean = math.median(scanresults[i][dspos].set)
                for j, dist in ipairs(scanresults[i][dspos].set) do
                    local RSD, accuracy = 0, 100
                    if j > 1 then
                        local pset = table.part(scanresults[i][dspos].set, j)
                        RSD = math.stdev(pset) / math.mean(pset) * 100
                        accuracy = math.round(math.mean(pset) / mean * 100)
                        if accuracy >= 95 and accuracy <= 105 then
                            if not scanresults[i][dspos].stabletime then
                                scanresults[i][dspos].stabletime = scanresults[i][dspos].settimes[j]
                            end
                        else
                            scanresults[i][dspos].stabletime = nil
                        end
                    end
                    file:write(string.format("%d;%d;%d;%f;%d\n", dspos,
                            scanresults[i][dspos].settimes[j], dist, RSD, accuracy))
                end
            end
        end
    end
    
    file:write("\n\ndServoPos;stabletime\n")
    
    for i in ipairs(scanresults) do
        for dspos in pairs(scanresults[i]) do
            if scanresults[i][dspos].stabletime then
                file:write(string.format("%d;%d\n", dspos,
                        scanresults[i][dspos].stabletime))
            end
        end
    end
    
    file:close()
end

--[[
function run()
    local scanresults = { }
    for i = 1, 4 do
        scanresults[i] = { }
        for spos = 0, 180, 15 do
            robot.setservo(spos)
            local lifetime = gettimems() + 1500
            local delay = 0
            
            scanresults[i][spos] = { }
            
            while lifetime >= gettimems() do
                if delay < gettimems() then
                    table.insert(scanresults[i][spos], robot.sensors.sharpir())
                    delay = gettimems() + 50
                end
                coroutine.yield()
            end
        end
    end
    
    local file = io.open("scanresults.csv", "w")
    for i in ipairs(scanresults) do
        for spos, set in pairs(scanresults[i]) do
            if not isvalidscandata(set) then
                file:write(string.format("%d invalid\n", spos))
            else
                for _, j in ipairs(set) do
                    file:write(string.format("%d,%d\n", spos, j))
                end
            end
        end
    end
end
--]]

return ret
