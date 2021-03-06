-- show remote directory as tree

--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"path=/remote/path"
--]]

fs = fr.client.fs
fsiter = fs.iterator

function show_directory( level, parent, subdir )

    local current_path = parent

    if subdir then --- change directory is not first call
        current_path = parent..'/'..subdir
        fs.cd( current_path )
    end

    local path, iterator = fsiter.begin( )

    while not fsiter.is_end( iterator ) do

        info = fs.info( path )

        if info.is_directory then

            println( level..'['..path..']' )

            if not info.is_empty then
                show_directory( level..'  ', current_path, path )
            end

        else
            println( level..' '..path )
        end
        path = fsiter.next( iterator )
    end

    fs.close( iterator ) -- ok ...
    fs.cd( parent ) -- revert directory
end

function main( argv )
    println( 'Show directory "', argv.path, '"')
    fs.cd( argv.path ) -- change our path
    show_directory( '', argv.path )
end

