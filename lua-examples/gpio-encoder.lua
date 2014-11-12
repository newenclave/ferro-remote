--[[
%ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua -p"gpio=id"
--]]

gpio = fr.client.gpio

old_print = print
open( "base" )
print = old_print

if not gpio.available then
    die( "GPIO is not available on the"..fr.client.server.."machine :(" )
end

current_state = 0
current_count = 0

function change_handler( new_value, device_id, direct ) -- other thread

    println( "Value for gpio ", device_id, " changed to ", direct )

    current_state = current_state + 1

    --print( current_state, " " )

    if current_state == 1 then
        if direct then
            current_count = current_count + 1
        else
            current_count = current_count - 1
        end
        --println( current_count )
    elseif current_state == 2 then
        current_state = 0
    end

end

function main( argv ) --- main lua thread

    dev1 = gpio.export( tonumber( argv.gpio1 ) )
    dev2 = gpio.export( tonumber( argv.gpio2 ) )

    if not gpio.edge_supported( dev1 ) then
        die("GPIO "..dev.." doesn't support edge. No edge -> no events")
    end

    if not gpio.edge_supported( dev2 ) then
        die("GPIO "..dev.." doesn't support edge. No edge -> no events")
    end

    gpio.set_edge( dev1, gpio.EDGE_FALLING )
    gpio.set_edge( dev2, gpio.EDGE_FALLING )

    gpio.register_for_change( dev1, "change_handler", argv.gpio1, false )
    gpio.register_for_change( dev2, "change_handler", argv.gpio2, true )

    while true do  --- do work
        --print('.')
        sleep( 1 )
    end

end
