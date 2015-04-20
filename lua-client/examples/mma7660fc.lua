i2c = fr.client.smbus
eq  = fr.client.event_queue
eqt = eq.timer 
con = fr.client.console

RANGE	        = 1500000
PRECISION       = 6
BOUNDARY        = 1 << (PRECISION - 1)
GRAVITY_STEP    = RANGE // BOUNDARY

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
    INTSU           = 6,
}

function convert_value( val )
    if val < BOUNDARY then 
        return val * GRAVITY_STEP
    else 
        return ~(((~val & 0x3f) + 1)* GRAVITY_STEP) + 1 
    end
end

function main ( argv )

    local id = 1
    if argv[1] then 
        id = tonumber(argv[1])
    end
    local i, err = i2c.open( 1 )

    con.clear( )
    if err then
        println( "Bus error: ", err )
        return 1
    end

	--- activate device
    dev:set_address( mma7660fc.default_address )
    dev:write_bytes( { [mma7660fc.mode] = mma7660fc.active } )
    
    local function read(  )
        local d, e = i:read_bytes( { mma7660fc.X,
                                     mma7660fc.Y,
                                     mma7660fc.Z,
                                     mma7660fc.TILT } )
        con.set_pos( 0, con.size( ).height - id )
        fr.print( "X=",     convert_value(d[mma7660fc.X]), "    ",
                  "Y=",     convert_value(d[mma7660fc.Y]), "    ",
                  "Z=",     convert_value(d[mma7660fc.Z]), "    ",
                  -- "TILT=",  d[mma7660fc.TILT],		   "    ",
                "    " )
	eqt.post( read, {milli=100} )
    end
    fr.run( )
    read( )
end

