## Global calls

```lua
    print    = function( ... )  // prints numbers as Float
    println  = function( ... )  // prints numbers as Float
    printi   = function( ... )  // prints numbers as Integers
    printiln = function( ... )  // prints numbers as Integers
    die      = function( reason ) // quits execution with 'reason' error
    open     = function( libaname or path_to_lua_script )
            available libs: 'base', 'table', 'io', 'math',
                            'debug', 'package', 'string', 'os'

    sleep    = function( seconds )
```

## fr table
```lua
fr = {

    client = {

        connect     = function( ip:port/local_name ) -> err, bool
        disconnect  = function( ) -> nil
        server      = "xxx.xxx.xxx.xxx:xxxxx" // if connected else nil

        os = {
            system  = function("command" ) -> int
            execute = function("command" ) -> int // the same as 'system'
        },

        gpio = {

            available  = true, // true or false

            EDGE_NONE    = 0, // edge
            EDGE_RISING  = 1, // edge
            EDGE_FALLING = 2, // edge
            EDGE_BOTH    = 3, // edge

            DIRECT_IN   = 0, // direction
            DIRECT_OUT  = 1, // direction

            export          = function( gpio_id[, direction] ) -> device_instance
            unexport        = function( device_instance ) -> nil

            direction       = function( device_instance ) -> direction
            set_direction   = function( device_instance, direction ),

            value           = function( device_instance ) -> value
            set_value       = function( device_instance, value ) -> nil,

            --/// make_pulse sets pin's value to 'set_value',
            --/// sleeps pulse_length microseconds
            --/// sets pin's value to 'reset_value',
            make_pulse      = function( device_instance, pulse_length[, set_value = 1[, reset_value = 0]] ) -> nil,

            edge_supported  = function( device_instance ) -> boolean,
            edge            = function( device_instance ) -> edge,
            set_edge        = function( device_instance, edge ) -> nil,

            register_for_change = function( device_instance, call_name, ... ) -> nil,
            unregister          = function( device_instance ) -> nil,

            info  = function( device_instance ) -> { ... },

            close = function( device_instance )
        },

        i2c = {
            bus_available = function( bus_id ) -> boolean,

            open          = function( bus_id[, slave_addr[, force_slave]] ) ->device
            close         = function( device ) -> nil

            functions     = function( device ) -> table { }

            read_byte     = function( device, command ) -> number
            read_byte     = function( device, { command1, command2 } ) -> { command1=value1, command2=value2 }
            write_byte    = function( device, command, value ) -> nil
            write_byte    = function( device, { command1=value1, command2=value2 } ) -> nil

            set_address   = function( device, slave_addr ) -> nil

            read          = function( device[, maximum] ) -> string
            write         = function( device, data ) -> nil

            read_block    = function( device, command ) -> string
            write_block   = function( device, command, value ) -> nil

            read_word     = function( device, command ) -> number
            read_word     = function( device, { command1, command2 } ) -> { command1=value1, command2=value2 }
            write_word    = function( device, command, value ) -> nil
            write_word    = function( device, { { command1=value1, command2=value2 } ) -> nil

            ioctl         = function( device, code, parameter ) -> error

        },

        fs = {
        
            read    = function( path_to_file, max_length ) -> data,
            write   = function( path_to_file, data ) -> nil,
            mkdir   = function( path_to_new_dir ),
            del     = function( path_to_new_dir ),
            rename  = function( path_to_old_name, path_to_new_name ),
            info    = function( path_to_old_name ) -> { ... },
            cd      = function( path_to_new_dir ),
            pwd     = function(  ) -> path_to_current_dir,
            stat    = function(  path_to_fs_object ) -> { ... },

            close   = function( file or iterator ),

            iterator = {
                begin   = function( [path_to_directory] ) -> fs_element_name, iterator
                clone   = function( iterator ) -> new_iterator,
                next    = function( iterator ) -> fs_element_name,
                is_end  = function( iterator ) -> boolean, -- cant use end =(
                get     = function( iterator ) -> full_path_of_fs_element
            },

            file = {

                RDONLY   = 0,        // flags
                WRONLY   = 1,
                RDWR     = 2,
                CREAT    = 64,
                EXCL     = 128,
                TRUNC    = 512,
                APPEND   = 1024,
                NONBLOCK = 2048,
                ASYNC    = 8192,
                SYNC     = 1052672,  // flags

                IXOTH   = 1,         // mode
                IWOTH   = 2,
                IROTH   = 4,
                IRWXO   = 7,
                IXGRP   = 8
                IWGRP   = 16,
                IRGRP   = 32,
                IRWXG   = 56,
                IXUSR   = 64,
                IWUSR   = 128,
                IRUSR   = 256,
                IRWXU   = 448,      // mode

                SEEK_SET = 0,       // seek_type
                SEEK_CUR = 1,       // seek_type
                SEEK_END = 2,       // seek_type

                open        = function( path_to_file[, flags [, mode ]] ) -> file_inst, 
                              flags = RDONLY is default
                open_device = function( path_to_file[, flags [, mode ]] ) -> file_inst, 
                              flags = RDONLY is default;
                              Every time you read or write this file,
                              agent sets its position to 0 after operation;
                              Easy way to read v2r_xxx devices =)
                ioctl       = function( code, parameter ) -> nil
                read        = function( file_inst[, max_len] ) -> data,
                write       = function( file_inst, data) -> nil,
                flags       = function( flag1, flag2, ..., flagN ) -> flags,
                seek        = function( file_inst, seek_type[, position] ) -> position,
                tell        = function( file_inst ) -> position),
                flush       = function( file_inst ) -> nil,

                register_for_events = function( file_inst, handler_name, ... ) -> nil,
                unregister          = function( file_inst ) -> nil,

            },
        }
    }
}
```
