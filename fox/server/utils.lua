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

function waityield(ms)
    local wait = gettimems() + ms
    while wait > gettimems() do
        coroutine.yield()
    end
end

-- Math extensions
function math.sum(set)
    local sum = 0
    
    for _, v in ipairs(set) do
        sum = sum + v
    end

    return sum
end

function math.mean(set)
    return math.sum(set) / #set
end

function math.median(set)
    local ret
    
    local mset = table.copy(set)
    table.sort(mset)
    
    if math.fmod(#mset, 2) > 0 then
        local i = math.floor(#mset / 2)
        ret = math.mean{mset[i], mset[i+1]}
    else
        ret = mset[#mset / 2]
    end
    
    io.write("median: ")
    for _, v in ipairs(mset) do
        io.write(string.format("%d, ", v))
    end
    print("\n== ", ret)
    
    return ret
end

function math.stdev(set)
    if #set < 1 then
        return 1
    end
    
    local mean = math.mean(set)
    local qsum = 0
    
    for _, v in ipairs(set) do
        qsum = qsum + math.pow(v - mean, 2)
    end
    
    return math.sqrt(qsum / (#set-1))
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

function math.round(v)
    return math.floor(v + 0.5)
end

function table.copy(t)
    local ret = { }
    for k, v in pairs(t) do
        ret[k] = v
    end
    return ret
end

function table.part(t, m, n)
    local s = (n and m) or 1
    local e = n or m
    local ret = { }
    for i = s, e do
        table.insert(ret, t[i])
    end
    return ret
end
