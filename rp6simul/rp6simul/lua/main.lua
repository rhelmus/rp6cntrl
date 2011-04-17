local driverList = { }
local IOHandleMap = { }

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
        func(...)
    end
end

local function basename(p)
    return string.gsub(p, "/*.+/", "")
end

local function dirname(p)
    return string.match(p, "/*.*/")
end

local function loadDriver(d)
    local dir = dirname(d) or "lua/drivers/" -- UNDONE: Default path
    local file = basename(d) .. ".lua"

    local chunk, stat = loadfile(dir .. file)
    if not chunk then
        errorLog(string.format("Failed to load driver %s%s (%s)\n", dir, file, stat))
        return
    end

    local stat, driver = pcall(chunk, basename(d))
    if not stat then
        errorLog(string.format("Failed to execute driver %s%s\n", dir, file))
        return
    end

    if not driver then
        errorLog(string.format("No driver returned from %s!\n", basename(d)))
        return
    end

    if driver.handledIORegisters then
        for _, v in ipairs(driver.handledIORegisters) do
            IOHandleMap[v] = IOHandleMap[v] or { }
            table.insert(IOHandleMap[v], driver)
        end

        table.insert(driverList, driver)
    end
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


-- Functions called by C++ code
function init()
    -- UNDONE: Need this?
end

function initPlugin()
    -- Load standard drivers
    -- UNDONE: Use project settings
    loadDriver("timer0")
    loadDriver("timer1")
    loadDriver("timer2")
    loadDriver("motor")
    loadDriver("uart")
    loadDriver("portlog")
    loadDriver("led")
    loadDriver("adc")

    for _, d in ipairs(driverList) do
        callOptDriverFunc(d, "initPlugin")
    end
end

function closePlugin()
    for _, d in ipairs(driverList) do
        callOptDriverFunc(d, "closePlugin")
    end
    driverList = { }
    IOHandleMap = { }
    collectgarbage("collect") -- Force removal of drivers
end

function handleIOData(type, data)
    if IOHandleMap[type] then
        for _, d in ipairs(IOHandleMap[type]) do
            d.handleIOData(type, data)
        end
    end
end

function getDriverList()
    -- UNDONE
    local ret = { }
    ret["leds"] = "Driver for the RP6 leds."
    return ret
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
