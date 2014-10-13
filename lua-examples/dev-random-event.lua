--[[
 =  open remote /dev/random
 =  subscribe to events
 =  do some work
 =  receive events asynchronously
 =  ....
 =  PROFIT!
--]]

function handler( err, data, params ) -- other lua thread !
    println( 'read from '..params.path, ' <- ', data ) --show this garbage
end

file = fr.client.fs.file

function main( argv ) -- main lua thread

    f = file.open( "/dev/random", file.RDONLY )
    file.register_for_events( f, "handler", { path = "/dev/random" } )
    while true do
        sleep( 1 )
        print( '.' ) --- some work! =)
    end

end
