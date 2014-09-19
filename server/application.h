#ifndef FR_APPLICATION_H
#define FR_APPLICATION_H

#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-rtti-wrapper.h"

#include "subsystem-iface.h"

#include <assert.h>

namespace fr { namespace server {

    class application: public vtrc::server::application {

        struct         impl;
        impl          *impl_;
        friend struct  impl;

    public:

        application( vtrc::common::pool_pair &pp );
        ~application( );

    private:

        template <class Tgt, class Src>
        static Tgt * poly_downcast ( Src * x )
        {
            assert( dynamic_cast<Tgt>(x) == x );  // logic error ?
            return static_cast<Tgt>(x);
        }

    public: // subsystems

        template <typename T>
        void add_subsystem( )
        {
            subsystem_sptr subsys ( T::create( this ) );
            add_subsystem( typeid( T ), subsys );
        }

        template <typename T>
        T &subsystem( )
        {
            subsystem_iface *subsys = subsystem( typeid(T) );
            if( NULL == subsys ) {
                throw std::runtime_error( "Invalid subsystem" );
            }
            return *(poly_downcast<T *>( subsys ));
        }

        template <typename T>
        const T &subsystem( ) const
        {
            const subsystem_iface *subsys = subsystem( typeid(T) );
            if( NULL == subsys ) {
                throw std::runtime_error( "Invalid subsystem" );
            }
            return *(poly_downcast<const T *>( subsys ));
        }

        template <typename T>
        T *subsystem_safe( )
        {
            subsystem_iface *subsys = subsystem( typeid(T) );
            return poly_downcast<const T *>( subsys );
        }

        template <typename T>
        const T *subsystem_safe( ) const
        {
            const subsystem_iface *subsys = subsystem( typeid(T) );
            return poly_downcast<const T *>( subsys );
        }

    private:

        void add_subsystem( const std::type_info &info, subsystem_sptr inst );
        subsystem_iface *subsystem( const std::type_info &info );
        const subsystem_iface *subsystem( const std::type_info &info ) const;
    };

}}


#endif // FR_APPLICATION_H
