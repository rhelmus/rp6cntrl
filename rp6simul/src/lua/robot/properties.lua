--[[
This table holds the positions (in px) and various
other properties of some of the robot peripherals.
The scale field is used for centimeter to pixel conversion, for example
to measure distances.

It is important to update this file whenever rp6-top.png is changed!

Data used for scale parameter:
RP6 (board) width: 11.3 cm
Using GIMP's ruler tool this should equal 565 px from the rp6-top.png image.

The bumper fields contain the different points needed to construct a polygon
for collision detection. These points are taken in such a way that they are
'outside' the bumper: due scaling artifacts of the robot image, the resulting
image gets a small 'border' around it. Thus to make sure that the bumpers are
really hit, a larger collision polygon must be used.
--]]

local robotWidthCm = 11.3
local robotWidthPx = 589
local cmPerPixel = robotWidthCm / robotWidthPx

local robotPicWidth = 630 -- Full width of rp6-top.png image

local green = { 0, 255, 0 }
local red = { 255, 0, 0 }
local yellow = { 255, 255, 0 }
local darkblue = { 0, 0, 192 }
local transblue = { 0, 0, 255, 80 }

-- In px. Real life measurements :)
local ACSDistances =
{
    low = 11 / cmPerPixel,
    medium = 23 / cmPerPixel,
    high = 32 / cmPerPixel
}
--187, 513
properties =
{
    clockSpeed = 8000000,
    cmPerPixel = cmPerPixel,
    robotLength = 18.9, -- in cm, used for rotation

    -- Normally the scale should equal (robotWidthPx/robotPicWidth), since
    -- the robot- and m32 board are equally wide. However due the imperfect
    -- images a small correction factor is applied
    m32Scale = robotWidthPx / robotPicWidth * 1.00,

    m32Front = { pos = { 17, 60 }, rotation = 180 },
    m32Back = { pos = { 10, 542 }, rotation = 0 },

    LED1 = { pos = { 440, 209 }, radius = 10, color = green },
    LED2 = { pos = { 440, 184 }, radius = 10, color = red },
    LED3 = { pos = { 440, 156 }, radius = 10, color = red },
    LED4 = { pos = { 174, 209 }, radius = 10, color = green },
    LED5 = { pos = { 174, 186 }, radius = 10, color = red },
    LED6 = { pos = { 174, 160 }, radius = 10, color = red },

    powerLED = { pos = { 178, 513 }, radius = 10, color = yellow },

    ACSLeft =
    {
        pos = { 169, 62 },
        radius = 11,
        color = darkblue,
        angle = 357,
        distance = ACSDistances
    },
    ACSRight =
    {
        pos = { 454, 60 },
        radius = 11,
        color = darkblue,
        angle = 3,
        distance = ACSDistances
    },

    bumperLeft =
    {
        points = { { 115, -1 }, { 189, -1 }, { 298, 19 }, { 120, 29 }, { 112, 13 } },
        color = transblue
    },
    bumperRight =
    {
        points = { { 327, 15 }, { 442, -4 }, { 508, 4 }, { 514, 16 }, { 507, 35 } },
        color = transblue
    },

    leftLDR = { pos = { 276, 72 } },
    rightLDR = { pos = { 340, 71 } },
}
