#ifndef FR_GPIO_HELPER_H
#define FR_GPIO_HELPER_H

#include <string>
#include <memory>
#include <functional>
#include "file-keeper.h"
#include "poll-reactor.h"

#include "boost/asio.hpp"

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

    using gpio_reaction = std::function<bool (unsigned, std::uint64_t)>;

    class gpio_helper {

        struct  impl;
        impl   *impl_;

    public:

        using queue_type = boost::asio::io_service::strand;

        gpio_helper( unsigned id, queue_type  &ios );
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

        std::uint32_t add_reactor_action( poll_reactor &react, std::uint32_t id,
                                          gpio_reaction cb );
        void del_reactor_action( poll_reactor &react, std::uint32_t id );
    };


    using gpio_helper_sptr = std::shared_ptr<gpio_helper>;

}}

#endif // GPIO_H
