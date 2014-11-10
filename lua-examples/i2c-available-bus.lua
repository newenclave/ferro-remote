--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua
--]]

-- test for the bus availability

open( "base" )

i2c = fr.client.i2c

function print_functions( f )
    local no_yes = { [false]="no", [true]="yes" }
    for k, v in pairs(f) do
        println( "\t", k, " = ", no_yes[v] )
    end
end

function main( argv )

    local i = 0

    while i < 10 do
        local a = i2c.bus_available(i)
        if a then
            println( "Bus ", i, " is available" )
            local i, err = i2c.open( i )
            f = i2c.functions( i )
            print_functions( f )
            i2c.close( i )
        else
            println( "Bus ", i, " is not available" )
        end
        i = i + 1
    end

end
