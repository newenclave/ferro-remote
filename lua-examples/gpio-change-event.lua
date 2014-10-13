--[[
%ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua -p"gpio=id"
--]]

gpio = fr.client.gpio

old_print = print -- base module imports own 'print' we don't want

open( "base" ) -- for tonamber

print = old_print -- restore our print

if not gpio.available then
    die "GPIO is not available on the target system :("
end

function change_handler( new_value, device_id ) -- other lua thread
    println( "Value for gpio "..device_id.." changed to "..new_value )
end

function main( argv ) --- main lua thread

    dev = gpio.export( tonumber( argv.gpio ) )
    gpio.set_edge( dev, gpio.EDGE_BOTH )

    gpio.register_for_change( dev, "change_handler", argv.gpio )

    while true do  --- do work
        print('.')
        sleep( 1 )
    end

end
