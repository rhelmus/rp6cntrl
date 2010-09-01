extmodule(...)


-- Cell class

local cellMT = { }
cellMT.__index = cellMT

function cellMT:init(x, y)
    self.x = x or 0
    self.y = y or 0
end

function cellMT:move(dx, dy)
    self.x = self.x + dx
    self.y = self.y + dy
end

function cellMT:set(x, y)
    self.x = x
    self.y = y
end

function cellMT:setx(x)
    self.x = x
end

function cellMT:sety(y)
    self.y = y
end

function cellMT.__add(c1, c2)
    return newcell(c1.x + c2.x, c1.y + c2.y)
end

function cellMT.__sub(c1, c2)
    return newcell(c1.x - c2.x, c1.y - c2.y)
end

function cellMT:__tostring()
    return string.format("cell(%d, %d)", self.x, self.y)
end


-- Grid class

local gridMT = { }

function gridMT:__index(key)
    if type(key) == "number" then
        return self.grid[key] -- Grid array access
    else
        -- Regular access from class table or it's MT
        return rawget(self, key) or gridMT[key]
    end
end

function gridMT:init(cellsize)
    self.cellsize = cellsize or 30
    self.pathengine = newpathengine()
    self:setsize(1, 1)
    self.pathstart, self.pathgoal, self.robot = self.grid[0][0], self.grid[0][0],
                                                self.grid[0][0]
    self.robotangle = 0
end

function gridMT:initclient()
    sendmsg("enablepathclient", true)
    sendmsg("grid", self.size.w, self.size.h)
    sendmsg("start", self.pathstart.x, self.pathstart.y)
    sendmsg("goal", self.pathgoal.x, self.pathgoal.y)
    sendmsg("robot", self.robot.x, self.robot.y)
    sendmsg("cellsize", self.cellsize)
end

function gridMT:handlecmd(cmd, ...)  
    if cmd == "setstart" then
        self.pathstart = self.grid[tonumber(selectone(1, ...))][tonumber(selectone(2, ...))]
    elseif cmd == "setgoal" then
        self.pathgoal = self.grid[tonumber(selectone(1, ...))][tonumber(selectone(2, ...))]
    elseif cmd == "setgrid" then
        self:setsize(tonumber(selectone(1, ...)), tonumber(selectone(2, ...)))
    else
        return false
    end
    
    return true
end

function gridMT:setsize(w, h)
    self.grid = { } -- Columns
    for x=0, w-1 do
        self.grid[x] = { } -- Rows
        for y=0, h-1 do
            self.grid[x][y] = newcell(x, y)
        end
    end
    
    self.size = self.size or { }
    self.size.w = w
    self.size.h = h
    
    self.pathengine:setgrid(w, h)
    sendmsg("grid", w, h)
end

function gridMT:getsize()
    return self.size
end

function gridMT:getcellsize()
    return self.cellsize
end

function gridMT:expand(left, up, right, down)
    if left > 0 then
        for n=1, left do
            table.insert(self.grid, 0, { }) -- Insert new col
        end
        
        self.size.w = self.size.w + left
        
        for x=0, self.size.w-1 do
            self.grid[x] = self.grid[x] or { } -- Add row
            for y=0, self.size.h-1 do
                if x < left then -- New cell?
                    self.grid[x][y] = newcell(x, y)
                else
                    self.grid[x][y]:move(left, 0)
                end
            end
        end
    end
    
    if up > 0 then
        self.size.h = self.size.h + up
        
        for x=0, self.size.w-1 do
            for y=0, self.size.h-1 do
                if y < up then -- New row?
                    table.insert(self.grid[x], 0, nav.newcell(x, y)) -- Insert row
                else
                    print(self.grid[x][y])
                    self.grid[x][y]:move(0, up)
                end
            end
        end
    end
    
    if right > 0 then
        for n=1, right do
            local col = { }
            table.insert(self.grid, col)
            self.size.w = self.size.w + 1
            for y=0, self.size.h-1 do
                col[y] = nav.newcell(self.size.w-1, y)
            end
        end
    end
    
    if down > 0 then
        self.size.h = self.size.h + down
        local y = self.size.h - 1
        for x=0, self.size.w-1 do
            self.grid[x][y] = nav.newcell(x, y)
        end
    end

-- UNDONE: Need this?
--[[    if left > 0 or up > 0 then -- Move cell
        self.pathstart.move(left, up)
        self.pathgoal.move(left, up)
        self.robot.move(left, up)
    end]]
    
    self.pathengine:expandgrid(left, up, right, down)
    sendmsg("gridexpanded", left, up, right, down)
end

function gridMT:setpathstart(s)
    self.pathstart = s
    sendmsg("start", s.x, s.y)
end

function gridMT:getpathstart()
    return self.pathstart
end

function gridMT:setpathgoal(g)
    self.pathgoal = g
    sendmsg("goal", g.x, g.y)
end

function gridMT:getpathgoal()
    return self.pathgoal
end

function gridMT:setrobot(r)
    self.robot = r
    sendmsg("robot", r.x, r.y)
end

function gridMT:getrobot()
    return self.robot
end

function gridMT:setrobotangle(a)
    self.robotangle = a
    sendmsg("rotation", a)
end

function gridMT:getrobotangle()
    return self.robotangle
end

function gridMT:calcpath()
    self.pathengine:init(self.pathstart.x, self.pathstart.y,
                         self.pathgoal.x, self.pathgoal.y)

    local stat, path = self.pathengine:calc()
    if stat then
        print("Calculated path!")
        sendmsg("path", path)
        
        -- Convert to cells
        local cellpath = { }
        for _, v in ipairs(path) do
            table.insert(cellpath, self.grid[v.x][v.y])
        end
        
        return true, cellpath
    else
        print("WARNING: Failed to calculate path!")
        return false
    end
end

function gridMT:addobstacle(cell)
    self.pathengine:setobstacle(cell.x, cell.y)
    sendmsg("obstacle", cell.x, cell.y)
end

function gridMT:getvec(cell)
    -- Returns coords of cell center
    local x, y = cell.x * self.cellsize, cell.y * self.cellsize
    x = x + ((self.cellsize-1) / 2)
    y = y + ((self.cellsize-1) / 2)
    return newvector(x, y)
end

function gridMT:getcell(vec)
    local x = math.floor(vec:x() / self.cellsize)
    local y = math.floor(vec:y() / self.cellsize)
    return self.grid[x][y]
end

-- Expands grid if necessary
function gridMT:safegetcell(vec)
    local x = math.floor(vec:x() / self.cellsize)
    local y = math.floor(vec:y() / self.cellsize)
    
    local exl, exu, exr, exd = 0, 0, 0, 0
    if x < 0 then
        exl = math.abs(x)
    elseif x > self.size.w then
        exr = x - self.size.w
    end
    
    if y < 0 then
        exu = math.abs(y)
    elseif y > self.size.h then
        exd = y - self.size.h
    end

    local expand = (exl > 0 or exu > 0 or exr > 0 or exd > 0)
    if expand then
        print("expand:", exl, exu, exr, exd)
        
        self:expand(exl, exu, exr, exd)
       
        if x < 0 then
            x = 0
        end
        if y < 0 then
            y = 0
        end
        
    end

    return self.grid[x][y], expand
end

function gridMT:__tostring()
    return string.format("grid(%d, %d)", self.size.w, self.size.h)
end


-- Misc functions

function newcell(x, y)
    local ret = { }
    setmetatable(ret, cellMT)
    ret:init(x, y)
    return ret
end

function newgrid(cellsize)
    local ret = {  }
    setmetatable(ret, gridMT)
    ret:init(cellsize)
    return ret
end
