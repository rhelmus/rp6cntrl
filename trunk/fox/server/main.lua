--[[
-- Generate access functions
for k, v in pairs(sensortable) do
    _G[string.format("get%s", k)] = function()
        return sensortable[k]
    end
end
--]]

-- Misc functions

local oldprint = print
function print(s, ...)
    oldprint(s, ...)
    
    for i=1, select("#", ...) do
        s = s .. "\t" .. tostring(select(i, ...))
    end

    sendtext(s .. "\n")
end

-- Similar to regular module function, but may extend existing module,
-- does no registration and always inherits environment
function extmodule(name)
    local M = _G[name]
    if not M then
        M = { }
        _G[name] = M
    end
    if not getmetatable(M) then
        setmetatable(M, { __index = _G })
    end
    setfenv(2, M)
end

require "robot"
require "nav"

-- External script functions
local curscript = nil

local function calloptscriptfunc(f, ...)
    -- Use rawget to avoid obtaining data from global environment
    local func = rawget(curscript, f)
    if func then
        func(...)
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
            scriptrunning(false)
        end
    elseif curscript then
        calloptscriptfunc("handlecmd", cmd, ...)
    end
end

function initclient()
    print("initclient:", tostring(curscript ~= nil))
    scriptrunning(curscript ~= nil)
    if curscript then
        calloptscriptfunc("initclient")
    end
end

function makescript()
    local ret = { }
    setmetatable(ret, { __index = _G })
    setfenv(2, ret)
    return ret
end
