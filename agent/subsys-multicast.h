#ifndef FR_SUBSYS_multicast_H
#define FR_SUBSYS_multicast_H

#include "subsystem-iface.h"
#include "vtrc-common/vtrc-signal-declaration.h"

#include "boost/asio/ip/udp.hpp"

namespace fr { namespace agent {

    class application;

namespace subsys {

    struct multicast_request {
        boost::asio::ip::udp::socket   *sender = nullptr;
        boost::asio::ip::udp::endpoint *from   = nullptr;
        const std::uint8_t             *data   = nullptr;
        size_t                          length = 0;
    };

    struct multicast_response {
        bool gpio_available = false;
        std::vector<std::string> endpoints;
    };

    class multicast: public subsystem_iface {

        struct  impl;
        impl   *impl_;

        VTRC_DECLARE_SIGNAL( on_request,
                             void( const multicast_request &,
                                   multicast_response & ) );

    protected:

        multicast( application *app );

    public:

        ~multicast( );

        static vtrc::shared_ptr<multicast> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;
    };

}}}

#endif

    
