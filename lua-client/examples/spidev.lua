spidev = { 
    bus	    = 0
    channel = 0
}

spidev.new = function( bus, channel )
    local spi = fr.client.spi
    local dev = assert(spi.open( bus, channel ))
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

