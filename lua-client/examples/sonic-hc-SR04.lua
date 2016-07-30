gpio = fr.client.gpio

local TRIG = 2
local ECHO = 3
local last_tick = 0 

function handler( data )
    -- fr.print( data )
    if last_tick ~= 0 then 
	local val = data.value
	if val == 1 then 
	    print( "signal sent!" )
	else  
	    print( "echo received; distance = ", (data.interval - last_tick)/58, "cm"  )
	    fr.exit( )
	end
    end   
    last_tick = data.interval
    
end


function main( )
    fr.run( )  
    T = assert(gpio.export(TRIG, "out", 0))
    E = assert(gpio.export(ECHO, "in",  1))
    assert(E:set( "edge", "both" ))
    assert(E:subscribe( "on_changed", handler, "E" ))
    assert(T:pulse( 50 ))
end

