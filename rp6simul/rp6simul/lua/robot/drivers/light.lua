local ret = driver(...)

description = "Driver for the light sensors."


local leftLightSensor, rightLightSensor

function initPlugin()
    leftLightSensor = createLightSensor(robotProperties.leftLDR.pos)
    rightLightSensor = createLightSensor(robotProperties.rightLDR.pos)
end

function getADCValue(a)
    -- UNDONE? robot status is only updated if ADC value is requested.
    -- Although this happens frequently when task_ADC() is called from the
    -- plugin.
    if a == "LS_L" then
        local l = leftLightSensor:getLight()
        updateRobotStatus("light", "left", l)
        return l
    elseif a == "LS_R" then
        local l = rightLightSensor:getLight()
        updateRobotStatus("light", "right", l)
        return l
    end

    -- return nil
end


return ret
