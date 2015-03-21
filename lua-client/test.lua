---

events = fr.client.event_queue

function test( param )
	fr.print( param )
	events.timer.post( fr.client.disconnect, {[2] = 1000} )
end
		
function main( argv )
	fr.client.subscribe( "on_disconnect", 
												function( data ) 
														fr.print ( "disconnected\n" ) 
														fr.print( "\n=============\n", fr, "\n===========\n" )
														fr.exit( )
												end )	
  fr.client.subscribe( "on_ready", 
												function( data ) 
														fr.print ( "ready!\n" ) 
														events.post( test, "Hello, world!\n" )
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
