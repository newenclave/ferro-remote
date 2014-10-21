-- show remote directory as tree

--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"path=/remote/path"
--]]

fs = fr.client.fs
fsiter = fs.iterator

if not fsiter then
    die "Client is too old."
end

function show_directory( level, directory )

    local leaf, iterator = fsiter.begin( directory )

    while not fsiter.is_end( iterator ) do

        local full_path = fsiter.get( iterator )
        local info      = fs.info( full_path )

        if info.is_directory then

            println( level..'['..leaf..']' )

            if not info.is_empty then
                show_directory( level..'  ', full_path )
            end

        else
            println( level..' '..leaf )
        end

        leaf = fsiter.next( iterator )

    end

    fs.close( iterator ) -- ok ...

end

function main( argv )
    println( 'Show directory "', argv.path, '"')
    fs.cd( argv.path ) -- change our path
    show_directory( '', argv.path )
end

