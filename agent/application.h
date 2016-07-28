#ifndef FR_APPLICATION_H
#define FR_APPLICATION_H

#include "vtrc-server/vtrc-application.h"
#include "vtrc-common/vtrc-signal-declaration.h"
#include "vtrc-common/vtrc-pool-pair.h"
#include "vtrc-common/vtrc-connection-iface.h"

#include "subsystem-iface.h"

#include "vtrc-function.h"
#include "logger.h"
#include "utils.h"

#include <assert.h>

#define FR_RTTI_DISABLE

#ifdef FR_RTTI_DISABLE
#   define FR_TYPE_ID( T ) fr::utilities::type_uid<T>::uid( )
#else
#   include "vtrc-common/vtrc-rtti-wrapper.h"
#   define FR_TYPE_ID( T ) typeid( T )
#endif

namespace fr { namespace agent {

    class application: public vtrc::server::application {

        struct         impl;
        impl          *impl_;
        friend struct  impl;

    public:

        class service_wrapper_impl: public vtrc::common::rpc_service_wrapper {

            application *app_;
            vtrc::common::connection_iface_wptr client_;

            typedef vtrc::common::rpc_service_wrapper super_type;

        public:

            typedef super_type::service_type service_type;
            typedef super_type::service_ptr  service_ptr;
            typedef super_type::service_sptr service_sptr;

            typedef super_type::method_type  method_type;

            service_wrapper_impl( application *app,
                                  vtrc::common::connection_iface_wptr c,
                                  service_sptr serv );

            ~service_wrapper_impl( );

        protected:

            const method_type *get_method ( const std::string &name ) const;
            application *get_application( );
            const application *get_application( ) const;
        };

        typedef vtrc::common::rpc_service_wrapper     parent_service_type;
        typedef vtrc::shared_ptr<parent_service_type> parent_service_sptr;

        typedef service_wrapper_impl service_wrapper;
        typedef vtrc::shared_ptr<service_wrapper> service_wrapper_sptr;

        ///
        /// func( app, connection )
        ///
        typedef vtrc::function<
            service_wrapper_sptr ( fr::agent::application *,
                                   vtrc::common::connection_iface_wptr )
        > service_getter_type;

        service_wrapper_sptr wrap_service (
                                    vtrc::common::connection_iface_wptr c,
                                    service_wrapper_impl::service_sptr serv );

    public:

        application( vtrc::common::pool_pair &pp,
                     const std::map<std::string, std::string> &keys );

        ~application( );

    private:

        template <class Tgt, class Src>
        static Tgt poly_downcast ( Src * x )
        {
            assert( dynamic_cast<Tgt>(x) == x );  // logic error ?
            return static_cast<Tgt>(x);
        }

    public: // services

        void register_service_factory( const std::string &name,
                                       service_getter_type func );

        void unregister_service_factory( const std::string &name );

        void quit( );

    public: // subsystems

        template <typename T>
        void add_subsystem( )
        {
            subsystem_sptr subsys ( T::create( this ) );
            add_subsystem( FR_TYPE_ID( T ), subsys );
        }

        template <typename T, typename ... Args >
        void add_subsystem( const Args & ... pars )
        {
            subsystem_sptr subsys ( T::create( this, pars ... ) );
            add_subsystem( FR_TYPE_ID( T ), subsys );
        }

        template <typename T>
        T &subsystem( )
        {
            subsystem_iface *subsys = subsystem( FR_TYPE_ID(T) );
            if( nullptr == subsys ) {
                throw std::runtime_error( "Invalid subsystem" );
            }
            return *(poly_downcast<T *>( subsys ));
        }

        template <typename T>
        const T &subsystem( ) const
        {
            const subsystem_iface *subsys = subsystem( FR_TYPE_ID(T) );
            if( nullptr == subsys ) {
                throw std::runtime_error( "Invalid subsystem" );
            }
            return *(poly_downcast<const T *>( subsys ));
        }

        template <typename T>
        T *subsystem_safe( )
        {
            subsystem_iface *subsys = subsystem( FR_TYPE_ID(T) );
            return poly_downcast<const T *>( subsys );
        }

        template <typename T>
        const T *subsystem_safe( ) const
        {
            const subsystem_iface *subsys = subsystem( FR_TYPE_ID(T) );
            return poly_downcast<const T *>( subsys );
        }

        void start_all( );
        void stop_all( );

        agent::logger       &get_logger( )      ;
        const agent::logger &get_logger( ) const;

//        auto ilog( ) -> decltype(get_logger( ).operator ()( ))
//        {
//            return get_logger( )(logger::level::info );
//        }

        static const std::string &thread_prefix( );

    private:
#ifdef FR_RTTI_DISABLE
        void add_subsystem( std::uintptr_t info, subsystem_sptr inst );
        /* === nothrow === */
        /*
         * return nullptr if not found
        */
        subsystem_iface       *subsystem( std::uintptr_t info );
        const subsystem_iface *subsystem( std::uintptr_t info) const;

        /* =============== */
#else
        void add_subsystem( const std::type_info &info, subsystem_sptr inst );

        /* === nothrow === */
        /*
         * returns NULL if not found
        */
        subsystem_iface *subsystem( const std::type_info &info );
        const subsystem_iface *subsystem( const std::type_info &info ) const;
        /* =============== */
#endif
        void configure_session( vtrc::common::connection_iface *connection,
                                vtrc::rpc::session_options &opts );

        parent_service_sptr get_service_by_name(
                                      vtrc::common::connection_iface* c,
                                      const std::string &service_name );

        std::string get_session_key( vtrc::common::connection_iface* c,
                                     const std::string &id );

        bool session_key_required( vtrc::common::connection_iface* c,
                                   const std::string &id );

    };

}}


#endif // FR_APPLICATION_H
