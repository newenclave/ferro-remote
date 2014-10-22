
--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e script.lua \
        -p"dev=/dev/event2"
--]]

file = fr.client.fs.file

function main( argv )
    dev = file.open_device( argv.dev )
    while true do
        data = file.read( dev )
        println( data )
    end
end
