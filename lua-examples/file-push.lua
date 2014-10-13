--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"src=/remote/file/path.txt" -p"out=/path/to/local/file.txt"
--]]

open("io") -- open 'io' library for read and write file

file = fr.client.fs.file

function main( argv )

    print( 'Push file "', argv.src, '"', ' to "', argv.out, '"...' )

    f = file.open( argv.out, file.flags( file.WRONLY,
                                         file.CREAT,
                                         file.TRUNC  ) )  -- remote file
    out = io.open( argv.src, 'rb' )                       -- local file

    d = out:read( 0 )

    while d do
        file.write( f, d )
        d = out:read( 44000 )
    end

    println( 'success!' )
end
