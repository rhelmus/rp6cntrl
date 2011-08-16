--[[
This table holds the positions (in px) and various
other properties of some of the robot pheripicals.
The scale field is used for centimeter to pixel conversion, for example
to measure distances.

It is important to update this file whenever rp6-top.png is changed!

Data used for scale parameter:
RP6 (board) width: 11.3 cm
Using GIMP's ruler tool this should equal 565 px from the rp6-top.png image.
--]]

local green = { 0, 255, 0 }
local red = { 255, 0, 0 }
local darkblue = { 0, 0, 192 }
local blue = { 0, 0, 255 }

robotProperties =
{
    scale = { cmPerPixel = 11.3 / 565 }, -- See top comments

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
        points = { { 121, 4 }, { 128, 1 }, { 201, 2 }, { 318, 26 }, { 128, 25 } },
        color = blue
    },
    bumperRight =
    {
        shape = "polygon",
        points = { { 322, 26 }, { 446, 8 }, { 516, 18 }, { 518, 28 }, { 510, 40 } },
        color = blue
    },
}
