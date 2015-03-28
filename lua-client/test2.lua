file = fr.client.fs.file
eq   = fr.client.event_queue

counter = { tasks = 0, total = 0 }

function starttask( )
    counter.tasks = counter.tasks + 1 
end

function stoptask( )
    counter.tasks = counter.tasks - 1 
    if counter.tasks == 0 then 
	fr.exit( 0 )
    end
end

function get_file( path, portion )
    local function impl( info )
        local d, err = file.read( info.id, info.read )
        if err then
            fr.print( "read error: ", err, "\n" )
            stoptask( )
            return
        end 
        local l = string.len(d)
        if l ~= 0  then
            --fr.print( "got data: ", info["path"], "; '", d, "'\n" )
            info.total = info.total + l
            counter.total = counter.total + l
            eq.post( impl, info )
        else 
            stoptask( )
            fr.print( "get file "..info["path"].." complete. total bytes: ", info.total, "\n" )
        end
    end
    local info = { id = nil, 
		   ["path"] = path, 
		   read = portion,
		   total = 0 }
    info.id, err = file.open( path )
    if err then 
        fr.print( "open error: ", path, "; ", err, "\n" )
        return
    end
    starttask( )
    eq.post( impl, info )
end

function main ( argv ) 
    for i, v in pairs( argv ) do 
        fr.print( "Add file: ", v, "\n" ) 
        get_file( v, 41227 )
    end
    local function hb_timer( )
	fr.print( counter,  "\n" ) 
	eq.timer.post( hb_timer, 1 )
    end
    hb_timer( )
end

