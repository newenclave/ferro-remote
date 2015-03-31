-- show parameters for 'main'
--[[
     %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e show-main-argv.lua \
        -p"param1=value1" -p"param2=value2" ... -p"paramN=valueN"
--]]

function main( argv )
    println( argv )
end
