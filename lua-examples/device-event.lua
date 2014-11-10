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

file = fr.client.fs.file

function handler( err, data, dev_name ) -- other lua thread !
    println( 'read from '..dev_name..' <- ', data ) --show data
end

function main( argv ) -- main lua thread

    f, err = file.open_device( argv.dev )

    if err then
        println( "Open device failed: ", err )
        return
    end

    file.register_for_events( f, 'handler', argv.dev )

    local i = 0;

    while true do
        sleep( 1 )
        print( '.' ) --- some work! =)
    end

    file.unregister( f )

end

