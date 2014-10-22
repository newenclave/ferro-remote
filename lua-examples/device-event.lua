--[[
 =  open remote device
 =  subscribe to events
 =  do some work
 =  receive events asynchronously
 =  ....
 =  PROFIT!

    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"dev=/dev/xxxxx"

--]]

function handler( err, data, dev_name ) -- other lua thread !
    println( 'read from '..dev_name..' <- ', data ) --show this garbage
end

file = fr.client.fs.file

function main( argv ) -- main lua thread

    --[[open device as file
        f = file.open( "/dev/random", file.RDONLY )
        is also valid
    -- ]]

    f = file.open_device( argv.dev )

    file.register_for_events( f, "handler", argv.dev )
    while true do
        sleep( 1 )
        print( '.' ) --- some work! =)
    end

end

