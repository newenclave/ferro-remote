--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua
--]]

i2c  = fr.client.i2c

open( "string" )

mma7660fc = {
    mode      = 0x7,
    active    = 1,
    stand_by  = 0,

    X         = 0,
    Y         = 1,
    Z         = 2,
    TILT      = 3,
}

function mma7660fc_activate( dev )
    i2c.write_byte( dev, mma7660fc.mode, mma7660fc.active )
end

function main ( argv )

    local err, i = i2c.open( 1 )

    if err then
        println( "Bus error: ", err )
        return 1
    end

    i2c.set_address( i, 0x4c )
    mma7660fc_activate( i )

    while true do

        local d = i2c.read( i, 7 )

        printi( "X=",     d:byte( mma7660fc.X    + 1 ), "\t",
                "Y=",     d:byte( mma7660fc.Y    + 1 ), "\t",
                "Z=",     d:byte( mma7660fc.Z    + 1 ), "\t",
                "TILT=",  d:byte( mma7660fc.TILT + 1 ), "\t",
                --"SRST=",  d:byte( 5 ), "\t",
                --"SPCNT=", d:byte( 6 ), "\t",
                --"INTSU=", d:byte( 7 ), "\t",
                "                       \n" )
        --sleep( 1 )

    end

end
