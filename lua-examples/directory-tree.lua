-- show remote directory as tree

--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e show-main-argv.lua \
        -p"path=/remote/path"
--]]

fs = fr.client.fs

function show_subdir( level, parent, subdir )

    local current_path = parent

    if subdir then --- change directory is not first call
        current_path = parent..'/'..subdir
        fs.cd( current_path )
    end

    local path, iterator = fs.iter_begin( )

    while not fs.iter_end( iterator ) do
        info = fs.info( path )
        if info.is_directory then
            name = '['..path..']'
            show_subdir( level..'  ', current_path, path )
        else
            name = ' '..path
        end
        println( level..name )
        path = fs.iter_next( iterator )
    end

    fs.cd( parent ) -- revert directory
end

function main( argv )
    println( 'Show directory "', argv.path, '"')
    fs.cd( argv.path ) -- change our path
    show_subdir( '', argv.path )
end

