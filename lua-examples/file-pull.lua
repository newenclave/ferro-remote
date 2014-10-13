--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"src=/remote/file/path.txt" -p"out=/path/to/local/file.txt"
--]]

open("io") -- open 'io' library for read and write file

file = fr.client.fs.file

function main( argv )

    println( 'Pull file "', argv.src, '"', ' to "', argv.out, '"' )

    f = file.open( argv.src, file.O_RDONLY ) -- remote file
    out = io.open( argv.out, 'wb' )          -- local file

    d = file.read( f, 44000 )

    while d ~= "" do
        out:write( d )
        d = file.read( f, 44000 )
    end

end
