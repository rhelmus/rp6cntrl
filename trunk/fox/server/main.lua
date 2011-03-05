dofile "utils.lua"

require "robot"
require "nav"

-- External script functions
local curscript = nil

-- Default delay times (ms). Restored when scripts finish. Zero == disabled
local defaultserialdelays =
{
    { "led", 1000 },
    { "light", 500 },
    { "motor", 1000 },
    { "battery", 2000 },
    { "acs", 500 },
    { "mic", 0 },
    { "rc5", 500 },
    { "sharpir", 500 },
}
    

local function calloptscriptfunc(f, ...)
    -- Use rawget to avoid obtaining data from global environment
    local func = rawget(curscript, f)
    if func then
        func(...)
    end
end

local function restoreserialdelays()
    for _, t in ipairs(defaultserialdelays) do
        exec(string.format("set slavedelay %s %d", t[1], t[2]))
    end
end

function runscript(s)
    local stat, ret = pcall(assert(loadstring(s)))

    if stat then
        curscript = ret
    else
        print("Failed to run script:", ret)
        return
    end
    
    curscript.corun = coroutine.create(curscript.run)
    
    calloptscriptfunc("init")

    scriptrunning(true)
end

function think()
    if curscript then
        assert(coroutine.resume(curscript.corun))
        if coroutine.status(curscript.corun) == "dead" then
            calloptscriptfunc("finish")
            curscript = nil
            restoreserialdelays()
            scriptrunning(false)
        end
    end 
end

function execcmd(cmd, ...)
    print("Executing script cmd:", cmd)
    
    if cmd == "abortcurrentscript" then
        if curscript then
            calloptscriptfunc("finish")
            curscript = nil
            restoreserialdelays()
            scriptrunning(false)
        end
    elseif curscript then
        calloptscriptfunc("handlecmd", cmd, ...)
    end
end

function setserialdelay(var, delay)
    exec(string.format("set slavedelay %s %d", var, delay))
end

function initclient()
    print("initclient:", tostring(curscript ~= nil))
    scriptrunning(curscript ~= nil)
    if curscript then
        calloptscriptfunc("initclient")
    else
        restoreserialdelays()
    end
end

function makescript()
    local ret = { }
    setmetatable(ret, { __index = _G })
    setfenv(2, ret)
    return ret
end
