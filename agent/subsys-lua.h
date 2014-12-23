
#ifndef FR_SUBSYS_LUA_H
#define FR_SUBSYS_LUA_H

#include "subsystem-iface.h"
#include "ferro-remote-config.h"

#if FR_WITH_LUA

namespace fr { namespace agent {

    class application;

namespace subsys {

    class lua: public subsystem_iface {

        struct  impl;
        impl   *impl_;

    protected:

        lua( application *app );

    public:

        typedef std::map<std::string, std::string>  lua_table_type;
        typedef std::vector<std::string>            lua_string_list_type;

        ~lua( );

        static vtrc::shared_ptr<lua> create( application *app );

        const std::string &name( )  const;

        void init( )  ;
        void start( ) ;
        void stop( )  ;

        void load_file( const std::string &path );

        lua_string_list_type get_table_list( const std::string &path );

    };

}}}

#endif
#endif // LUA
    
