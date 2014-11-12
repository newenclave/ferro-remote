--[[
%ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua -p"gpio=id"
--]]

gpio = fr.client.gpio

old_print = print
open( "base" )
print = old_print

if not gpio.available then
    die( "GPIO is not available on the "..fr.client.server.." machine :(" )
end

states      = { [true] = 0, [false] =  0 }
directions  = { [true] = 1, [false] = -1 }
current     = 0

function change_handler( new_value, direct )

    states[direct] = new_value

    if new_value == 1 and states[not direct] == 1 then
        current = current + directions[direct]
        println( current )
    end

end

function main( argv ) --- main lua thread

    dev1 = gpio.export( tonumber( argv.gpio1 ) )
    dev2 = gpio.export( tonumber( argv.gpio2 ) )

    if not gpio.edge_supported( dev1 ) or not gpio.edge_supported( dev2 ) then
        die("GPIO doesn't support edge. No edge -> no events")
    end

    gpio.set_edge( dev1, gpio.EDGE_BOTH )
    gpio.set_edge( dev2, gpio.EDGE_BOTH )

    gpio.register_for_change( dev1, change_handler, false )
    gpio.register_for_change( dev2, change_handler, true )

    while true do  --- do work
        --print('.')
        sleep( 1 )
    end

end



--current_state = 0
--current_count = 0
--last_direct = nil
--last_start = 0
--function change_handler( new_value, time_diff, direct )
----printiln( "Value for gpio ", direct, ' ', time_diff )
---- stop = hight_clock( )
---- if stop - last_start > 10000000 then
---- last_direct = nil
---- current_state = 0
---- last_start = hight_clock( )
---- end
---- if time_diff > 10000 then -- microseconds
---- last_direct = nil
---- current_state = 0
---- end
--if last_direct == direct then
----println( direct, ' ', current_state )
--return
--end
--current_state = current_state + 1
--last_direct = direct
----println( current_state, " ", direct, " " )
----print( current_state, " " )
--if current_state == 1 then
--if direct then
--current_count = current_count + 1
--else
--current_count = current_count - 1
--end
--println( current_count )
--else
--current_state = 0
--end
--end
