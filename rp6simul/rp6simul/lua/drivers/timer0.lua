module(..., package.seeall)

handledTypes = { avr.UCSRA, avr.UDR }

function initPlugin()
    clock.setPrescaler(clock.TIMER0, 1)
end

function handleIOData(type, data)
    if type == avr.TCCR0 then
        
    elseif type == avr.OCR0 then
        clock.setCompareValue(data)
    end
end
