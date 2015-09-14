#ifndef FR_INDEXMAP_H
#define FR_INDEXMAP_H

#include <map>

#include "vtrc-common/vtrc-mutex-typedefs.h"
#include "vtrc-atomic.h"
#include "vtrc-common/vtrc-exception.h"

namespace fr { namespace agent {

    template <typename IdType, typename T>
    class index_map {

    public:
        typedef IdType  index_type;
        typedef T       value_type;

    private:

        using map_type = std::map<index_type, value_type>;

        map_type                 map_;
        vtrc::shared_mutex       map_lock_;
        vtrc::atomic<index_type> id_;

        index_type next_id( )
        {
            return ++id_;
        }

    public:

        index_map( )
            :id_(index_type( ))
        { }

        index_map( index_type begin )
            :id_(begin)
        { }

        index_type set( const value_type &value )
        {
            vtrc::uint32_t id = next_id( );
            vtrc::unique_shared_lock lck( map_lock_ );
            map_[id] = value;
            return id;
        }

        value_type &get( index_type id, int error = EBADF,
                         const char * mess = "Bad id value." )
        {
            vtrc::shared_lock lck( map_lock_ );
            typename map_type::iterator f( map_.find(id) );
            if( f == map_.end( ) ) {
                vtrc::common::throw_system_error( error, mess );
            }
            return f->second;
        }

        void del( index_type id )
        {
            vtrc::unique_shared_lock lck( map_lock_ );
            map_.erase( id );
        }
    };

}}

#endif // INDEXMAP_H
