-- show remote directory list

--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"path=/remote/path"
--]]

fs = fr.client.fs

function main( argv )

    println( 'Show directory "', argv.path, '"')

    fs.cd( argv.path ) -- change our path

    path, iterator = fs.iter_begin( )

    while not fs.iter_end( iterator ) do

        info = fs.info( path )

        if info.failed then
            name = '?'..path..'?'
        elseif info.is_directory then
            name = '['..path..']'
        else
            name = ' '..path
        end
        println( '  '..name )
        path = fs.iter_next( iterator )
    end
    fs.close( iterator ) -- not necessary
end

