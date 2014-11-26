#ifndef FR_APPLICATIONDATA_H
#define FR_APPLICATIONDATA_H

namespace vtrc { namespace common {
    class pool_pair;
}}

namespace fr { namespace declarative {

    class application_data {

        struct impl;
        impl  *impl_;

    public:
        application_data( );
        ~application_data( );
    public:
        vtrc::common::pool_pair &pools( );
        void reset_pools( unsigned io, unsigned rpc );
        const vtrc::common::pool_pair &pools( ) const;
    };

    extern application_data global_app_data;

}}

#endif // APPLICATIONDATA_H
