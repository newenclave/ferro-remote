eq = fr.client.event_queue
fs = fr.client.fs
file = fs.file

v2r_device = "/dev/v2r_gpio"

function next_call( err, info )

	local function on_off( device, on )
	    local values = { [true] = "1", [false] = "0" }
	    file.write( device, "set gpio 74 output "..values[on] )
	end
	
	local vals = { 20, 20, 20, 60, 60, 60, 20, 20, 20 }
	   -- local symbol = { [0] = "S", [1] = "O", [2] = "S", [3] = "\n" } 
	local tout = { [true] = 20, [false] = 100 }
	local to   = tout[info.index < #vals]
	
	info.index = info.index % #vals
	
	info.on = not info.on 
	on_off( info.device, info.on )
	
	
	if not info.on then 
	    eq.timer.post( next_call, {[2] = to}, info )
	else
	    info.index = info.index + 1
	    eq.timer.post( next_call, {[2] = vals[info.index]}, info )
	end
	if info.index == 1 and info.on then 
					fr.print( "SOS\n" )
	end
end

function main( )
    if not fs.exists( v2r_device ) then
        fr.print( "Target system is not Virt2Real device." )
        return 
    end
    dev, err = file.open( v2r_device, file.flag.WRONLY | file.flag.SYNC )
    if err then 
        print( "Open device failed. ", err )
        return
    end
    fr.run(  ) -- start event loop
    next_call( nil, { device = dev, index = 0, on = false } )
end
