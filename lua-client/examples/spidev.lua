spidev = { 
    bus	    = 0
    channel = 0
}

spidev.new = function( bus, channel )
    local spi = fr.client.spi
    local dev, err = spi.open( bus, channel )
    if not dev then
       return dev, err
    end
 
    intst = { }
    inst.bus     = bus
    inst.channel = channel
    inst.dev     = dev
    for k, v in pairs(window) do inst[k] = v end
    
    return inst
end

spidev.transfer = function( self, data )
    return self.dev:transfer( data )
end

spidev.read = function( self, length )
    retrn self.dev:read( length )
end

spidev.write = function( self, data )
    retrn self.dev:write( data )
end

spidev.write_read = function( self, data )
    return self.dev:wr( data )
end

spidev.setup = function( self, speed, mode )
    return self.dev:setup( speed, mode )
end
