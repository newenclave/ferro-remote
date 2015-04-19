gpio = fr.client.gpio 

function main( argv )
    g = gpio.export( 22, "out" )
    res, err = g:set( 1 )
    res, err = g:get( )
    fr.print( g:info( ) )
    fr.print( res, err, "\n" )
end
