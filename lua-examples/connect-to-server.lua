--[[
    % ferro_remote_client -clua -e script.lua -p"server=ip:port/local_name"
--]]


client = fr.client

function main( argv )

    if client.server then
        println( 'CLIENT IS ALREADY CONNECTED TO '..client.server )
    end

    println( 'Main table before: \n========\n', fr, '\n========' )
    print( 'Trying connect to '..argv.server..'..' )

    client.connect( argv.server )

    println( 'success!' )
    println( 'Main table after connect:\n>>>>>>>>\n', fr, '\n>>>>>>>>' )

    client.disconnect( )

    println( 'Main table after disconnect:\n<<<<<<<<\n', fr, '\n<<<<<<<<' )

end
