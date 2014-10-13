-- check for GPIO available on the remote system

function value_to_string( value )
    if value then
        return 'available'
    else
        return 'not available'
    end
end

function main( argv )
    println( 'GPIO is '..value_to_string( fr.client.gpio.available ) )
end
