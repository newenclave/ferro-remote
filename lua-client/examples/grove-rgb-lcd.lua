i2c = fr.client.smbus
eq  = fr.client.event_queue
eqt = eq.timer 

grove_lcd_rgb = {
    RGB_ADDR = 0x62,
    TXT_ADDR = 0x3E
}

grove_lcd_rgb.new = function( )
    inst = { }
    for k, v in pairs(grove_lcd_rgb) do 
        inst[k] = v 
    end
    inst.txt = assert(i2c.open( 1, grove_lcd_rgb.TXT_ADDR ))
    inst.rgb = assert(i2c.open( 1, grove_lcd_rgb.RGB_ADDR ))
    setmetatable( inst, grove_lcd_rgb )
    return inst
end

grove_lcd_rgb.set_color = function( self, R, G, B )
    assert(self.rgb:write_bytes( {
		[0]=0, [1]=0, 
		[0x8]=0xAA, 
		[4]=R, [3]=G, [2]=B }))
end

grove_lcd_rgb.clr = function( self )
    assert(self.txt:write_bytes({[0x80] = 0x01}))
end

grove_lcd_rgb.control = function( self, byte )
    assert(self.txt:write_bytes( {[0x80] = byte} ))
end

grove_lcd_rgb.write = function( self, txt )
    local send_txt_cmd = function( byte )
	self:control( byte )
    end
    local send_txt = function( byte )
	assert(self.txt:write_bytes( {[0x40] = byte} ))
    end
    send_txt_cmd( 0x01 )
    send_txt_cmd( 0x08 | 0x04 )
    send_txt_cmd( 0x28 )
    for i = 1, string.len(txt) do 
	local b = string.byte( txt, i )
	if b == 0x0A then
	    send_txt_cmd( 0xC0 )
	else 
	    send_txt( b )
	end
    end
end

function next_char( err, b, dev )
    dev:clr( )
    dev.txt:write_bytes( {[0x40] = b} )
    if b > 0 then 
	eqt.post( next_char, {milli=50}, b - 1, dev )
    else 
	next_char( nil, 255, dev )
    end
end

function set_color( err, R, G, B, dev )
    dev:set_color( R, G, B )
    local changed = false
    if R > 0 then 
	R = R - 1
	changed = true
    elseif B > 0 then 
	B = B - 1
	changed = true
    elseif R > 0 then
	R = R - 1
	changed = true
    end
    if changed then 
	eqt.post( set_color, {milli=10}, R, G, B, dev )
    else 
	eqt.post( set_color, {milli=10}, 255, 255, 255, dev )
    end
end

function main( )
    fr.run( )
    local i = grove_lcd_rgb.new( )
    i:set_color( 00, 255, 100 ) 
    i:write( "Hello, grove\nSecond line" )
--   eqt.post( function( err, i ) 
--         i:set_color( 0, 0, 0 ) 
--         i:clr( )
--         fr.exit( )  
--      end, 
--       {sec=5}, i )
    next_char( nil, 255, i )
    set_color( nil, 255, 255, 255, i ) 
end

