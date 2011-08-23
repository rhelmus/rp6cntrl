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

local green = { 0, 255, 0 }
local red = { 255, 0, 0 }
local darkblue = { 0, 0, 192 }
local transblue = { 0, 0, 255, 80 }

robotProperties =
{
    scale = { cmPerPixel = 11.3 / 565 }, -- See top comments
    robotLength = { length = 18.9 }, -- in cm, used for rotation

    led1 = { pos = { 458, 209 }, shape = "ellips", radius = 10, color = green },
    led2 = { pos = { 458, 184 }, shape = "ellips", radius = 10, color = red },
    led3 = { pos = { 458, 158 }, shape = "ellips", radius = 10, color = red },
    led4 = { pos = { 191, 209 }, shape = "ellips", radius = 10, color = green },
    led5 = { pos = { 191, 181 }, shape = "ellips", radius = 10, color = red },
    led6 = { pos = { 191, 156 }, shape = "ellips", radius = 10, color = red },

    ACSLeft = { pos = { 181, 62 }, shape = "ellips", radius = 11, color = darkblue },
    ACSRight = { pos = { 465, 64 }, shape = "ellips", radius = 11, color = darkblue },

    bumperLeft =
    {
        shape = "polygon",
        points = { { 119, -1 }, { 210, -1 }, { 319, 23 }, { 125, 28 } },
        color = transblue
    },
    bumperRight =
    {
        shape = "polygon",
        points = { { 322, 23 }, { 439, 2 }, { 523, 12 }, { 525, 28 }, { 515, 44 } },
        color = transblue
    },
}
