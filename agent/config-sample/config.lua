--- test config for agent

agent.set_name "Virt2Real Test Agent"

-- agent.daemon( true )

-- io    - threads for io operations; default = 1
-- rpc   - for rpc calls; default = 1
-- only  - use only pool for all operations; default = false

pools.set { io = 1, rpc = 1, only = false }

-- string format:
--   x.x.x.x:yy 		- for tcp
--   :::yy      		- for tcp6
--   /path/to/sock 	- for UNIX local sockets
endpoint.add "0.0.0.0:12345"
endpoint.add "/tmp/fr.sock"

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
logger.add "syslog[e-w]" 			-- only errors and warnings to syslog

--logger.add "all.log[0-4]" 		-- all logs to all.log file


-- multiast
--[[
/*********************************************************************************

  Here is some info about multicast addresses

  There are 5 types of addresses:
+-------------------------------------------------------------------------------+
|                    |          |                    IPv4                       |
|  1: Scope          | 2: IPv6  |-----------------------------------------------|
|                    |    scope | 3:  TTL scope |   4:  Administrative scope    |
|-------------------------------------------------------------------------------|
| Interface-local    |     1    |       0       |                               |
| Link-local         |     2    |       1       |   224.0.0.0 - 224.0.0.255     |
| Site-local         |     5    |      <32      | 239.255.0.0 - 239.255.255.255 |
| Organization-local |     8    |               | 239.192.0.0 - 239.195.255.255 |
| Global             |    14    |     <=255     |   224.0.1.0 - 238.255.255.255 |
+-------------------------------------------------------------------------------+

 IPv6 multicast structure:
 1 byte             : 0xff
 1 byte
      \ high  4 bits: flags
      \ low   4 bits: IPv6 scope
 80 bytes           : zeros
 Last 4 bytes       : group ID

**********************************************************************************/
]]

mcast.add "239.194.1.3:18852" -- ipv4
mcast.add "ff08::13:18853"    -- ipv6


