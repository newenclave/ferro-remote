#ifndef FR_APPLICATION_H
#define FR_APPLICATION_H

#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-rtti-wrapper.h"
#include "vtrc-common/vtrc-connection-iface.h"

#include "subsystem-iface.h"

#include "vtrc-function.h"

#include <assert.h>

namespace fr { namespace server {

    class application: public vtrc::server::application {

        struct         impl;
        impl          *impl_;
        friend struct  impl;

    public:

        typedef vtrc::common::rpc_service_wrapper service_wrapper;
        typedef vtrc::shared_ptr<service_wrapper> service_wrapper_sptr;

        ///
        /// func( app, connection )
        ///
        typedef vtrc::function<
            service_wrapper_sptr ( fr::server::application *,
                                   vtrc::common::connection_iface_wptr )
        > service_getter_type;

        application( vtrc::common::pool_pair &pp );
        ~application( );

    private:

        template <class Tgt, class Src>
        static Tgt * poly_downcast ( Src * x )
        {
            assert( dynamic_cast<Tgt>(x) == x );  // logic error ?
            return static_cast<Tgt>(x);
        }

    public: // services

        void register_service_creator( const std::string &name,
                                       service_getter_type func );

        void unregister_service_creator( const std::string &name );

        void quit( );

    public: // subsystems

        template <typename T>
        void add_subsystem( )
        {
            subsystem_sptr subsys ( T::create( this ) );
            add_subsystem( typeid( T ), subsys );
        }

#if VTRC_DISABLE_CXX11

        template <typename T, typename P1>
        void add_subsystem( const P1 &p1 )
        {
            subsystem_sptr subsys ( T::create( this, p1 ) );
            add_subsystem( typeid( T ), subsys );
        }

        template <typename T, typename P1, typename P2>
        void add_subsystem( const P1 &p1, const P2 &p2 )
        {
            subsystem_sptr subsys ( T::create( this, p1, p2 ) );
            add_subsystem( typeid( T ), subsys );
        }

        template <typename T, typename P1, typename P2, typename P3>
        void add_subsystem( const P1 &p1, const P2 &p2, const P3 &p3 )
        {
            subsystem_sptr subsys ( T::create( this, p1, p2, p3 ) );
            add_subsystem( typeid( T ), subsys );
        }

#else
        template <typename T, typename ... Args >
        void add_subsystem( Args & ... pars )
        {
            subsystem_sptr subsys ( T::create( this, pars ... ) );
            add_subsystem( typeid( T ), subsys );
        }

#endif
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

        void start_all( );
        void stop_all( );

    private:

        void add_subsystem( const std::type_info &info, subsystem_sptr inst );
        subsystem_iface *subsystem( const std::type_info &info );
        const subsystem_iface *subsystem( const std::type_info &info ) const;

        void configure_session( vtrc::common::connection_iface *connection,
                                vtrc::rpc::session_options &opts );

        service_wrapper_sptr get_service_by_name(
                                      vtrc::common::connection_iface* c,
                                      const std::string &service_name );

        std::string get_session_key( vtrc::common::connection_iface* c,
                                     const std::string &id);

    };

}}


#endif // FR_APPLICATION_H
