con = fr.client.console
cf = con.flush

window = { 
    x = 0, y = 0, 
    children = { },    
    h = con.size( ).height, w = con.size( ).width
}

window.size = function ( self )
    return self.w, self.h
end

window.add = function ( self, new_wind )
    self.children[#self.children + 1] = new_wind
end

window.pos = function( self )
    if self.parent then 
	return self.parent.x + self.x, self.parent.y + self.y
    else 
	return self.x, self.y
    end
end

window.new = function( )
    nw = { }
    for k, v in pairs(window) do nw[k] = v end
    nw.parent = window
    nw.children = { }
    window:add(nw)
    setmetatable(nw, window)
    return nw
end

window.clone = function( self )
    nw = { }
    for k, v in pairs(self) do nw[k] = v end
    setmetatable(nw, self)
    if self.parent then
	self.parent:add( nw )
    end
    return nw
end

window.draw = function( self )
    local W = io.stdout
    local P = con.set_pos
    
    local x, y = self:pos( )
    local w, h = self:size( )

    local function mkstring( c )    
	local s = '' 
	for i = 0, w - 1 do 
	    s = s..c
	end
	return s
    end

    local function draw_border( )
	local s = mkstring( '-' )
	con.set_pos( x, y )
	W:write( s )
	con.set_pos( x, y + h )
	W:write( s )
	for i = 0, h do 
	    con.set_pos( x, y + i )
	    W:write( '|' )
	    con.set_pos( x + w, y + i )
	    W:write( '|' )
	end
    end

    local function fill( )
	local s = mkstring( ' ' )
	for i = 0, h  do 
	    con.set_pos( x, y + i )
	    W:write( s )
	end
    end

    fill( )
    draw_border( )
    P( x,     y     ) W:write( "+" )
    P( x + w, y     ) W:write( "+" )
    P( x,     y + h ) W:write( "+" )
    P( x + w, y + h ) W:write( "+" )
    for k, v in pairs( self.children ) do 
	v:draw( )
    end
    io.stdout:flush( )
end

function draw_all(  )
    con.clear( )
    window:draw( )
    fr.client.event_queue.timer.post(draw_all, {0, 100} )
end

function main( )
    con.clear( )
    
    local h = window:new( )

    h.x, h.y = 5,  5
    h.w, h.h = 10, 5

    local h2 = h:clone( )	
    h2.x, h2.y = h2.x + 2, h2.y + 2 

    local h3 = h2:clone( )
    h3.x, h3.y = 25, 8 

    draw_all( window ) 

    fr.run( )
end
