-- LuaJIT compat fix: http://lua-users.org/lists/lua-l/2010-09/msg00161.html
os.setlocale("C", "numeric")


dofile(getLuaSrcPath("utils.lua"))


local simulatorEnv = nil
local simulatorPath = nil
local driverList = { }
-- Used to check if drivers are properly destructed
local weakDriverList = setmetatable({ }, { __mode = "kv" })
local IOHandleMap = { }
local exportedDriverSyms = { }


-- Local utilities

local function loadSimulator(s)
    local file = getLuaSrcPath(string.format("%s/%s.lua", s, s))

    local chunk, stat = loadfile(file)
    if not chunk then
        errorLog(string.format("Failed to load %s simulator: %s\n", s, stat))
        return nil
    end

    local stat, sim = pcall(chunk, s)
    if not stat then
        errorLog(string.format("Failed to execute %s simulator: %s\n", s, sim))
        return nil
    end

    if not sim then
        errorLog(string.format("No simulator returned from %s!\n", file))
        return nil
    end

    log(string.format("Succesfully loaded simulator %s (%s)\n", s, file))

    return sim
end

local function loadDriver(d)
    local dir = dirName(d)
    local file = baseName(d)

    if not file:match("%.lua", -4) then
        file = file .. ".lua"
    end

    local path

    -- Pick a default directory?
    if not dir then
        path = joinPath(simulatorPath, "drivers", file)
        if not fileExists(path) then
            path = getLuaSrcPath("shared_drivers/" .. file)
        end
    else
        path = d
    end

    local chunk, stat = loadfile(path)
    if not chunk then
        errorLog(string.format("Failed to load driver %s (%s)\n", path, stat))
        return nil
    end

    local name = file:gsub(".lua", "")
    local stat, driver = pcall(chunk, name)
    if not stat then
        errorLog(string.format("Failed to execute driver %s: %s\n", path, driver))
        return nil
    end

    if not driver then
        errorLog(string.format("No driver returned from %s!\n", baseName(d)))
        return nil
    end

    log(string.format("Succesfully loaded driver %s (%s)\n", name, path))

    return driver
end

local function initDriver(d)
    local driver = loadDriver(d)

    if not driver then
        return
    end

    if driver.handledIORegisters then
        for _, v in ipairs(driver.handledIORegisters) do
            IOHandleMap[v] = IOHandleMap[v] or { }
            table.insert(IOHandleMap[v], driver)
        end
    end

    table.insert(driverList, driver)
    table.insert(weakDriverList, driver)

    callOptTabFunc(driver, "initPlugin")

    addDriverLoaded(baseName(d))
end


-- ADC values can either be set by the user interface program
-- or from drivers. The former overrides the latter.
local UIADCValues = { }

-- Function called by drivers to get ADC values
function getADCValue(a)
    if UIADCValues[a] then
        return UIADCValues[a]
    end

    -- No overridden ADC value, see if a driver has one
    for _, d in ipairs(driverList) do
        local val = callOptTabFunc(d, "getADCValue", a)
        if val then
            return round(val)
        end
    end

    -- Nothing...
    return 0
end


-- Functions called by C++ code
function init(sim)
    math.randomseed(os.time())

    simulatorEnv = loadSimulator(sim)
    simulatorPath = getLuaSrcPath(sim)

    callOptTabFunc(simulatorEnv, "init")
end

function initPlugin(drivers)
    assert(tableIsEmpty(driverList) and tableIsEmpty(IOHandleMap))

    callOptTabFunc(simulatorEnv, "initPlugin")

    if drivers then
        for _, d in ipairs(drivers) do
            initDriver(d)
        end
    end
end

function closePlugin()
    callOptTabFunc(simulatorEnv, "closePlugin")

    for _, d in ipairs(driverList) do
        callOptTabFunc(d, "closePlugin")
    end

    driverList = { }
    IOHandleMap = { }

    -- Force removal of all collectable data. Since some operations may
    -- take more that one cycle, this function is repeatably called until
    -- no memory is freed.
    debugLog("pre-collect")
    collectgarbage("collect")
    local memleft = collectgarbage("count")
    debugLog("memleft:", memleft)
    while true do
        collectgarbage("collect")
        local m = collectgarbage("count")
        debugLog("memleft:", m)
        if m == memleft then
            break
        end
        memleft = m
    end
    debugLog("post-collect")

    if not tableIsEmpty(weakDriverList) then
        warning("Not all drivers were properly destroyed! Remaining drivers:\n")
        for k, v in pairs(weakDriverList) do
            warning(string.format("\t[%d] = %s\n", k, v.description))
        end
    end
end

function handleIOData(type, data)
    callOptTabFunc(simulatorEnv, "handleIOData", type, data)
    if IOHandleMap[type] then
        for _, d in ipairs(IOHandleMap[type]) do
            d.handleIOData(type, data)
        end
    end
end

function getDriverInfoList()
    local ret = { }

    for i, d in ipairs(simulatorEnv.driverList) do
        local driver = loadDriver(d.name)
        if driver then
            ret[i] = { }
            ret[i].name = d.name
            ret[i].description = driver.description or "No description."
            ret[i].default = d.default
        end
    end

    return ret
end

function getDriver(d)
    local driver = loadDriver(d)
    if driver then
        local name = baseName(d)
        name = name:gsub(".lua", "")
        return name, (driver.description or "No description")
    end
end

function setUIADCValue(a, v)
    debugLog(string.format("setUIADCValue: %s = %s\n", a, tostring(v)))
    UIADCValues[a] = v
end

function getADCValues()
    local ret = { }
    local ports = simulatorEnv.getADCPortNames()

    for _, p in ipairs(ports) do
        ret[p] = getADCValue(p)
    end

    return ret
end


-- Called by every simulator (robot, m32)
function simulator(name)
    local ret = { }
    setmetatable(ret, { __index = _G })

    -- Neater logging
    -- UNDONE: need this?
    --    ret.log = function(s, ...) log(string.format("<%s> %s", name, s), ...) end
    --    ret.debugLog = function(s, ...) debugLog(string.format("<%s> %s", name, s), ...) end
    --    ret.warning = function(s, ...) warning(string.format("<%s> %s", name, s), ...) end
    --    ret.errorLog = function(s, ...) errorLog(string.format("<%s> %s", name, s), ...) end

    setfenv(2, ret)
    return ret
end


-- Called by every driver
function driver(name)
    local ret = { }
    setmetatable(ret, { __index = simulatorEnv })

    -- Neater logging
    ret.log = function(s, ...) appendLogOutput("log", name, getVarargString(s, ...)) end
    ret.debugLog = function(s, ...) debugLog(string.format("<%s> %s", name, s), ...) end
    ret.warning = function(s, ...) appendLogOutput("warning", name, getVarargString(s, ...)) end
    ret.errorLog = function(s, ...) appendLogOutput("error", name, getVarargString(s, ...)) end

    setfenv(2, ret)
    return ret
end
