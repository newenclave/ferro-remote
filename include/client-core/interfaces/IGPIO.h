#ifndef FR_IGPIO_H
#define FR_IGPIO_H

#include "vtrc-function.h"
#include "vtrc-stdint.h"

namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace gpio {

    enum direction_type {
         DIRECT_IN   = 0
        ,DIRECT_OUT  = 1
        ,DIRECT_NONE = 0xFF
    };

    static
    inline direction_type direction_val2enum( unsigned val )
    {
        switch (val) {
        case DIRECT_IN:
        case DIRECT_OUT:
            return static_cast<direction_type>( val );
        }
        return DIRECT_OUT;
    }

    enum edge_type {
         EDGE_NONE     = 0
        ,EDGE_RISING   = 1
        ,EDGE_FALLING  = 2
        ,EDGE_BOTH     = 3
    };

    static
    inline edge_type edge_val2enum( unsigned val )
    {
        switch (val) {
        case EDGE_NONE:
        case EDGE_FALLING:
        case EDGE_RISING:
        case EDGE_BOTH:
            return static_cast<edge_type>(val);
        }
        return gpio::EDGE_NONE;
    }

    ///
    /// error, new_state
    ///
    typedef vtrc::function<
            void (unsigned, unsigned)
    > value_change_callback;

    ///
    /// error, new_state, vtrc::interval
    ///
    typedef vtrc::function<
            void (unsigned, unsigned, uint64_t)
    > value_change_interval_callback;

    struct info {
        unsigned        id;
        unsigned        value;
        unsigned        active_low;
        direction_type  direction;
        edge_type       edge;
    };

    struct iface {
        virtual ~iface( ) { }

        virtual info get_info( ) const = 0;

        virtual unsigned id( ) const = 0;

        virtual unsigned value( ) const = 0;
        virtual void  set_value( unsigned value ) const = 0;

        /// - sets value to 'set_value',
        /// - sleeps 'length' microseconds
        /// - sets value to 'reset_value'
        virtual void  make_pulse( vtrc::uint64_t length,
                                  unsigned set_value = 1,
                                  unsigned reset_value = 0 ) const = 0;

        virtual unsigned active_low( ) const = 0;
        virtual void  set_active_low( unsigned value ) const = 0;

        virtual void export_device( ) const = 0;
        virtual void unexport_device( ) const = 0;

        virtual direction_type direction( ) const = 0;
        virtual void set_direction( direction_type value ) const = 0;

        virtual bool edge_supported( ) const = 0;
        virtual edge_type edge( ) const = 0;
        virtual void set_edge( edge_type value ) const = 0;

        /// to do fix
        virtual void register_for_change( value_change_callback  cb ) const = 0;
        virtual void register_for_change_int(
                                  value_change_interval_callback cb ) const = 0;
        virtual void unregister( ) const = 0;

    };

    typedef iface* iface_ptr;

    iface_ptr create( core::client_core &cl, unsigned gpio_id );
    iface_ptr create_output( core::client_core &cl, unsigned gpio_id,
                             unsigned value = 0 );
    iface_ptr create_input( core::client_core &cl, unsigned gpio_id );

    bool available( core::client_core &cl );

}}}}

#endif // IGPIO_H
