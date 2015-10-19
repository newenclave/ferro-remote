spidev = { 
    bus	    = 0,
    channel = 0
}

spidev.new = function( bus, channel )
    local spi = fr.client.spi
    local dev, err = spi.open( bus, channel )
    if not dev then
       return dev, err
    end
 
    inst = { }
    inst.bus     = bus
    inst.channel = channel
    inst.dev     = dev
    for k, v in pairs(spidev) do inst[k] = v end
    
    return inst
end

spidev.transfer = function( self, data )
    return self.dev:transfer( data )
end


spidev.read = function( self, length )
    return self.dev:read( length )
end

spidev.write = function( self, data )
    return self.dev:write( data )
end

spidev.write_read = function( self, data )
    return self.dev:wr( data )
end

spidev.setup = function( self, speed, mode )
    return self.dev:setup( speed, mode )
end

function main( )
    local s = assert(spidev.new(0, 1)) 
    fr.print( "transfer res: ", assert(s:transfer( {0xFF, 0xFF} )), "\n" )
    
end