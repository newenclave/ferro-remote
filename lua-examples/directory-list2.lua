-- show remote directory list

--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"path=/remote/path"
--]]

fs = fr.client.fs

function main( argv )

    println( 'Show directory "', argv.path, '"')

    path, iterator = fs.iter_begin( argv.path )

    local empty_or_not = { [false]='+', [true] = ' ' }

    while not fs.iter_end( iterator ) do

        info = fs.info( fs.iter_get( iterator ) )

        if info.failed then
            name = '?'..path..'?'
        elseif info.is_directory then
            name = empty_or_not[info.is_empty]..'['..path..']'
        else
            name = '  '..path
        end
        println( '  '..name )
        path = fs.iter_next( iterator )

    end

    fs.close( iterator ) -- not necessary
end

