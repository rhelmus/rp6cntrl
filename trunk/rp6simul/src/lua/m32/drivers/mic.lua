local ret = driver(...)

description = "Driver for the microphone."

handledIORegisters = {
    avr.IO_DDRA,
}


local MICInfo =
{
    currentSoundLevel = 0,
}

local noiseFunction


local function handleSound(l)
    local level
    if l == "soft" then
        level = 200
    elseif l == "normal" then
        level = 500
    elseif l == "loud" then
        level = 900
    end

    level = level + math.random(-100, 100)

    if MICInfo.currentSoundLevel < level then
        MICInfo.currentSoundLevel = level
    end
end



function initPlugin()
    noiseFunction = makeADCNoiseFunction(50, 200, -10, 10)
    setSoundCallback(handleSound)
end

function handleIOData(type, data)
    if bit.isSet(data, avr.PINA0) then
        MICInfo.currentSoundLevel = 0 -- Discharge
    end
end

function getADCValue(a)
    if a == "MIC" then
        return bound(0, MICInfo.currentSoundLevel + noiseFunction(), 1023)
    end

    -- return nil
end

return ret
