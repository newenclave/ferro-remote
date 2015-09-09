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

grove_lcd_rgb.write = function( self, txt )
    local send_txt_cmd = function( byte )
	assert(self.txt:write_bytes( {[0x80] = byte} ))
    end
    local send_txt = function( byte )
	assert(self.txt:write_bytes( {[0x40] = byte} ))
    end
    send_txt_cmd( 0x01 )
    send_txt_cmd( 0x08 | 0x04 )
    send_txt_cmd( 0x28 )
    for i = 1, string.len(txt) do 
	local b = string.byte( txt, i )
	send_txt( b )
    end
end

function main( )
    fr.run( )
    local i = grove_lcd_rgb.new( )
    i:set_color( 255, 255, 255 ) 
    i:write( "1" )
    eqt.post( function( err, i ) 
		  i:set_color( 0, 0, 0 ) 
		  fr.exit( )  
	      end, 
	      {sec=5}, i )
end

