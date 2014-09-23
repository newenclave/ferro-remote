#ifndef FR_IGPIO_H
#define FR_IGPIO_H

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace gpio {

    enum direction_type {
         DIRECT_IN   = 0
        ,DIRECT_OUT  = 1
    };

    enum edge_type {
         EDGE_NONE     = 0
        ,EDGE_RISING   = 1
        ,EDGE_FALLING  = 2
        ,EDGE_BOTH     = 3
    };

    struct iface {
        virtual ~iface( ) { }

        virtual unsigned id( ) const = 0;

        virtual unsigned value( ) const = 0;
        virtual void  set_value( unsigned value ) const = 0;

        virtual void export_device( ) const = 0;
        virtual void unexport_device( ) const = 0;

        virtual direction_type direction( ) const = 0;
        virtual void set_direction( direction_type value ) const = 0;

        virtual edge_type edge( ) const = 0;
        virtual void set_edge( edge_type value ) const = 0;

        /// to do fix
        virtual void register_for_change( ) const = 0;

    };

    typedef iface* iface_ptr;

    iface_ptr create( core::client_core &cl, unsigned gpio_id );
    iface_ptr create_output( core::client_core &cl, unsigned gpio_id,
                             unsigned value = 0 );
    iface_ptr create_input( core::client_core &cl, unsigned gpio_id );

}}}}

#endif // IGPIO_H
