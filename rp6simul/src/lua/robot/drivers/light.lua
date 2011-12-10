local ret = driver(...)

description = "Driver for the light sensors."


local leftLightSensor, rightLightSensor
local leftNoiseFunction, rightNoiseFunction

function initPlugin()
    leftLightSensor = createLightSensor(properties.leftLDR.pos)
    rightLightSensor = createLightSensor(properties.rightLDR.pos)
    leftNoiseFunction = makeADCNoiseFunction(50, 100, -4, 4)
    rightNoiseFunction = makeADCNoiseFunction(50, 100, -4, 4)
end

function getADCValue(a)
    if a == "LS_L" then
        local l = leftLightSensor:getLight() + leftNoiseFunction()
        return bound(0, l, 1023)
    elseif a == "LS_R" then
        local l = rightLightSensor:getLight() + rightNoiseFunction()
        return bound(0, l, 1023)
    end

    -- return nil
end


return ret
