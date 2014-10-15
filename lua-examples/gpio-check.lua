-- check for GPIO available on the remote system

function main( argv )
    println( 'GPIO is '..({ [false] = "not ",
                            [true]  = ""
                         }) [fr.client.gpio.available].."available" )
end
