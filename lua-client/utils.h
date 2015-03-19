#ifndef FR_LUA_UTILS_H
#define FR_LUA_UTILS_H

#include <stdlib.h>

namespace fr { namespace lua { namespace utils {

    typedef void * handle;

    template <typename T>
    handle to_handle( T value  )
    {
        return reinterpret_cast<handle>(value);
    }

    template <typename T>
    T from_handle( const handle value  )
    {
        return reinterpret_cast<T>(value);
    }

}}}

#endif // FR_LUA_UTILS_H
