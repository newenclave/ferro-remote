gpio = fr.client.gpio 

function main( )
    g, e = assert(gpio.export( 3, "out" ))
    if e then 
       fr.print( "Export error: ", e, "\n" )
       return 
    end 
    r, e = gpio:set( "edge", "both" )
    if e then 
        fr.print( "Set edge error: ", e, "\n" )
        return 
    end 
    r, e = gpio:subscribe( "on_changed", function( data ) fr.print( data, "\n" ) end )
    if e then 
        fr.print( "Subscribe error: ", e, "\n" )
        return 
    end 
    fr.run( )
end
