-- show stat for the remote path
--[[
%ferro_remote_client -s xx.xx.xx.xx:xxxx -clua -e show-main-argv.lua \
        -p"path=/remote/path/to/file.ext"
--]]

fs = fr.client.fs

function main( argv )
    printiln( 'Stat for file "', argv.path, '": ', fs.stat( argv.path ) )
end
