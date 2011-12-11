function getVarargString(s, ...)
    s = tostring(s)

    for i=1, select("#", ...) do
        s = s .. "\t" .. tostring(select(i, ...))
    end

    return s
end

function callOptTabFunc(t, f, ...)
    -- Use rawget to avoid obtaining data from global environment
    local func = rawget(t, f)
    if func then
        return func(...)
    end
end

function fileExists(f)
    local f = io.open(f, "r")
    if not f then
        return false
    end
    f:close()
    return true
end

function tableIsEmpty(t)
    return next(t) == nil
end

function selectOne(n, ...)
    local ret = select(n, ...)
    return ret
end

function round(v)
    return math.floor(v + 0.5)
end

function bound(min, v, max)
    if v > max then
        return max
    elseif v < min then
        return min
    else
        return v
    end
end

function makeADCNoiseFunction(mindelay, maxdelay, minnoise, maxnoise)
    local nextupdate = 0
    local ret = 0

    return function()
        local curtimems = clock.getTimeMS()
        if nextupdate < curtimems then
            nextupdate = curtimems + math.random(mindelay, maxdelay)
            ret = math.random(minnoise, maxnoise)
        end
        return ret
    end
end

-- Convenience log functions
function log(s, ...)
--    debugLog(string.format("LOG: %s", getVarargString(s, ...)))
    appendLogOutput("log", nil, getVarargString(s, ...))
end

function debugLog(s, ...)
    print("Lua DEBUG:", getVarargString(s, ...))
    io.flush()
end

function warning(s, ...)
    debugLog(string.format("WARNING: %s", getVarargString(s, ...)))
    appendLogOutput("warning", nil, getVarargString(s, ...))
end

function errorLog(s, ...)
    debugLog(string.format("ERROR: %s", getVarargString(s, ...)))
    appendLogOutput("error", nil, getVarargString(s, ...))
end

