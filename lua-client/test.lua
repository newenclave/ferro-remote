con = fr.client.console
eqt = fr.client.event_queue.timer

function draw( )
	local pos = con.size( )
	con.clear( )
	con.set_pos( pos.width // 2, pos.height // 2 )
	fr.print( "¡Hola! ", pos )
	eqt.post( draw, 1 )
end

function main( )
	fr.run( )
	draw( )	
end
