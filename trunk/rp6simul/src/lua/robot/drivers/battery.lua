local ret = driver(...)

description = "Driver for the battery ADC."

local noiseFunction


function initPlugin()
    noiseFunction = makeADCNoiseFunction(100, 200, -2, 2)
end

function getADCValue(a)
    if a == "UBAT" then
        return 900 + noiseFunction()
    end

    -- return nil
end

return ret
