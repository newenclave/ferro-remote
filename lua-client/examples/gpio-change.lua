gpio = fr.client.gpio 

function main( )
				g, e = assert(gpio.export( 3, "out" ))
				if e then 
								fr.print( "Export error: ", e, "\n" )
								return 
				end 
				r, e = gpio.set( g, "edge", "both1" )
				if e then 
								fr.print( "Set edge error: ", e, "\n" )
								return 
				end 
				r, e = gpio.subscribe( g, "on_changed", function( data ) fr.print( data, "\n" ) end )
				if e then 
								fr.print( "Subscribe error: ", e, "\n" )
								return 
				end 
				fr.run( )
end
