
#ifndef FR_SUBSYS_gpio_H
#define FR_SUBSYS_gpio_H

#include "subsystem-iface.h"
#include "gpio-helper.h"

namespace fr { namespace agent {

    class application;

namespace subsys {

    class gpio: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        gpio( application *app );

    public:

        ~gpio( );

        static vtrc::shared_ptr<gpio> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;

        gpio_helper_sptr open_pin( std::uint32_t pin );
        void close_pin( std::uint32_t pin );

    };

}}}

#endif

    
