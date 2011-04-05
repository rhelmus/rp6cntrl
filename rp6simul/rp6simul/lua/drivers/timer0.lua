module(..., package.seeall)

handledTypes = { avr.IO_TCCR0, avr.IO_OCR0 }

local timer

function initPlugin()
    timer = clock.createTimer()
    timer:setPrescaler(1)
    timer:setCompareValue(2000)
--    timer:setTimeOut(function () print("Hi there ") end)
    timer:setTimeOut(avr.ISR_TIMER0_COMP_vect)
    clock.enableTimer(timer, true)
end

function handleIOData(type, data)
    if type == avr.TCCR0 then
        
    elseif type == avr.OCR0 then
        clock.setCompareValue(data)
    end
end

-- UNDONE! Reset timer
