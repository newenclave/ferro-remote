gpio = fr.client.gpio 

function main( )
				g, e = gpio.export( 3, "out" )
				if e then 
								fr.print( "Error: ", e, "\n" )
								return 
				end 
				r, e = gpio.set( g, "edge", "both" )
				if e then 
								fr.print( "Error: ", e, "\n" )
								return 
				end 
				r, e = gpio.subscribe( g, "on_changed", function( data ) fr.print( data, "\n" ) end )
				if e then 
								fr.print( "Error: ", e, "\n" )
								return 
				end 
				fr.run( )
end
