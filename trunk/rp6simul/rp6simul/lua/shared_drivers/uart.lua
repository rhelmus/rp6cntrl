local ret = driver(...)

-- UNDONE: Receiving (i.e. handling user input)(don't forgot receiverEnabled/enabled!)

description = "Driver for the UART (serial input/output)."

handledIORegisters = {
    avr.IO_UCSRA,
    avr.IO_UCSRB,
    avr.IO_UCSRC,
    avr.IO_UDR,
    avr.IO_UBRR,
    avr.IO_UBRRL,
    avr.IO_UBRRH
}

local UARTInfo = {
    enabled = true,
    receiverISREnabled = false,
    receiverEnabled = false,
    receivedData = 0, -- Last received byte
    transmitterEnabled = false,
    baudRate = 0,
    dataSend = 0,
    dataSendPerSec = 0,
    dataReceived = 0
}

local function checkCond(cond, msg)
    if not cond then
        warning(msg)
        if UARTInfo.enabled then
            warning("Disabling UART driver.\n")
            UARTInfo.enabled = false
        end
    end
    return cond
end

local function setCntrlStatRegisterA(data)
    local cond = (bit.unSet(data, avr.UDRE) == 0)
    if cond and not bit.isSet(data, avr.UDRE) then
        -- We're always ready
        avr.setIORegister(avr.IO_UCSRA, bit.set(0, avr.UDRE))
        log("Restored UART UDRE flag\n")
    end

    return checkCond(cond, "Unsupported settings found for UCSRA.\n")
end

local function setCntrlStatRegisterB(data)
    local cond = (bit.unSet(data, avr.RXCIE, avr.RXEN, avr.TXEN) == 0)

    if cond then
        local function update(lvar, avar, desc)
            if UARTInfo[lvar] ~= bit.isSet(data, avr[avar]) then
                UARTInfo[lvar] = bit.isSet(data, avr[avar])
                local stat = (UARTInfo[lvar] and "enabled") or "disabled"
                log(string.format("UART setting %s %s\n", avar, stat))
                updateRobotStatus("UART", desc, stat)
            end
        end

        update("receiverISREnabled", "RXCIE", "Receiver interrupt")
        update("receiverEnabled", "RXEN", "receiver")
        update("transmitterEnabled", "TXEN", "transmitter")
    end

    return checkCond(cond, "Unsupported settings found for UCSRB.\n")
end

local function setCntrlStatRegisterC(data)
    local cond = (bit.unSet(data, avr.URSEL, avr.UCSZ0, avr.UCSZ1) == 0)
    return checkCond(cond, "Unsupported settings found for UCSRC.\n")
end

local function setDataRegister(data)
    -- The UDR register is used both for receiving and transmitting data. From
    -- my understanding reading UDR always gives the received data and writing
    -- always transfers data. To emulate this, the register is reset back to the
    -- received data as soon as something was written to it.
    avr.setIORegister(avr.IO_UDR, UARTInfo.receivedData)

    if UARTInfo.transmitterEnabled then
        -- Can/want/need we to buffer this?
        local ch = string.char(data)
        log("ch:", ch, data, "\n")
        appendSerialOutput(ch)
        UARTInfo.dataSend = UARTInfo.dataSend + 1
        updateRobotStatus("UART", "Data send", UARTInfo.dataSend)
    end

    avr.setIORegister(avr.IO_UCSRA, bit.set(0, avr.UDRE))
end

local function setBaudRate(data)
    -- Baud rate is set as: (freq / (16 * Baud rate)) - 1
    -- Ref: http://extremeelectronics.co.in/avr-tutorials/using-the-usart-of-avr-microcontrollers/
    -- Note that we do not use it yet, but this will be usefull when
    -- virtual serial ports are supported.

    -- UNDONE: There seems to be a little rounding (int cutoff) error
    -- when calculating back to the desired bps.
    local baud = (properties.clockSpeed / (data + 1)) / 16
    log(string.format("Setting UART baud rate to %d (%d bps)\n", data, baud))
    updateRobotStatus("UART", "Baudrate (bps)", string.format("%d", baud))
    UARTInfo.baudRate = data
end

local function sendSerial(text)
    if UARTInfo.receiverEnabled and UARTInfo.receiverISREnabled then
        local lastch
        for ch in text:gmatch(".") do
            avr.setIORegister(avr.IO_UDR, string.byte(ch))
            avr.execISR(avr.ISR_USART_RXC_vect)
            lastch = ch
        end
        UARTInfo.receivedData = string.byte(lastch)
    end
end


function initPlugin()
    -- Apply initial UART settings
    -- UNDONE: Move out of driver
    avr.setIORegister(avr.IO_UCSRA, bit.set(0, avr.UDRE))
    avr.setIORegister(avr.IO_UCSRC, bit.set(0, avr.URSEL, avr.UCSZ0, avr.UCSZ1))

    -- Update data register even when set with previous value!
    avr.setIORegisterIgnoreEqual(avr.IO_UDR, true)

    setSerialSendCallback(sendSerial)
end

function handleIOData(type, data)
    if UARTInfo.enabled then
        if type == avr.IO_UCSRA then
            setCntrlStatRegisterA(data)
        elseif type == avr.IO_UCSRB then
            setCntrlStatRegisterB(data)
        elseif type == avr.IO_UCSRC then
            setCntrlStatRegisterC(data)
        elseif type == avr.IO_UDR then
            setDataRegister(data)
        elseif type == avr.IO_UBRR then
            setBaudRate(data)
        elseif type == avr.IO_UBRRL then
            setBaudRate(bit.unPack(data, bit.upper(UARTInfo.baudRate, 8), 8))
        elseif type == avr.IO_UBRRH then
            setBaudRate(bit.unPack(UARTInfo.baudRate, data, 8))
        end
    end
end


return ret
