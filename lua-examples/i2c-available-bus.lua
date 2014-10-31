--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua
--]]

-- test for the bus availability


i2c = fr.client.i2c

function main( argv )

    local i = 0
    local avail_table = { [false]="not available", [true]="available" }

    while i < 10 do
        local a = i2c.bus_available(i)
        if a then
            print( "Bus ", i, " is ", avail_table[a] )
            local err, i = i2c.open( i )
            f = i2c.functions( i )
            println( ": ", f )
            i2c.close( i )
        else
            println( "Bus ", i, " is ", avail_table[a] )
        end
        i = i + 1
    end

end
