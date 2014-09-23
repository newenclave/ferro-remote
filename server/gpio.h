#ifndef FR_GPIO_H
#define FR_GPIO_H

#include <string>
#include "file-keeper.h"

namespace fr { namespace server {

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

        gpio::edge_type edge( ) const;
        void  set_edge( gpio::edge_type val ) const;

        unsigned value( ) const;
        void set_value( unsigned val ) const;

        int value_fd( ) const;

    };

}}

#endif // GPIO_H
