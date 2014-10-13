## fr table
```lua
fr = {

    client = {

        os = {
            system = function@0x532424 // int system( "command" )
        },

        gpio = {

            available  = true, // true or false

            EDGE_NONE    = 0,
            EDGE_RISING  = 1,
            EDGE_FALLING = 2,
            EDGE_BOTH    = 3,

            DIRECT_IN   = 0,
            DIRECT_OUT  = 1,

            export      = function@0x643eac, // device export( gpio_id [, DIRECT_IN or DIRECT_OUT] )
            unexport    = function@0x644356, // void unexport( device )

            direction       = function@0x6448a3, // returns direction (direction( device ))
            set_direction   = function@0x6447a8,

            value           = function@0x64451f
            set_value       = function@0x644433,

            edge            = function@0x6446e1,
            set_edge        = function@0x6445e6,

            register_for_change = function@0x644aa0,
            unregister          = function@0x644e1d,

            info  = function@0x64411b,

            close = function@0x64496a, // close(device)
        },
        fs = {

            iter_begin  = function@0x50c2f6,
            iter_next   = function@0x50c609,
            iter_get    = function@0x50c7da,
            iter_end    = function@0x50c7da,

            read    = function@0x50b85a,
            write   = function@0x50b9fd,
            close   = function@0x50b34c,
            mkdir   = function@0x50b68c,
            del     = function@0x50b773,
            rename  = function@0x50b4ec,
            info    = function@0x50bfdc,
            cd      = function@0x50b265,
            pwd     = function@0x50b193,
            stat    = function@0x50bb91,

            file = {

                RDONLY   = 0,
                WRONLY   = 1,
                RDWR     = 2,
                CREAT    = 64,
                EXCL     = 128,
                TRUNC    = 512,
                APPEND   = 1024,
                NONBLOCK = 2048,
                ASYNC    = 8192,
                SYNC     = 1052672,

                IXOTH   = 1,
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
                IRWXU   = 448,

                SEEK_SET = 0,
                SEEK_CUR = 1,
                SEEK_END = 2,

                read    = function@0x50d06a,
                write   = function@0x50d22e,
                flags   = function@0x50ca34,
                seek    = function@0x50cdae,
                open    = function@0x50caff,
                flush   = function@0x50cfb3,
                tell    = function@0x50ceea,

                register_for_events = function@0x50d422,
                unregister          = function@0x50d79f,

            },
        }
    }
}
```
