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
    println( 'read from ', dev_name, ' <- ', data ) --show data
end

function main( argv ) -- main lua thread

    f, err = file.open_device( argv.dev )

    if err then
        println( "Open device failed: ", err )
        return
    end

    -- file.register_for_events( f, 'handler', argv.dev ) -- also possible
    -- file.register_for_events( f, handler, argv.dev )   -- also possible

    file.register_for_events( f,
        function( err, data ) -- other thread used will be
            println( 'read from ', argv.dev, ' <- ', data ) --show data
        end
    )

    local i = 0;

    while i < 2 do
        sleep( 1 )
        print( '.' ) --- some work! =)
        i = i + 1
    end

    file.unregister( f )

end

