#ifndef FR_SUBSYSCONFIG_H
#define FR_SUBSYSCONFIG_H

#include "subsystem-iface.h"

#include <vector>
#include <map>

namespace boost { namespace program_options {
    class variables_map;
    class options_description;
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

        static
        void all_options( boost::program_options::options_description &desc );

        ~config( );

        static vtrc::shared_ptr<config> create( application *app,
                    const boost::program_options::variables_map &vm );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;

        boost::program_options::variables_map const &variables( ) const;

        const std::vector<std::string> &endpoints( ) const;
        void  set_endpoints( std::vector<std::string> const &ep ) const;
        const std::map<std::string, std::string> &id_keys( ) const;

        const std::string &script_path( ) const;

    };

}}}


#endif // FR_SUBSYSCONFIG_H
