local ret = driver(...)

-- UNDONE: m32 support

-- ADC ports (RP6)
--      0: ADC0
--      1: ADC1
--      2: Light right
--      3: Light left
--      5: Motor current right
--      6: Motor current left
--      7: Battery

handledIORegisters = {
    avr.IO_ADMUX,
    avr.IO_ADCSRA
}

local ADCInfo = {
    currentPort = 0,
    ADCEnabled = false,
}

local ACDData = { }

local function getADCPort(data)
    for p=0,7 do
        -- Skip 4: not used by RP6
        -- Mask by 7(0b111): Only look at first 3 bits
        if p ~= 4 and (bit.bitAnd(data, 7) == p) then
            return p
        end
    end
end

function initPlugin()
    for i=0,7 do
        ACDData[i] = 0
    end
end

function handleIOData(type, data)
    if type == avr.IO_ADMUX then
        local port = getADCPort(data)
        if not port then
            error("Unsupported ADC port specified (ignoring change)\n")
        elseif port ~= ADCInfo.currentPort then
            -- No logging...excessive with task_ADC() function...
--            log(string.format("Changed ADC port to %d\n", port))
            ADCInfo.currentPort = port
        end
    elseif type == avr.IO_ADCSRA then
        if ADCInfo.ADCEnabled ~= bit.isSet(data, avr.ADEN) then
            ADCInfo.ADCEnabled = bit.isSet(data, avr.ADEN)
            log(string.format("ADC %s\n", (ADCInfo.ADCEnabled and "enabled") or "disabled"))
        end

        if bit.isSet(data, avr.ADSC) then
            -- Start conversion
            avr.setIORegister(avr.IO_ADC, ACDData[ADCInfo.currentPort])
            -- Notify we're ready
            avr.setIORegister(avr.IO_ADCSRA, bit.unSet(data, avr.ADSC))
        end
    end
end

return ret
