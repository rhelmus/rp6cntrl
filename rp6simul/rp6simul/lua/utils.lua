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

function baseName(p)
    return string.gsub(p, "/*.+/", "")
end

function dirName(p)
    return string.match(p, "/*.*/")
end

function tableIsEmpty(t)
    return next(t) == nil
end

function selectOne(n, ...)
    local ret = select(n, ...)
    return ret
end


-- Convenience log functions
function log(s, ...)
--    debugLog(string.format("LOG: %s", getVarargString(s, ...)))
    appendLogOutput("LOG", getVarargString(s, ...))
end

function debugLog(s, ...)
    print("Lua DEBUG:", getVarargString(s, ...))
    io.flush()
end

function warning(s, ...)
    debugLog(string.format("WARNING: %s", getVarargString(s, ...)))
    appendLogOutput("WARNING", getVarargString(s, ...))
end

function errorLog(s, ...)
    debugLog(string.format("ERROR: %s", getVarargString(s, ...)))
    appendLogOutput("ERROR", getVarargString(s, ...))
end

