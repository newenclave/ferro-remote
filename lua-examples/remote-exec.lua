-- execute command on the target system

--[[
    %ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e show-main-argv.lua \
        -p"command=some_remote_command"
--]]

function main( argv )
    print( 'Execute command: ', argv.command, '...' )
    fr.client.os.system( argv.command )
    println( 'success' )
end

