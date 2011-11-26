local ret = driver(...)

-- Left bumper: PINB, pin 0 (same as LED6)
-- Right bumper: PINC, pin 6 (same as LED3)

description = "Driver for the bumpers."


local bumperLeft, bumperRight


function initPlugin()
    local function makecallback(register, pin)
        return function (e)
            local data = avr.getIORegister(register)
            if e then
                avr.setIORegister(register, bit.set(data, pin))
            else
                avr.setIORegister(register, bit.unSet(data, pin))
            end
        end
    end

    bumperLeft = createBumper(properties["bumperLeft"].points,
                              properties["bumperLeft"].color)
    bumperLeft:setCallback(makecallback(avr.IO_PINB, avr.PINB0))

    bumperRight = createBumper(properties["bumperRight"].points,
                               properties["bumperRight"].color)
    bumperRight:setCallback(makecallback(avr.IO_PINC, avr.PINC6))
end

return ret
