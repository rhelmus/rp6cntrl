local ret = driver(...)

description = "Driver for the battery ADC."

local noiseFunction


function initPlugin()
    noiseFunction = makeADCNoiseFunction(50, 200, -5, 5)
end

function getADCValue(a)
    if a == "UBAT" then
        return 900 + noiseFunction()
    end

    -- return nil
end

return ret
