i2c = fr.client.smbus
eq  = fr.client.event_queue
eqt = eq.timer 

device_address	= 0x69
ctrl_regs	= { 0x20, 0x21, 0x22, 0x23, 0x24 }
scale_values	= { [250]=0, [500]=16, [2000]=48 }

R_xLSB = 0x28
R_xMSB = 0x29

R_yLSB = 0x2A
R_yMSB = 0x2B

R_zLSB = 0x2C
R_zMSB = 0x2D

read_xyz_data = { R_xLSB, R_xMSB, R_yLSB, R_yMSB, R_zLSB, R_zMSB }

L3G4200D = { }

L3G4200D.setup = function( self, scale ) 
    -- setup table for write
    local data = { 
	[ctrl_regs[1]] = 15,
	[ctrl_regs[2]] =  0,
	[ctrl_regs[3]] =  8,
	[ctrl_regs[4]] = scale_values[scale],
	[ctrl_regs[5]] =  0 
    }
    -- write registry
    assert(self.i:write_bytes(data))
end

L3G4200D.read_xyz = function( self )
    local res = assert(self.i:read_bytes( read_xyz_data ))
    return { x = res[R_xMSB] << 8 | res[R_xLSB],
             y = res[R_yMSB] << 8 | res[R_yLSB],
             z = res[R_zMSB] << 8 | res[R_zLSB] }
end

L3G4200D.new = function( ) 
    inst = { }
    for k, v in pairs(L3G4200D) do 
        inst[k] = v 
    end
    inst.i = assert(i2c.open( 1, device_address ))
    setmetatable( inst, L3G4200D )
    return inst
end

function show_values( err, dev )
    if nil == err then 
        local values = dev:read_xyz( )
	print( "X:", values.x, 
               "Y:", values.y, 
               "Z:", values.z )
	eqt.post( show_values, {milli=100}, dev )
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

