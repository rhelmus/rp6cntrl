local ret = driver(...)

description = "Driver for the light sensors."


local leftLightSensor, rightLightSensor

function initPlugin()
    leftLightSensor = createLightSensor(properties.leftLDR.pos)
    rightLightSensor = createLightSensor(properties.rightLDR.pos)
end

function getADCValue(a)
    if a == "LS_L" then
        local l = leftLightSensor:getLight()
        return l
    elseif a == "LS_R" then
        local l = rightLightSensor:getLight()
        return l
    end

    -- return nil
end


return ret
