#ifndef FR_SUBSYSCONFIG_H
#define FR_SUBSYSCONFIG_H

#include "subsystem-iface.h"

namespace boost { namespace program_options {
    class variables_map;
}}

namespace fr { namespace agent {

    class application;

namespace subsys {

    class config: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        config( application *app,
                const boost::program_options::variables_map &vm );

    public:

        ~config( );

        static vtrc::shared_ptr<config> create( application *app,
                    const boost::program_options::variables_map &vm );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;

        boost::program_options::variables_map const &variables( ) const;
    };

}}}


#endif // FR_SUBSYSCONFIG_H
