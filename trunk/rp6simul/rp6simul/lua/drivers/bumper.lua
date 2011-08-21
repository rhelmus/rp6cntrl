local ret = driver(...)

-- Left bumper: PINB, pin 0 (same as LED6)
-- Right bumper: PINC, pin 6 (same as LED3)

description = "Driver for the bumpers."


function initPlugin()
    setBumperHandler(function (b, e)
        if b == "left" then
            local data = avr.getIORegister(avr.IO_PINB)
            if e then
                avr.setIORegister(avr.IO_PINB, bit.set(data, avr.PINB0))
            else
                avr.setIORegister(avr.IO_PINB, bit.unSet(data, avr.PINB0))
            end
        elseif b == "right" then
            local data = avr.getIORegister(avr.IO_PINC)
            if e then
                avr.setIORegister(avr.IO_PINC, bit.set(data, avr.PINC6))
            else
                avr.setIORegister(avr.IO_PINC, bit.unSet(data, avr.PINC6))
            end
        end
    end)
end

return ret
