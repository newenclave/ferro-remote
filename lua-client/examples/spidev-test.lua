spi = fr.client.spi


function hex_dump(buf)
    for i=1,math.ceil(#buf/16) * 16 do
        if (i-1) % 16 == 0 then io.write(string.format('%08X  ', i-1)) end
        io.write( i > #buf and '   ' or string.format('%02X ', buf:byte(i)) )
        if i %  8 == 0 then io.write(' ') end
        if i % 16 == 0 then io.write( buf:sub(i-16+1, i):gsub('%c','.'), '\n' ) end
    end
end

function main( ) 
    local inbuf =  "\xFF\xFF\xFF\xFF\xFF\xFF\x40\x00\x00\x00\x00\x95\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xF0\x0D"
    local dev = assert(spi.open(0, 1))
    local res = dev:write( inbuf )
    print( "Input: " )
    hex_dump( inbuf )
    print( "Result:" )
    hex_dump( res )
end 
