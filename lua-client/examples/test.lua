con = fr.client.console
eqt = fr.client.event_queue.timer

function draw( )
	local pos = con.size( )
	con.clear( )
	local s = "¡Hola!"
	con.set_pos( pos.width // 2 - s:len( ) // 2, 
                 pos.height // 2 )
	fr.print( s )
	eqt.post( draw, { milli = 100} )
end

function main( )
	fr.run( )
	draw( )	
end
