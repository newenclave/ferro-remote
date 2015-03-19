#ifndef FR_LUA_UTILS_H
#define FR_LUA_UTILS_H

namespace fr { namespace lua { namespace utils {

    inline void * to_handle( size_t value  )
    {
        return reinterpret_cast<void *>(value);
    }

    inline size_t from_handle( const void * value  )
    {
        return reinterpret_cast<size_t>(value);
    }

}}}

#endif // FR_LUA_UTILS_H
