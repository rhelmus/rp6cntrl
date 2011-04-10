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


local function loadDriver(d)
    local driver = require(d)
    
    if driver.handledIORegisters then
        for _, v in ipairs(driver.handledIORegisters) do
            IOHandleMap[v] = IOHandleMap[v] or { }
            table.insert(IOHandleMap[v], driver)
        end
        
        table.insert(driverList, driver)
    end
end

function init()
    package.path = "lua/drivers/?.lua"

    -- Load standard drivers (UNDONE: Make optional, ie move to initPlugin)
    loadDriver("timer0")
    loadDriver("timer1")
    loadDriver("timer2")
    loadDriver("motor")
    loadDriver("uart")
end

function initPlugin()
    for _, d in ipairs(driverList) do
        callOptDriverFunc(d, "initPlugin")
    end
end

function handleIOData(type, data)
    if IOHandleMap[type] then
        for _, d in ipairs(IOHandleMap[type]) do
            d.handleIOData(type, data)
        end
    end
end

