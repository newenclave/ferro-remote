#ifndef FR_BIN_SEARCH_H
#define FR_BIN_SEARCH_H

#include <iterator>

namespace fr { namespace utils {

    template <typename IterType, typename ValueType>
    struct default_iterator_accessor {
        ValueType &operator ( )( IterType &iter )
        {
            return *iter;
        }

        const ValueType &operator ( )( const IterType &iter )
        {
            return *iter;
        }
    };

    template <typename IterType, typename ValueType, typename AccessorT>
    const IterType bin_search( IterType begin, IterType end,
                               ValueType value, AccessorT at)
    {
        IterType e(end);
        while( begin != end ) {
            IterType m( begin );
            std::advance( m, (std::distance( begin, end ) >> 1) );
            const ValueType &val(at(m));
            if( val < value ) {
                begin = m;
                std::advance( begin, 1 );
            } else if( value < val ) {
                end = m;
            } else {
                return m;
            }
        }
        return e;
    }

    template <typename IterType, typename ValueType>
    const IterType bin_search( IterType begin, IterType end, ValueType value)
    {
        return bin_search( begin, end, value,
                           default_iterator_accessor<IterType, ValueType>( ));
    }

    template <typename IterType, typename ValueType, typename AccessorT>
    IterType bin_lower_bound( IterType begin, IterType end,
                              ValueType value, AccessorT at )
    {
        while( begin != end ) {
            IterType m( begin );
            std::advance( m, ( std::distance( begin, end ) >> 1 ) );
            const ValueType &val(at(m));
            if( val < value ) {
                begin = m;
                std::advance( begin, 1 );
            } else {
                end = m;
            }
        }
        return begin;
    }

    template <typename IterType, typename ValueType>
    IterType bin_lower_bound( IterType begin, IterType end, ValueType value)
    {
        return bin_lower_bound( begin, end, value,
                            default_iterator_accessor<IterType, ValueType>( ));
    }

    template <typename IterType, typename ValueType, typename AccessorT>
    IterType bin_upper_bound( IterType begin, IterType end,
                              ValueType value, AccessorT at )
    {
        while( begin != end ) {
            IterType m( begin );
            std::advance( m, ( std::distance( begin, end ) >> 1 ) );
            const ValueType &val(at(m));
            if( value < val  ) {
                end = m;
            } else  {
                begin = m;
                std::advance( begin, 1 );
            }
        }
        return begin;
    }

    template <typename IterType, typename ValueType>
    IterType bin_upper_bound( IterType begin, IterType end, ValueType value)
    {
        return bin_upper_bound( begin, end, value,
                            default_iterator_accessor<IterType, ValueType>( ));
    }

}}

#endif // BINSEARCH_H
