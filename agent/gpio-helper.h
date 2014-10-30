#ifndef FR_GPIO_H
#define FR_GPIO_H

#include <string>
#include "file-keeper.h"

namespace fr { namespace agent {

    namespace gpio {

        enum direction_type {
             DIRECT_IN   = 0
            ,DIRECT_OUT  = 1
        };

        enum edge_type {
             EDGE_NONE     = 0
            ,EDGE_RISING   = 1
            ,EDGE_FALLING  = 2
            ,EDGE_BOTH     = 3
        };

        bool available( );
    }

    class gpio_helper {

        struct  impl;
        impl   *impl_;

    public:

        gpio_helper( unsigned id );
        ~gpio_helper(  );

        bool exists( ) const;
        unsigned id( ) const;

        void exp( ) const;
        void unexp( ) const;

        gpio::direction_type direction( ) const;
        void  set_direction( gpio::direction_type val ) const;

        bool edge_supported( ) const;
        gpio::edge_type edge( ) const;
        void  set_edge( gpio::edge_type val ) const;

        static unsigned value_by_fd( int fd );

        unsigned value( ) const;
        void set_value( unsigned val ) const;

        unsigned active_low( ) const;
        void set_active_low( unsigned val ) const;

        int value_fd( ) const;
        bool value_opened( ) const;

    };

}}

#endif // GPIO_H
