i2c = fr.client.smbus
spi = fr.client.spi
eq  = fr.client.event_queue
eqt = eq.timer 

device_address	= 0x69
ctrl_regs	= { 0x20, 0x21, 0x22, 0x23, 0x24 }
scale_values	= { [250]=0, [500]=16, [2000]=48 }

R_xL = 0x28
R_xH = 0x29

R_yL = 0x2A
R_yH = 0x2B

R_zL = 0x2C
R_zH = 0x2D

R_Temp = 0x26

read_xyz_data = { R_xL, R_xH, R_yL, R_yH, R_zL, R_zH, R_Temp }

L3G4200D = { }

L3G4200D.new = function( ) 
    inst = { }
    for k, v in pairs(L3G4200D) do 
        inst[k] = v 
    end
    inst.i = assert(i2c.open( 1, device_address ))
    setmetatable( inst, L3G4200D )
    return inst
end

L3G4200D.setup = function( self, scale ) 
    -- setup table for write
    local data = { 
	[ctrl_regs[1]] = 15, -- 00001111 PD, X, Y, Z
	[ctrl_regs[2]] =  0, -- Normal mode (default)
	[ctrl_regs[3]] =  8, -- Date Ready on DRDY/INT2.
	[ctrl_regs[4]] = scale_values[scale],
	[ctrl_regs[5]] =  0 -- default here
    }
    -- write registry
    assert(self.i:write_bytes(data))
end

L3G4200D.read_xyz = function( self )
    local res = assert(self.i:read_bytes( read_xyz_data ))
    return { x = res[R_xH] << 8 | res[R_xL],
             y = res[R_yH] << 8 | res[R_yL],
             z = res[R_zH] << 8 | res[R_zL], 
             t = res[R_Temp] }
end

function show_values( err, dev )
    if nil == err then 
        local values = dev:read_xyz( )
	print( "X:", values.x, 
               "Y:", values.y, 
               "Z:", values.z, 
               "T:", values.t )
	eqt.post( show_values, {milli=50}, dev )
    else 
	fr.exit(err)
    end
end
 
function main( )
    fr.run( )
    local r = L3G4200D.new( )
    r:setup(250)
    show_values( nil, r )
end

