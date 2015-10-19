import "spidev.lua"
    
function main( )
    local d = assert(spidev.new(0, 0) )
    d:setup(100000, 0)
    local res = assert(d:transfer( "\xff\xff" ) )
    for i = 1, #res do
        local c = res:byte(i,i)
        print(c)
    end
    fr.print( res, "\n" )
end
