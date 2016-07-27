#ifndef FR_SUBSYSCONFIG_H
#define FR_SUBSYSCONFIG_H

#include "subsystem-iface.h"

#include <vector>
#include <string>
#include <map>

namespace boost { namespace program_options {
    class variables_map;
    class options_description;
}}

namespace fr { namespace agent {

    class application;

namespace subsys {

    struct config_values {
        std::vector<std::string>            endpoints;
        std::vector<std::string>            loggers;
        std::size_t                         io_count  = 1;
        std::size_t                         rpc_count = 1;
        bool                                only_pool = false;
        std::map<std::string, std::string>  key_map;
        void clear( );
    };

    std::ostream &operator << ( std::ostream &o, const config_values &c );

    class config: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        config( application *app, const config_values &vm );

    public:

        static
        void all_options( boost::program_options::options_description &desc );

        ~config( );

        static vtrc::shared_ptr<config> create( application *app,
                    const config_values &vm );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;

        static config_values load_config(
                const boost::program_options::variables_map &vm );

        config_values &cfgs( );
        const config_values &cfgs( ) const;

        const std::string &script_path( ) const;

    };

}}}


#endif // FR_SUBSYSCONFIG_H
