--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua
--]]

i2c  = fr.client.i2c

open( "string" )

mma7660fc = {

    default_address = 0x4c,

    mode            = 0x7,
    active          = 1,
    stand_by        = 0,

    X               = 0,
    Y               = 1,
    Z               = 2,
    TILT            = 3,
    SRST            = 4,
    SPCNT           = 5,
    INTSU           = 6
}

function mma7660fc_activate( dev )
    i2c.set_address( dev, mma7660fc.default_address )
    i2c.write_byte( dev, { [mma7660fc.mode] = mma7660fc.active } )
end

function main ( argv )

    local i, err = i2c.open( 1 )

    if err then
        println( "Bus error: ", err )
        return 1
    end

    mma7660fc_activate( i )

    while true do

        local d = i2c.read_byte( i, { mma7660fc.X,
                                      mma7660fc.Y,
                                      mma7660fc.Z,
                                      mma7660fc.TILT } )

        printi( "X=",     d[mma7660fc.X],       "\t",
                "Y=",     d[mma7660fc.Y],       "\t",
                "Z=",     d[mma7660fc.Z],       "\t",
                "TILT=",  d[mma7660fc.TILT],    "\t",
                "\n" )
        --sleep( 1 )

    end

end
