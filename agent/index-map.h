#ifndef FR_INDEXMAP_H
#define FR_INDEXMAP_H

#include <map>

#include "vtrc/common/config/vtrc-mutex.h"
#include "vtrc/common/exception.h"
#include "vtrc-atomic.h"

namespace fr { namespace agent {

    template <typename IdType, typename T>
    class index_map {

    public:
        typedef IdType  index_type;
        typedef T       value_type;

    private:

        using map_type = std::map<index_type, value_type>;

        map_type                 map_;
        vtrc::mutex              map_lock_;
        vtrc::atomic<index_type> id_;

        typedef vtrc::lock_guard<vtrc::mutex> locker_type;

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
            locker_type lck( map_lock_ );
            map_[id] = value;
            return id;
        }

        value_type &get( index_type id, int error = EBADF,
                         const char * mess = "Bad id value." )
        {
            locker_type lck( map_lock_ );
            typename map_type::iterator f( map_.find(id) );
            if( f == map_.end( ) ) {
                vtrc::common::throw_system_error( error, mess );
            }
            return f->second;
        }

        void del( index_type id )
        {
            locker_type lck( map_lock_ );
            map_.erase( id );
        }
    };

}}

#endif // INDEXMAP_H
