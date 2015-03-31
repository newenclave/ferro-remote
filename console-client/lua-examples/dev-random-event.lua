--[[
 =  open remote /dev/random
 =  subscribe to events
 =  do some work
 =  receive events asynchronously
 =  ....
 =  PROFIT!
--]]

function handler( err, data ) -- other lua thread !
    println( 'read from /dev/random <- ', data ) --show this garbage
end

file = fr.client.fs.file

function main( argv ) -- main lua thread

    --[[open device as file
        f = file.open( "/dev/random", file.RDONLY )
        is also valid
    -- ]]

    f = file.open_device( "/dev/random", file.RDONLY )

    file.register_for_events( f, "handler" )
	while true do
		sleep( 1 )
		print( '.' ) --- some work! =)
	end

end

