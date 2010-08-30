-- Various utility functions

-- Overide regular print to send text to clients
local oldprint = print
function print(s, ...)
    oldprint(s, ...)
    
    s = tostring(s)
    
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

function selectone(n, ...)
    local ret = select(n, ...)
    return ret
end

-- Math extensions
function math.sum(set)
    local sum = 0
    
    for _, v in ipairs(set) do
        sum = sum + v
    end

    return sum
end

function math.average(set)
    return math.sum(set) / #set
end

function math.median(set)
    local ret
    
    table.sort(set)
    
    if math.fmod(#set, 2) > 0 then
        local i = math.floor(#set / 2)
        ret = math.average{set[i], set[i+1]}
    else
        ret = set[#set / 2]
    end
    
    io.write("median: ")
    for _, v in ipairs(set) do
        io.write(string.format("%d, ", v))
    end
    print("\n== ", ret)
    
    return ret
end

function math.wrapangle(a)
    while a < 0 do
        a = a + 360
    end
    
    while a >= 360 do
        a = a - 360
    end
    
    return a
end
