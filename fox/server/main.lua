--Generate access functions
for k, v in pairs(sensortable) do
    _G[string.format("get%s", k)] = function()
        return sensortable[k]
    end
end

