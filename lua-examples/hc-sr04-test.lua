--[[
%ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua
--]]

TRIG = 24
ECHO = 3

gpio = fr.client.gpio

last_event = 0

function change_handler( new_value, l_event )
    local now_event = hight_clock( )
    println( "Events duration: ", (now_event - last_event) / 1000 )
    printiln( l_event )
end

function main( argv )

    devin = gpio.export( ECHO, gpio.DIRECT_IN )
    gpio.set_edge( devin, gpio.EDGE_BOTH )

    devout = gpio.export( TRIG, gpio.DIRECT_OUT )

    gpio.set_value( devout, 1 )
    sleep( 2 )
    gpio.set_value( devout, 0 )

    gpio.register_for_change_int( devin, "change_handler" )

    last_event = hight_clock( )
    gpio.make_pulse( devout, 10 )

    sleep( 1 )

end

