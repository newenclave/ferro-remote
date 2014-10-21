## fr table
```lua
fr = {

    client = {

        connect     = function( ip:port/local_name ) -> err, bool
        disconnect  = function( ) -> nil
        server      = "xxx.xxx.xxx.xxx:xxxxx" // if connected else nil

        os = {
            system = function("command" ) -> int
        },

        gpio = {

            available  = true, // true or false

            EDGE_NONE    = 0, // edge
            EDGE_RISING  = 1, // edge
            EDGE_FALLING = 2, // edge
            EDGE_BOTH    = 3, // edge

            DIRECT_IN   = 0, // direction
            DIRECT_OUT  = 1, // direction

            export          = function( gpio_id [, DIRECT_IN or DIRECT_OUT] ) -> device_instance
            unexport        = function( device_instance ) -> nil

            direction       = function( device_instance ) -> direction
            set_direction   = function( device_instance, direction ),

            value           = function( device_instance ) -> value
            set_value       = function( device_instance, value ) -> nil,

            edge_supported  = function( device_instance ) -> boolean,
            edge            = function( device_instance ) -> edge,
            set_edge        = function( device_instance, edge ) -> nil,

            register_for_change = function( device_instance, call_name, ... ) -> nil,
            unregister          = function( device_instance ) -> nil,

            info  = function( device_instance ) -> { ... },

            close = function( device_instance )
        },
        fs = {

            iter_begin  = function, // depricated
            iter_next   = function, // depricated
            iter_get    = function, // depricated
            iter_end    = function, // depricated

            read    = function( path_to_file, max_length ) -> data,
            write   = function( path_to_file, data ) -> nil,
            mkdir   = function( path_to_new_dir ),
            del     = function( path_to_new_dir ),
            rename  = function( path_to_old_name, path_to_new_name ),
            info    = function( path_to_old_name ) -> { ... },
            cd      = function( path_to_new_dir ),
            pwd     = function(  ) -> path_to_current_dir,
            stat    = function,

            close   = function( file or iterator ),

            iterator = {
                begin   = function( [path_to_directory] ) -> fs_element_name, iterator
                clone   = function( iterator ) -> new_iterator,
                next    = function( iterator ) -> fs_element_name,
                is_end  = function( iterator ) -> boolean,
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

                open    = function( path_to_file[, flags [, mode ]] ) -> file_inst, RDONLY is default
                read    = function( file_inst[, max_len] ) -> data,
                write   = function( file_inst, data) -> nil,
                flags   = function( flag1, flag2, ..., flagN ) -> flags,
                seek    = function( file_inst, seek_type[, position] ) -> position,
                tell    = function( file_inst ) -> position),
                flush   = function( file_inst ) -> nil,

                register_for_events = function( file_inst, handler_name, ... ) -> nil,
                unregister          = function( file_inst ) -> nil,

            },
        }
    }
}
```
