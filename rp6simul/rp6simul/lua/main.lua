-- LuaJIT compat fix: http://lua-users.org/lists/lua-l/2010-09/msg00161.html
os.setlocale("C", "numeric")

local driverList = { }
-- Used to check if drivers are properly destructed
local weakDriverList = setmetatable({ }, { __mode = "kv" })
local IOHandleMap = { }

-- Local utilities
local function getVarargString(s, ...)
    s = tostring(s)

    for i=1, select("#", ...) do
        s = s .. "\t" .. tostring(select(i, ...))
    end

    return s
end

local function callOptDriverFunc(driver, f, ...)
    -- Use rawget to avoid obtaining data from global environment
    local func = rawget(driver, f)
    if func then
        return func(...)
    end
end

local function basename(p)
    return string.gsub(p, "/*.+/", "")
end

local function dirname(p)
    return string.match(p, "/*.*/")
end

local function tableIsEmpty(t)
    return next(t) == nil
end

local function loadDriver(d)
    local dir = dirname(d) or "lua/drivers/" -- UNDONE: Default path
    local file = basename(d)

    if not file:match("%.lua", -4) then
        file = file .. ".lua"
    end

    local chunk, stat = loadfile(dir .. file)
    if not chunk then
        errorLog(string.format("Failed to load driver %s%s (%s)\n", dir, file, stat))
        return nil
    end

    local name = file:gsub(".lua", "")
    local stat, driver = pcall(chunk, name)
    if not stat then
        errorLog(string.format("Failed to execute driver %s%s: %s\n", dir, file, driver))
        return nil
    end

    if not driver then
        errorLog(string.format("No driver returned from %s!\n", basename(d)))
        return nil
    end

    log(string.format("Succesfully loaded driver %s (%s)\n", name, dir .. file))

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

    callOptDriverFunc(driver, "initPlugin")
end


-- Convenience log functions
function log(s, ...)
    appendLogOutput("LOG", getVarargString(s, ...))
end

function debug(s, ...)
    print("Lua DEBUG:", getVarargString(s, ...))
    io.flush()
end

function warning(s, ...)
    appendLogOutput("WARNING", getVarargString(s, ...))
end

function errorLog(s, ...)
    appendLogOutput("ERROR", getVarargString(s, ...))
end


-- Global driver functions
local serialInputHandler
function setSerialInputHandler(func)
    serialInputHandler = func
end

local IRCOMMSendHandler
function setIRCOMMSendHandler(func)
    IRCOMMSendHandler = func
end

-- ADC values can either be set by the user interface program
-- or from drivers. The former overrides the latter.
local UIADCValues = { }

function getADCValue(a)
    if UIADCValues[a] then
        return UIADCValues[a]
    end

    -- No overridden ADC value, see if a driver has one
    for _, d in ipairs(driverList) do
        local val = callOptDriverFunc(d, "getADCValue", a)
        if val then
            return val
        end
    end

    -- Nothing...
    return 0
end

function getADCPortNames()
    return { "ADC0", "ADC1", "LS_R", "LS_L", "E_INT1", "MCURRENT_R",
             "MCURRENT_L", "UBAT" }
end


-- Functions called by C++ code
function init()
    math.randomseed(os.time())
end

function initPlugin(drivers)
    assert(tableIsEmpty(driverList) and tableIsEmpty(IOHandleMap))

    if drivers then
        for _, d in ipairs(drivers) do
            initDriver(d)
        end
    end
end

function closePlugin()
    for _, d in ipairs(driverList) do
        callOptDriverFunc(d, "closePlugin")
    end

    serialInputHandler = nil
    IRCOMMSendHandler = nil

    driverList = { }
    IOHandleMap = { }

    -- Force removal of all collectable data. Since some operations may
    -- take more that one cycle, this function is repeatably called until
    -- no memory is freed.
    debug("pre-collect")
    collectgarbage("collect")
    local memleft = collectgarbage("count")
    debug("memleft:", memleft)
    while true do
        collectgarbage("collect")
        local m = collectgarbage("count")
        debug("memleft:", m)
        if m == memleft then
            break
        end
        memleft = m
    end
    debug("post-collect")

    if not tableIsEmpty(weakDriverList) then
        warning("Not all drivers were properly destroyed! Remaining drivers:\n")
        for k, v in pairs(weakDriverList) do
            warning(string.format("\t[%d] = %s\n", k, v.description))
        end
    end
end

function handleIOData(type, data)
    if IOHandleMap[type] then
        for _, d in ipairs(IOHandleMap[type]) do
            d.handleIOData(type, data)
        end
    end
end

function getDriverList()
    local drivers = { "timer0", "timer1", "timer2", "motor", "adc",
                      "led", "uart", "portlog", "acs" }
    local default = { timer0 = true, timer2 = true, motor = true,
                      adc = true, led = true, uart = true, acs = true }
    local ret, dret = { }, { }

    for _, d in ipairs(drivers) do
        local driver = loadDriver(d)
        if driver then
            ret[d] = driver.description or "No description."
            if default[d] then
                table.insert(dret, d)
            end
        end
    end

    return ret, dret
end

function getDriver(d)
    local driver = loadDriver(d)
    if driver then
        local name = basename(d)
        name = name:gsub(".lua", "")
        return name, (driver.description or "No description")
    end
end

function sendSerial(text)
    if serialInputHandler then
        serialInputHandler(text)
    end
end

function sendIRCOMM(adr, key, toggle)
    if IRCOMMSendHandler then
        IRCOMMSendHandler(adr, key, toggle)
    end
end

function setUIADCValue(a, v)
    debug(string.format("setUIADCValue: %s = %s\n", a, tostring(v)))
    UIADCValues[a] = v
end


-- Called by every driver
function driver(name)
    debug(string.format("Created driver for %s", tostring(name)))
    local ret = { }
    setmetatable(ret, { __index = _G })

    -- Neater logging
    ret.log = function(s, ...) log(string.format("<%s> %s", name, s), ...) end
    ret.debug = function(s, ...) debug(string.format("<%s> %s", name, s), ...) end
    ret.warning = function(s, ...) warning(string.format("<%s> %s", name, s), ...) end
    ret.errorLog = function(s, ...) errorLog(string.format("<%s> %s", name, s), ...) end

    setfenv(2, ret)
    return ret
end

dofile("lua/properties.lua")
