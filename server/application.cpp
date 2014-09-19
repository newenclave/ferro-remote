#include "application.h"

#include <map>
#include <vector>

namespace fr { namespace server {

    namespace vserver = vtrc::server;
    namespace vcommon = vtrc::common;

    typedef std::map<vtrc::common::rtti_wrapper, subsystem_sptr> subsys_map;
    typedef std::vector<subsystem_sptr>                          subsys_vector;

    struct subsystem_comtrainer {
        subsys_map      subsys_;
        subsys_vector   subsys_order_;
    };

    struct application::impl {
        application          *parent_;
        subsystem_comtrainer  subsystems_;
    };

    application::application( vtrc::common::pool_pair &pp )
        :vserver::application( pp )
        ,impl_(new impl)
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

}}

