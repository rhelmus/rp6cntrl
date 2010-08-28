extmodule(...)


-- Cell class

local cellMT = { }
cellMT.__index = cellMT

function cellMT:init(x, y)
    self.x = x or 0
    self.y = y or 0
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
    self.grid = { }
    self.size = { w = 0, h = 0 }
    self.cellsize = cellsize or 30
    self.pathengine = newpathengine()
    self.start, self.goal, self.robot = newcell(), newcell(), newcell()
    print("New grid initialized")
end

function gridMT:initclient()
    sendmsg("enablepathclient", true)
    sendmsg("grid", size.w, size.h)
    sendmsg("start", self.pathstart.x, self.pathstart.y)
    sendmsg("goal", self.pathgoal.x, self.pathgoal.y)
    sendmsg("robot", self.robot.x, self.robot.y)
end

function gridMT:setsize(w, h)
    self.grid = { } -- Columns
    for x=0, w-1 do
        self.grid[x] = { } -- Rows
        for y=0, y-1 do
            self.grid[x][y] = newcell(x, y)
        end
    end
    
    self.size.w = w
    self.size.h = h
    
    pathengine:setgrid(w, h)
    sendmsg("grid", size.w, size.h)
end

function gridMT:getsize()
    return self.size
end

function gridMT:expand(left, up, right, down)
    self.size.w = self.size.w + left + right
    self.size.h = self.size.h + up + down

    if left > 0 or up > 0 then -- Move cell
        local dcell = newcell(left, up)
        self.pathstart = self.pathstart + dcell
        self.pathgoal = self.pathgoal + dcell
    end
    
    self.pathengine:expandgrid(left, up, right, down)
    sendmsg("gridexpanded", left, up, right, down)
end

function gridMT:calcpath()
    pathengine:init(startcell.x, startcell.y, goalcell.x, goalcell.y)
    local stat, path = pathengine:calc()
    if stat then
        print("Calculated path!")
        sendmsg("path", path)
        return true, path
    else
        print("WARNING: Failed to calculate path!")
        return true, nil
    end
end

function gridMT:addobstacle(cell)
    pathengine:setobstacle(x, y)
    sendmsg("obstacle", x, y)
end

function gridMT:getvec(cell)
    -- Returns coords of cell center
    local x, y = cell.x * self.cellsize, cell.y * self.cellsize
    x = x + (self.cellsize / 2)
    y = y + (self.cellsize / 2)
    return newvector(x, y)
end

function gridMT:getcell(vec)
    local x, fx = math.modf(vec:x() / cellsize)
    local y, fy = math.modf(vec:y() / cellsize)
    
    -- Round up when past center
    if fx > 0.5 then
        x = x + 1
    end
    if fy > 0.5 then
        y = y + 1
    end
    
    return newcell(x, y)
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
