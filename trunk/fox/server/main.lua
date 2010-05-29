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
        sendtext(s .. table.concat(..., "\t"))
    else
        sendtext(s)
    end
end