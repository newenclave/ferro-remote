gpio = fr.client.gpio 

function main( argv )
    g = gpio.export( 22, "out" )
    res, err = gpio.set( g, 1 )
    res, err = gpio.get( g )
    fr.print( gpio.info( g ) )
    fr.print( res, err, "\n" )
end
