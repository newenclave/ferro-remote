-- show remote directory list

--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"path=/remote/path"
--]]

fs     = fr.client.fs
fsiter = fs.iterator

function main( argv )

    println( 'Show directory "', argv.path, '"')

    path, iterator = fsiter.begin( argv.path )

    local empty_or_not = { [false]='+', [true] = ' ' }

    while not fsiter.is_end( iterator ) do

        info = fs.info( fsiter.get( iterator ) )

        if info.failed then
            name = '?'..path..'?'
        elseif info.is_directory then
            name = empty_or_not[info.is_empty]..'['..path..']'
        else
            name = '  '..path
        end
        println( '  '..name )
        path = fsiter.next( iterator )

    end

    fs.close( iterator ) -- not necessary
end

