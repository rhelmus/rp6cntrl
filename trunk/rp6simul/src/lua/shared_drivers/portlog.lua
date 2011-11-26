local ret = driver(...)

description = "Driver that logs all port changes."

handledIORegisters = {
    avr.IO_PORTA,
    avr.IO_PORTB,
    avr.IO_PORTC,
    avr.IO_PORTD,
    avr.IO_DDRA,
    avr.IO_DDRB,
    avr.IO_DDRC,
    avr.IO_DDRD,
    avr.IO_PINA,
    avr.IO_PINB,
    avr.IO_PINC,
    avr.IO_PIND
}

local portLogInfo = {
    oldPORTA = 0,
    oldPORTB = 0,
    oldPORTC = 0,
    oldPORTD = 0,
    oldDDRA = 0,
    oldDDRB = 0,
    oldDDRC = 0,
    oldDDRD = 0,
    oldPINA = 0,
    oldPINB = 0,
    oldPINC = 0,
    oldPIND = 0
}

function handleIOData(type, data)
    local function checkdiff(name, flagpre)
        local oldpstr = string.format("old%s", name)
        local oldp = portLogInfo[oldpstr]
        if data ~= oldp then
            log(string.format("%s changed:\n", name))
            for n=0,7 do
                local flagstr = string.format("%s%d", flagpre, n)
                local flag = avr[flagstr]
                if bit.isSet(data, flag) ~= bit.isSet(oldp, flag) then
                    log(string.format("\t%s %s\n", flagstr,
                        (bit.isSet(data, flag) and "enabled") or "disabled"))
                end
            end
            portLogInfo[oldpstr] = data
        end
    end

    if type == avr.IO_PORTA then
        checkdiff("PORTA", "PA")
    elseif type == avr.IO_PORTB then
        checkdiff("PORTB", "PB")
    elseif type == avr.IO_PORTC then
        checkdiff("PORTC", "PC")
    elseif type == avr.IO_PORTD then
        checkdiff("PORTD", "PD")
    elseif type == avr.IO_DDRA then
        checkdiff("DDRA", "DDA")
    elseif type == avr.IO_DDRB then
        checkdiff("DDRB", "DDB")
    elseif type == avr.IO_DDRC then
        checkdiff("DDRC", "DDC")
    elseif type == avr.IO_DDRD then
        checkdiff("DDRD", "DDD")
    elseif type == avr.IO_PINA then
        checkdiff("PINA", "PINA")
    elseif type == avr.IO_PINB then
        checkdiff("PINB", "PINB")
    elseif type == avr.IO_PINC then
        checkdiff("PINC", "PINC")
    elseif type == avr.IO_PIND then
        checkdiff("PIND", "PIND")
    end
end

return ret
