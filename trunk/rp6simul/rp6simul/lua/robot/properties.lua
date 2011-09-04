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
local robotWidthPx = 565
local cmPerPixel = robotWidthCm / robotWidthPx

local robotPicWidth = 637 -- Full width of rp6-top.png image

local green = { 0, 255, 0 }
local red = { 255, 0, 0 }
local darkblue = { 0, 0, 192 }
local transblue = { 0, 0, 255, 80 }

-- In px. Real life measurements :)
local ACSDistances =
{
    low = 11 / cmPerPixel,
    medium = 23 / cmPerPixel,
    high = 32 / cmPerPixel
}

properties =
{
    clockSpeed = 8000000,
    cmPerPixel = cmPerPixel,
    robotLength = 18.9, -- in cm, used for rotation

    -- Normally the scale should equal (robotWidthPx/robotPicWidth), since
    -- the robot- and m32 board are equally wide. However due the imperfect
    -- images a small correctection factor is applied
    m32Scale = robotWidthPx / robotPicWidth * 1.08,

    m32Front = { pos = { 22, 60 }, rotation = 180 },
    m32Back = { pos = { 12, 540 }, rotation = 0 },

    led1 = { pos = { 458, 209 }, radius = 10, color = green },
    led2 = { pos = { 458, 184 }, radius = 10, color = red },
    led3 = { pos = { 458, 158 }, radius = 10, color = red },
    led4 = { pos = { 191, 209 }, radius = 10, color = green },
    led5 = { pos = { 191, 181 }, radius = 10, color = red },
    led6 = { pos = { 191, 156 }, radius = 10, color = red },

    ACSLeft =
    {
        pos = { 181, 62 },
        radius = 11,
        color = darkblue,
        angle = 357, -- UNDONE
        distance = ACSDistances
    },
    ACSRight =
    {
        pos = { 465, 64 },
        radius = 11,
        color = darkblue,
        angle = 3, -- UNDONE
        distance = ACSDistances
    },

    bumperLeft =
    {
        points = { { 119, -1 }, { 210, -1 }, { 319, 23 }, { 125, 28 } },
        color = transblue
    },
    bumperRight =
    {
        points = { { 322, 23 }, { 439, 2 }, { 523, 12 }, { 525, 28 }, { 515, 44 } },
        color = transblue
    },

    leftLDR = { pos = { 290, 70 } },
    rightLDR = { pos = { 353, 70 } },
}
