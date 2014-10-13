--[[
%ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e show-main-argv.lua -p"gpio=id"
--]]

gpio = fr.client.gpio

open( "base" ) -- for tonumber

if not gpio.available then
    die "GPIO is not available on the target system :("
end

function main( argv )
    dev = gpio.export( tonumber( argv.gpio ) )
    println( "GPIO info:  ", gpio.info( dev ) )
    gpio.unexport( dev )
end

