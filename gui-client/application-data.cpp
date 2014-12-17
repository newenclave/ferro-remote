#include <memory>
#include "application-data.h"
#include "vtrc-common/vtrc-pool-pair.h"


namespace fr { namespace declarative {

    namespace vcomm = vtrc::common;

    struct application_data::impl {
        std::shared_ptr<vtrc::common::pool_pair> pools_;
    };

    application_data::application_data( )
        :impl_(new impl)
    {

    }

    application_data::~application_data( )
    {
        delete impl_;
    }

    vtrc::common::pool_pair &application_data::pools( )
    {
        return *impl_->pools_;
    }

    void application_data::reset_pools( unsigned io, unsigned rpc )
    {
        impl_->pools_ = std::make_shared<vcomm::pool_pair>( io, rpc );
    }

    const vtrc::common::pool_pair &application_data::pools( ) const
    {
        return *impl_->pools_;
    }

\
}}
