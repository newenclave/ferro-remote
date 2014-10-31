#include "lua-interface.h"

#if FR_WITH_LUA

#include "interfaces/II2C.h"

#include "fr-lua/lua-wrapper.hpp"

namespace fr { namespace lua {

    namespace {
        struct data: public base_data {

            client::core::client_core    &cc_;

            data( std::shared_ptr<lua::state> &ls,
                  client::core::client_core &cc )
                :cc_(cc)
            { }

            virtual lua::objects::table_sptr init_table( )
            {
                objects::table_sptr t( new objects::table );

                using objects::new_string;
                using objects::new_function;
                using objects::new_light_userdata;

                t->add( new_string( names::inst_field ),
                        new_light_userdata( this ));

                return t;
            }
        };
    }

namespace i2c {

    const char *table_name( ) { return "i2c"; }
    const char *table_path( )
    {
        static const std::string path =
                std::string( names::client_table ) + '.' + table_name( );
        return path.c_str( );
    }

    data_sptr init( std::shared_ptr<lua::state> &ls,
                    client::core::client_core &cc )
    {
        return data_sptr( new data( ls, cc ) );
    }
}

}}

#endif
