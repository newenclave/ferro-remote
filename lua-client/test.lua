---

events = fr.client.event_queue

function test( param )
	fr.print( param )
	--events.timer.post( fr.client.disconnect, {[2] = 1000} )
  call_os( )
end

function call_os(  )
		fr.print("\n\nfile: ", fr.client.fs.file, "\n\n==========\n\n")
end

function con_handler( data )
				if data.error then 
								fr.print( "Error ", data.error )
				else 
								fr.print( string.len(data.data), "\n" )
								if data.data == "edit\n" then
												fr.client.os.system( "gedit" )
								end
				end
end
		
function main( argv )

		fr.print( fr.client.gpio, "\n\n" )
	fr.print( "Main function; argv = ", argv, "\n" )	
	fr.client.subscribe( "on_disconnect", 
	                     function( data ) 
	                     		fr.print ( "disconnected\n" ) 
	                     		--fr.print( "\n=============\n", fr, "\n===========\n" )
	                     		--fr.exit( )
	                     end )	
	fr.client.subscribe( "on_ready", 
	                     function( data ) 
	                     	  fr.print ( "ready!\n" ) 
	                        --fr.print( fr, "\n" )	
	                        --fr.client.fs.cd( "/home/data" )		
	                        --fr.print(fr.client.fs.info( ), "\n")	
	                     	  events.post( test, "Hello, world!\n" )
	                     end )	
	fr.client.subscribe( "on_init_error", 
	                     function( data ) 
	                     		fr.print ( "init error: ", data.message, "\n" )
	                     		fr.exit( )
	                     end )	

	fr.print( fr.client.events( ), "\n" )	
	fr.client.connect( "127.0.0.1:12345")	
 fr.client.console.subscribe( "on_read", con_handler )
 fr.run( )
end

