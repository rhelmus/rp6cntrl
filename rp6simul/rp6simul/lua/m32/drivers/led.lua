local ret = driver(...)

description = "M32 LED driver."

handledIORegisters = {
    avr.IO_PORTD,
}


local LEDInfo =
{
    LEDs = { },
    LEDStatus = { false, false, false, false },
}


function handleIOData(type, data)
    if bit.isSet(data, avr.PIND4) then
        local reg = avr.getIORegister(avr.IO_SPDR)
        for l=1,4 do
            local e = bit.isSet(reg, l-1)
            if e ~= LEDInfo.LEDStatus[l] then
                log(string.format("LED %d %s\n", l, (e and "enabled") or "disabled"))
--                LEDInfo.LEDs[l]:setEnabled(e) UNDONE
                LEDInfo.LEDStatus[l] = e
                updateRobotStatus("leds", tostring(l), (e and "ON") or "OFF")
            end
        end
    end
end


return ret
