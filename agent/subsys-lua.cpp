
#include "application.h"
#include "subsys-lua.h"

#if FR_WITH_LUA

#include <iostream>

#include "agent-lua.h"
#include "subsys-config.h"
#include "subsys-logging.h"

//#include "vtrc-memory.h"

#define LOG(lev) log_(lev) << "[lua] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)

namespace fr { namespace agent { namespace subsys {

    namespace {

        using lua_table_type = std::map<std::string, std::string>;
        using lua_list_type  = std::vector<std::string>;

        const std::string subsys_name( "lua" );

        namespace frlua = ::fr::lua;

    }

    struct lua::impl {

        application     *app_;
        logger          &log_;
        frlua::state     state_;

        impl( application *app )
            :app_(app)
            ,log_(app_->get_logger( ))
        { }

        void reg_creator( const std::string &name,
                          application::service_getter_type func )
        {
            app_->register_service_factory( name, func );
        }

        void unreg_creator( const std::string &name )
        {
            app_->unregister_service_factory( name );
        }

        static
        bool is_pair( frlua::objects::base const *o )
        {
            return o->type_id( ) == frlua::objects::base::TYPE_PAIR;
        }

        static
        bool is_string( frlua::objects::base const *o )
        {
            return o->type_id( ) == frlua::objects::base::TYPE_STRING;
        }

        lua_list_type read_list( const std::string &path )
        {
            lua_list_type result;
            int level = state_.get_table( path.c_str( ) );
            if( level ) {
                frlua::objects::base_sptr t = state_.get_table( -1 );
                for( size_t i=0; i<t->count( ); ++i ) {
                    const frlua::objects::base *next( t->at( i ) );
                    if( is_pair( next ) && is_string( next->at( 1 ) ) ) {
                        result.push_back( next->at( 1 )->str( ) );
                    }
                }
                state_.pop( level );
            }
            return result;
        }

    };


    lua::lua( application *app )
        :impl_(new impl(app))
    { }

    lua::~lua( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<lua> lua::create( application *app )
    {
        vtrc::shared_ptr<lua> new_inst(new lua(app));
        return new_inst;
    }

    const std::string &lua::name( )  const
    {
        return subsys_name;
    }

    void lua::init( )
    { }

    void lua::start( )
    {
        impl_->LOGINF << "Started.";
    }

    void lua::stop( )
    {
        impl_->LOGINF << "Stopped.";
    }

    void lua::load_file( const std::string &path )
    {
        impl_->state_.check_call_error(
                    impl_->state_.load_file( path.c_str( ) ) );
    }

    lua::lua_string_list_type lua::get_table_list( const std::string &path )
    {
        return impl_->read_list( path );        
    }

}}}

#endif
