local ret = driver(...)

description = "Driver for the m32 keypad."


local currentKeyPressed = nil


local function handleKeyPress(key)
    currentKeyPressed = key
    if key then
        log("Key pressed: ", key, "\n")
    else
        log("Key released\n")
    end
end


function initPlugin()
    setKeyPressCallback(handleKeyPress)
end

function getADCValue(a)
    if a == "KEYPAD" then
        if currentKeyPressed ~= nil then
            -- ADC levels based from lib
            -- UNDONE: Configurable?
            if currentKeyPressed == 1 then
                return 40
            elseif currentKeyPressed == 2 then
                return 500
            elseif currentKeyPressed == 3 then
                return 600
            elseif currentKeyPressed == 4 then
                return 750
            elseif currentKeyPressed == 5 then
                return 810
            end
        end
    end

    -- return nil
end

return ret
