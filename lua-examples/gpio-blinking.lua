--[[
%ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua -p"gpio=id"
--]]

gpio = fr.client.gpio

open( "base" ) -- for tonamber

if not gpio.available then
    die "GPIO is not available on the target system :("
end

function main( argv ) --- main lua thread

    dev = gpio.export( tonumber( argv.gpio ), gpio.DIRECT_OUT )

    i = 0

    while true do  --- do work
        if 0 == i % 2 then
            println( 'OFF' )
        else
            println( 'ON' )
        end
        gpio.set_value( dev, i % 2 )
        sleep( 1 )
        i = i + 1
    end

end
