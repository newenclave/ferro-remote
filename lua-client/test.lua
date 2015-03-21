---

function main( argv )
	fr.client.subscribe( "on_disconnect", 
												function( data ) 
														fr.print ( "disconnected\n" ) 
														fr.exit( )
												end )	
  fr.client.subscribe( "on_ready", 
												function( data ) 
														fr.print ( "ready!\n" ) 
												end )	
  fr.client.subscribe( "on_init_error", 
												function( data ) 
														fr.print ( "init error: ", data.message, "\n" )
														fr.exit( )
												end )	

	fr.print( fr, "\n" )	
	fr.print( fr.client.events( ), "\n" )	
	fr.client.connect( "127.0.0.1:12345")	
end
