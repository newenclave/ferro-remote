-- show main fr.* table

open( "base" )

function print_table ( t, space )
    for k, v in pairs( t ) do
        printi( space, k, ' = ' )
        if type( v ) == "table" then
            println( '{' )
            print_table ( v, space..'  ' )
            println( space, '}' )
        else
            printiln( v )
        end
    end
end

function main( argv )
    print_table( fr, '' )
    -- println( fr ) -- internal fr printer can print tables
end
