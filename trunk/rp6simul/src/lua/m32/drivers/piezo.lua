local ret = driver(...)

description = "Piezo (beeper) driver."

handledIORegisters = {
    avr.IO_TCCR2,
    avr.IO_OCR2,
}


local piezoInfo =
{
    enabled = false,
    prescaler = 0,
    pitch = 0,
}


-- Copy of function defined in timer2.lua
local function getPrescaler(data)
    local clockSet0 = bit.isSet(data, avr.CS20)
    local clockSet1 = bit.isSet(data, avr.CS21)
    local clockSet2 = bit.isSet(data, avr.CS22)

    if not clockSet0 and not clockSet1 and not clockSet2 then
        return 0
    elseif clockSet0 and not clockSet1 and not clockSet2 then
        return 1
    elseif not clockSet0 and clockSet1 and not clockSet2 then
        return 8
    elseif clockSet0 and clockSet1 and not clockSet2 then
        return 32
    elseif not clockSet0 and not clockSet1 and clockSet2 then
        return 64
    elseif clockSet0 and not clockSet1 and clockSet2 then
        return 128
    elseif not clockSet0 and clockSet1 and clockSet2 then
        return 256
    elseif clockSet0 and clockSet1 and clockSet2 then
        return 1024
    end

    -- return nil
end

local function updateFreq()
    -- Factor two: piezo timer is set as toggle
    local freq
    if piezoInfo.pitch ~= 0 and piezoInfo.prescaler ~= 0 then
        freq = properties.clockSpeed / (2*piezoInfo.prescaler*(piezoInfo.pitch+1))
    else
        freq = 0
    end
    setBeeperFrequency(piezoInfo.pitch, freq)
    updateRobotStatus("piezo", "pitch", piezoInfo.pitch)
    updateRobotStatus("piezo", "frequency", freq)
end


function handleIOData(type, data)
    if type == avr.IO_TCCR2 then
        local e = bit.isSet(data, avr.WGM21, avr.COM20)
        if e ~= piezoInfo.enabled then
            piezoInfo.enabled = e
            setBeeperEnabled(e)
            updateRobotStatus("piezo", "enabled", tostring(e))
        end

        local ps = getPrescaler(data)
        if ps ~= piezoInfo.prescaler then
            piezoInfo.prescaler = ps
            updateFreq()
        end
    elseif type == avr.IO_OCR2 then
        piezoInfo.pitch = data
        updateFreq()
    end
end


return ret
