--[[
    % ferro_remote_client -clua -e script.lua -p"server=ip:port/local_name"
--]]


client = fr.client

function main( argv )
    println( 'Main table before\n========\n', fr, '\n========' )
    print( 'Trying connect to '..argv.server..'..' )
    client.connect( argv.server )
    println( 'success!' )
    println( 'Main table after\n========\n', fr, '\n========' )
    client.disconnect( )
    println( 'Main table again\n========\n', fr, '\n========' )
end
