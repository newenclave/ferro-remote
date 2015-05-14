con = fr.client.console
cf = con.flush

window = { 
    x = 0, y = 0, 
    children = { },    
    h = con.size( ).height, w = con.size( ).width
}

window.size = function ( self )
    if self.parent then
        return self.w, self.h
    else 
	return con.size( ).width, con.size( ).height 
    end
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
    
    w, h = w - 1, h -1

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
    
    if self.cdraw then 
	self:cdraw( )
    end	
    
    for k, v in pairs( self.children ) do 
	v:draw( )
    end
    io.stdout:flush( )
end

function draw_all( err, win )
    --con.clear( )
    win:draw( )
    fr.client.event_queue.timer.post(draw_all, {0, 100}, win )
end

function timer( err, win, milli, set_text )
    win.value = win.value + 1
    if set_text then
        status.text =  "progress: "..tostring(win.value)..":"..tostring(win.w - 1)
    end
    if win.value > (win.w - 3) then
	win.value = 0
    end
    win:draw( )
    fr.client.event_queue.timer.post( timer, {0, milli}, win, milli, set_text )
end

function main( )
    con.clear( )
    
    local h = window:new( )

    h.x, h.y = 5,  5
    h.w, h.h = 10, 5

    local h2 = h:clone( )	
    h2.x, h2.y = h2.x + 2, h2.y + 2 

    local h4 = h:clone( )
    h4.x, h4.y = 10, 1
    h4.w, h4.h = 35, 3 
    h4.value = 0    

    h4.cdraw = function( self )
        con.set_pos( self.x + 1 + self.parent.x, self.y + 1 + self.parent.y )
        
        if self.value < ( self.w // 3 ) then
            con.set_color( "green" )
        elseif self.value < ( self.w - self.w // 3 )  then 
            con.set_color( "yellow" )
        else 
            con.set_color( "red" )
        end
        
        for i = 0, self.value do 
            io.stdout:write( "|" )
        end
        con.set_color( )
    end

    local h5 = h4:clone( )
    h5.x, h5.y = h5.x, h5.y + h4.h
 
    status = window:new( )
    status.text = ""
    status.size = function( )
	return con.size( ).width, 3
    end
    status.pos = function( )
	return 0, con.size( ).height - 3 
    end
    status.cdraw = function( self )
	con.set_pos( 2, con.size( ).height - 2 )
	io.stdout:write( self.text )
    end   

 
    draw_all( nil, window ) 
    timer( nil, h4, 200, true )
    timer( nil, h5, 500 )    

    fr.run( )
end
