-- console eval 

con = fr.client.console

function handler( data )
				local function trim(s)
								local match = string.match
								return string.match(s,'^()%s*$') and '' or match(s,'^%s*(.*%S)')
				end				
				local s = trim(data.data)
				if s ~= "" then 
								local f = load( "" .. s  .. ""  )
				end 
				fr.print( "\n$ " )
end

function main( )
				con.subscribe( "on_read", handler )
				fr.print( "$ " )
				fr.run( )
end
