--Generate access functions
for k, v in pairs(sensortable) do
    _G[string.format("get%s", k)] = function()
        return sensortable[k]
    end
end

local oldprint = print
function print(s, ...)
    oldprint(s, ...)

    if ... then
        sendtext(s .. table.concat({...}, "\t"))
    else
        sendtext(s)
    end
end

local curscript = nil
function runscript(s)
    curscript = assert(loadstring(s))()
    if curscript.init then
        curscript.init()
    end

    curscript.run()

    if curscript.finish then
        curscript.finish()
    end

    curscript = nil
end

function execcmd(cmd, ...)
    print("Executing script cmd:", cmd)
    if curscript and curscript.handlecmd then
        curscript.handlecmd(cmd, ...)
    end
end

function makescript()
    local ret = { }
    setmetatable(ret, { __index = _G })
    setfenv(0, ret)
    return ret
end
