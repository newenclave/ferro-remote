client = fr.client

function main( argv )
		if not fr.client.connected then
				fr.print( "Client is not connected. Use -s parameter for setup connection\n " )
				fr.exit( -1 )
				return
		end
		local path = argv[1]
		if not path then 
				fr.print( "Path not specified. Use -p parameter for setup path\n" )
		end
		client.fs.cd( path )
		fr.print( "Cd: ", path, "\n" )  
		local iterators = client.fs.iterator
		local i = iterators.begin( )
		fr.print(iterators.get( i ), "\n")
		while iterators.has_next( i ) do 
				iterators.next( i )
				fr.print(iterators.get( i ), "\n")
		end
		fr.exit(0)
		fr.print( "Exit\n" )
end

