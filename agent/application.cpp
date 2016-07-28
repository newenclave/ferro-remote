#include "application.h"

#include <map>

#include <iostream>
#include <vector>
#include <sstream>
#include <stdexcept>

#include "vtrc-mutex.h"
#include "vtrc-bind.h"

#include "subsys-config.h"
#include "subsys-logging.h"

#include "vtrc-common/vtrc-hash-iface.h"

#include "google/protobuf/descriptor.h"
#include "protocol/ferro.pb.h"
#include "vtrc-common/protocol/vtrc-rpc-lowlevel.pb.h"

#include "boost/program_options.hpp"

#include "vtrc-common/vtrc-closure-holder.h"
#include "vtrc-common/vtrc-connection-list.h"

#include "thread-prefix.h"


#define LOG(lev) logger_(lev) << "[app] "
#define LOGINF   LOG(logger::level::info)
#define LOGDBG   LOG(logger::level::debug)
#define LOGERR   LOG(logger::level::error)
#define LOGWRN   LOG(logger::level::warning)
namespace fr { namespace agent {

    namespace vserv = vtrc::server;
    namespace vcomm = vtrc::common;
    namespace gpb   = google::protobuf;

#ifdef FR_RTTI_DISABLE
    typedef std::map<std::uintptr_t, subsystem_sptr> subsys_map;
#else
    typedef std::map<vcomm::rtti_wrapper, subsystem_sptr> subsys_map;
#endif

    typedef std::vector<subsystem_sptr>                   subsys_vector;

    typedef std::map<std::string, application::service_getter_type> service_map;

    typedef std::map<std::string, std::string> key_map_type;

    struct subsystem_comtrainer {
        subsys_map      subsys_;
        subsys_vector   subsys_order_;
    };

    class internal_calls: public fr::proto::internal {
        application *app_;
    public:
        internal_calls( application *app )
            :app_(app)
        { }

        void exit_process( ::google::protobuf::RpcController* /*controller*/,
                           const ::fr::proto::empty*          /*request*/,
                           ::fr::proto::empty*                /*response*/,
                           ::google::protobuf::Closure* done ) override
        {
            vcomm::closure_holder holder(done);
            app_->get_logger( )
                    ( logger::level::info ) << "Shutdown service ...";
            app_->quit( );
        }

        void ping( ::google::protobuf::RpcController* controller,
                   const ::fr::proto::ping_req* request,
                   ::fr::proto::ping_res* response,
                   ::google::protobuf::Closure* done) override
        {
            // do nothing!
            vcomm::closure_holder holder(done);
        }

        static const std::string &name( )
        {
            return fr::proto::internal::descriptor( )->full_name( );
        }

        static application::service_wrapper_sptr create_call_inst(
                                      fr::agent::application *app,
                                      vtrc::common::connection_iface_wptr cl)
        {
            vtrc::shared_ptr<internal_calls>
                    inst( vtrc::make_shared<internal_calls>( app ) );

            return app->wrap_service( cl, inst );
        }
    };

    struct application::impl {

        vtrc::common::pool_pair &pools_;
        application             *parent_;
        subsystem_comtrainer     subsystems_;
        logger                   logger_;

        service_map              services_;
        vtrc::mutex              services_lock_;

        key_map_type             keys_;
        std::string              empty_key_;

        impl( vtrc::common::pool_pair &pools, const key_map_type &keys )
            :pools_(pools)
            ,logger_(pools_.get_io_service( ), logger::level::debug)
        {
            init_keys( keys );
        }

        application::service_wrapper_sptr get_service( const std::string &name,
                            vcomm::connection_iface_wptr &connection )
        {
            vtrc::lock_guard<vtrc::mutex> lck( services_lock_ );
            service_map::iterator f(services_.find( name ));

            if( f != services_.end( ) ) {
                return f->second( parent_, connection );
            } else {
                return application::service_wrapper_sptr( );
            }
        }

        void init_keys( const key_map_type &km )
        {
            keys_.insert( km.begin( ), km.end( ) );
            key_map_type::const_iterator f(keys_.find( "" ));
            if( f != keys_.end( ) ) {
                empty_key_ = f->second;
                keys_.erase( f );
            }
        }
    };

//////// service wrapper

    application::service_wrapper_impl::service_wrapper_impl( application *app,
                              vcomm::connection_iface_wptr c,
                              vtrc::shared_ptr<gpb::Service> serv)
        :vcomm::rpc_service_wrapper( serv )
        ,app_(app)
        ,client_(c)
    { }

    application::service_wrapper_impl::~service_wrapper_impl( )
    { }

    const
    application::service_wrapper_impl::method_type *
                application::service_wrapper_impl
                    ::get_method( const std::string &name ) const
    {
        const method_type* m = super_type::find_method( name );
        return m;
    }


    application *application::service_wrapper_impl
                            ::get_application( )
    {
        return app_;
    }

    const application *application::service_wrapper_impl
                                  ::get_application( ) const
    {
        return app_;
    }

/////////////////////////////

    application::service_wrapper_sptr
        application::wrap_service( vcomm::connection_iface_wptr cl,
                                   service_wrapper_impl::service_sptr serv )
    {
        return vtrc::make_shared<application::service_wrapper>(this, cl, serv);
    }

    application::application( vtrc::common::pool_pair &pp,
                              const std::map<std::string, std::string> &keys )
        :vserv::application( pp )
        ,impl_(new impl(pp, keys))
    {
        impl_->parent_ = this;
        register_service_factory( internal_calls::name( ),
                                  internal_calls::create_call_inst );
    }

    application::~application( )
    {
        delete impl_;
    }

    void application::quit( )
    {
        stop_all( );
        impl_->pools_.stop_all( );
    }

    void application::start_all( )
    {
        typedef subsys_vector::iterator iter_type;
        subsys_vector &vec(impl_->subsystems_.subsys_order_);

        for( iter_type b(vec.begin( )), e(vec.end( )); b!=e; ++b ) {
            (*b)->init( );
        }

        for( iter_type b(vec.begin( )), e(vec.end( )); b!=e; ++b ) {
            (*b)->start( );
        }
    }

    void application::stop_all( )
    {
        typedef subsys_vector::reverse_iterator iter_type;
        subsys_vector &vec(impl_->subsystems_.subsys_order_);

        for( iter_type b(vec.rbegin( )), e(vec.rend( )); b!=e; ++b ) {
            (*b)->stop( );
        }

        {
            vtrc::lock_guard<vtrc::mutex> lck(impl_->services_lock_);
            impl_->services_.clear( );
        }

        stop_all_clients( );
        get_clients( )->clear( );

    }

    agent::logger &application::get_logger( )
    {
        return impl_->logger_;
    }

    const agent::logger &application::get_logger( ) const
    {
        return impl_->logger_;
    }

    const std::string &application::thread_prefix( )
    {
        return get_thread_prefix( );
    }

#ifdef FR_RTTI_DISABLE
    void application::add_subsystem( std::uintptr_t info, subsystem_sptr inst )
    {
        impl_->LOGDBG << "Add subsystem '"<< inst->name( ) << "' as 0x"
                      << std::hex << info;

        impl_->subsystems_.subsys_[info] = inst;
        impl_->subsystems_.subsys_order_.push_back( inst );
    }

    subsystem_iface *application::subsystem( std::uintptr_t info )
    {
        subsys_map::iterator f( impl_->subsystems_.subsys_.find( info ) );

        if( f == impl_->subsystems_.subsys_.end( ) ) {
            return NULL;
        } else {
            return f->second.get( );
        }
    }

    const subsystem_iface *application::subsystem( std::uintptr_t info ) const
    {
        subsys_map::const_iterator f( impl_->subsystems_.subsys_.find( info ) );

        if( f == impl_->subsystems_.subsys_.end( ) ) {
            return NULL;
        } else {
            return f->second.get( );
        }
    }

#else
    void application::add_subsystem( const std::type_info &info,
                                     subsystem_sptr inst )
    {
        impl_->subsystems_.subsys_[vcomm::rtti_wrapper(info)] = inst;
        impl_->subsystems_.subsys_order_.push_back( inst );
    }

    subsystem_iface *application::subsystem( const std::type_info &info )
    {
        subsys_map::iterator f( impl_->subsystems_
                               .subsys_
                               .find( vcomm::rtti_wrapper(info) ) );

        if( f == impl_->subsystems_.subsys_.end( ) ) {
            return NULL;
        } else {
            return f->second.get( );
        }
    }

    const subsystem_iface *application::subsystem(
                                             const std::type_info &info ) const
    {
        subsys_map::const_iterator f( impl_->subsystems_
                                      .subsys_
                                      .find( vcomm::rtti_wrapper(info) ) );

        if( f == impl_->subsystems_.subsys_.end( ) ) {
            return NULL;
        } else {
            return f->second.get( );
        }
    }
#endif
    ///
    /// services
    ///
    void application::register_service_factory( const std::string &name,
                                                service_getter_type func )
    {
        vtrc::lock_guard<vtrc::mutex> lck(impl_->services_lock_);
        service_map::iterator f(impl_->services_.find( name ));
        if( f != impl_->services_.end( ) ) {
            std::ostringstream oss;
            oss << "Service '" << name << "'' already exists.";
            throw std::runtime_error( oss.str( ) );
        }
        impl_->services_.insert( std::make_pair( name, func ) );
    }

    void application::unregister_service_factory( const std::string &name )
    {
        vtrc::lock_guard<vtrc::mutex> lck(impl_->services_lock_);
        service_map::iterator f(impl_->services_.find( name ));
        if( f != impl_->services_.end( ) ) {
            impl_->services_.erase( f );
        }
    }

    ///
    /// parent calls
    ///
    void application::configure_session( vcomm::connection_iface*/*connection*/,
                                         vtrc::rpc::session_options &opts )
    {

        opts.CopyFrom( vtrc::common::defaults::session_options( ) );
    }

    application::parent_service_sptr
        application::get_service_by_name ( vcomm::connection_iface* c,
                                           const std::string &service_name )
    {
        vcomm::connection_iface_wptr wp(c->weak_from_this( ));
        return impl_->get_service( service_name, wp );
    }

    std::string application::get_session_key( vcomm::connection_iface* c,
                                               const std::string & /*id*/ )
    {
        key_map_type::const_iterator f( impl_->keys_.find( c->id( ) ) );

        if( f == impl_->keys_.end( ) ) {
            return impl_->empty_key_;
        } else {
            return f->second;
        }
    }

    bool application::session_key_required( vtrc::common::connection_iface* c,
                                            const std::string & /*id*/ )
    {
        return !impl_->keys_.empty( ) || !impl_->empty_key_.empty( );
    }

}}

