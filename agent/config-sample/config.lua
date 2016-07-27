--- test config for agent

-- io    - threads for io operations; default = 1
-- rpc   - for rpc calls; default = 1
-- only  - use only pool for all operations; default = false

pools.set { io = 1, rpc = 1, only = false }

-- string format:
--   x.x.x.x:yy 		- for tcp
--   :::yy      		- for tcp6
--   /path/to/sock 	- for UNIX local sockets
endpoint.add "0.0.0.0:12345"
endpoint.add "/home/data/fr.sock"

--- here are 
---   levels zero    = z = 0
---          error   = e = 1
---          warning = w = 2
---          info    = i = 3
---          debug   = d = 4
--- default = info
--- stdout 				- write to console
--- syslog 				- write to syslog
--- /path/to/file - for log file
logger.add "stdout" 					-- default info level
logger.add "syslog[e-w]" 			-- only errors to syslog
--logger.add "all.log[0-4]" 		-- all logs to all.log file


