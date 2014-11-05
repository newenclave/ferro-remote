--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
    -p"bus=XX" -p"address=XX"
--]]

-- shows i2c dump

old_print = print

open( "base" )
open( "string" )
open( "table" )

print = old_print

i2c = fr.client.i2c

function main( argv )

    bus = tonumber(argv.bus)
    address = tonumber(argv.address)

    println( "bus: ", bus, " address: ", address )

    local ic, err = i2c.open( bus, address )
    println("     ",
            "0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f",
            "    0123456789abcdef")

    --- just values 1-255
    request = {}

    for i=0, 255 do
        table.insert( request, i )
    end


    local test = i2c.read_byte( ic, request )

    local i = 0
    local r = 0
    local str = ""

    print( string.format('%02X: ', r ) )

    while i < 256 do

        local b = test[i]
        print( string.format('%02X ', b ) )

        str = str..string.char(b)
        i = i + 1
        if i % 16 == 0 then
            r = r + 16
            print( '   ', str:gsub('%c','.'), '\n' )
            str = ""
            if i < 256 then
                print( string.format('%02X: ', r ) )
            end
        end
    end

end
