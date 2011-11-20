local ret = driver(...)

description = "Driver for the microphone."

handledIORegisters = {
    avr.IO_DDRA,
}


local MICInfo =
{
    currentSoundLevel = 0,
}


local function handleSound(l)
    local level
    -- UNDONE: Randomize?
    -- UNDONE: Levels OK?
    if l == "soft" then
        level = 40
    elseif l == "normal" then
        level = 100
    elseif l == "loud" then
        level = 200
    end

    if MICInfo.currentSoundLevel < level then
        MICInfo.currentSoundLevel = level
    end
end



function initPlugin()
    setSoundCallback(handleSound)
end

function handleIOData(type, data)
    if bit.isSet(data, avr.PINA0) then
        MICInfo.currentSoundLevel = 0 -- Discharge
    end
end

function getADCValue(a)
    if a == "MIC" then
        return MICInfo.currentSoundLevel
    end

    -- return nil
end

return ret
