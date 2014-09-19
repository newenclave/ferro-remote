#include "application.h"

#include <map>
#include <vector>
#include <sstream>
#include <stdexcept>

#include "vtrc-mutex.h"

namespace fr { namespace server {

    namespace vserver = vtrc::server;
    namespace vcommon = vtrc::common;

    typedef std::map<vtrc::common::rtti_wrapper, subsystem_sptr> subsys_map;
    typedef std::vector<subsystem_sptr>                          subsys_vector;

    typedef std::map<std::string, application::service_getter_type> service_map;

    struct subsystem_comtrainer {
        subsys_map      subsys_;
        subsys_vector   subsys_order_;
    };

    struct application::impl {

        vtrc::common::pool_pair &pools_;
        application             *parent_;
        subsystem_comtrainer     subsystems_;

        service_map              services_;
        vtrc::mutex              services_lock_;

        impl( vtrc::common::pool_pair &pools )
            :pools_(pools)
        { }

        application::service_wrapper_sptr get_service( const std::string &name,
                            vcommon::connection_iface_wptr &connection )
        {
            vtrc::lock_guard<vtrc::mutex> lck( services_lock_ );
            service_map::iterator f(services_.find( name ));

            if( f != services_.end( ) ) {
                return f->second( parent_, connection );
            } else {
                return application::service_wrapper_sptr( );
            }
        }
    };

    application::application( vtrc::common::pool_pair &pp )
        :vserver::application( pp )
        ,impl_(new impl(pp))
    {
        impl_->parent_ = this;
    }

    application::~application( )
    {
        delete impl_;
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
    }

    void application::add_subsystem( const std::type_info &info,
                                     subsystem_sptr inst )
    {
        impl_->subsystems_.subsys_[vcommon::rtti_wrapper(info)] = inst;
        impl_->subsystems_.subsys_order_.push_back( inst );
    }

    subsystem_iface *application::subsystem( const std::type_info &info )
    {
        subsys_map::iterator f( impl_->subsystems_
                               .subsys_
                               .find( vcommon::rtti_wrapper(info) ) );

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
                                      .find( vcommon::rtti_wrapper(info) ) );

        if( f == impl_->subsystems_.subsys_.end( ) ) {
            return NULL;
        } else {
            return f->second.get( );
        }
    }

    //// services
    ///

    void application::register_service_creator( const std::string &name,
                                                service_getter_type &func )
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

    void application::unregister_service_creator( const std::string &name )
    {
        vtrc::lock_guard<vtrc::mutex> lck(impl_->services_lock_);
        service_map::iterator f(impl_->services_.find( name ));
        if( f != impl_->services_.end( ) ) {
            impl_->services_.erase( f );
        }
    }

    /////// parent calls
    ///
    void application::configure_session( vcommon::connection_iface* connection,
                                         vtrc_rpc::session_options &opts )
    {

    }

    vtrc::shared_ptr<vcommon::rpc_service_wrapper>
         application::get_service_by_name(vcommon::connection_iface* c,
                                          const std::string &service_name )
    {
        vcommon::connection_iface_wptr wp(c->weak_from_this( ));
        return impl_->get_service( service_name, wp );
    }

    std::string application::get_session_key( vcommon::connection_iface* c,
                                               const std::string &id)
    {
        return std::string( );
    }

}}

