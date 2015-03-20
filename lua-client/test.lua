---

function main( argv )
	fr.client.subscribe( "on_disconnect", 
												function( data ) 
														fr.print ( "disconnected\n" ) 
														fr.exit( )
												end )	
	fr.print( fr, "\n" )	
	fr.print( fr.client.events( ), "\n" )	
end
