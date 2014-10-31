--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua
--]]

-- test mma7660fc device as file


i2c = fr.client.i2c

function main( argv )

    local i = 0
    local avail_table = { [false]="not available", [true]="available" }

    while i < 10 do
        println( "Bus ", i, " is ", avail_table[i2c.bus_available(i)] )
        i = i + 1
    end

end
