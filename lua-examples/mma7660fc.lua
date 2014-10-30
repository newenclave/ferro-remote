-- test mma7660fc device as file

file = fr.client.fs.file

open( "string" )

function main ( argv )

    f = file.open( "/dev/i2c-1", file.RDWR )

    file.ioctl( f, 0x0703, 0x4c )
    file.write( f, "\7\1" )

    while true do

        d = file.read( f, 10 )

        printi( "X=", d:byte( 1 ), "\t",
                "Y=", d:byte( 2 ), "\t",
                "Z=", d:byte( 3 ),
                "                   \n" )

        --sleep( 1 )
    end

end
