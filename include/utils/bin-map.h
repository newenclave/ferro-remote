#ifndef FR_BIN_MAP_H
#define FR_BIN_MAP_H

#include <vector>
#include <iterator>
#include "bin-search.h"

namespace fr { namespace utils {

    template <typename KeyT, typename ValueT>
    class bin_map {

    public:

        typedef KeyT                                     first_type;
        typedef ValueT                                   second_type;
        typedef std::pair<const first_type, second_type> value_type;

    private:

        typedef std::vector<value_type *> container_type;
        typedef typename container_type::const_iterator data_const_iterator;
        typedef typename container_type::iterator       data_iterator;

        container_type data_;

        struct key_iterator_access {
            typedef typename container_type::iterator iterator;
            typedef typename container_type::const_iterator const_iterator;

            first_type operator ( )( iterator &i ) const
            {
                return (*i)->first;
            }

            first_type operator ( )( const_iterator const &i ) const
            {
                return (*i)->first;
            }
        };

        data_iterator search( const first_type &key )
        {
            return fr::utils::bin_search( data_.begin( ), data_.end( ),
                                          key, key_iterator_access( ) );
        }

        data_const_iterator search( const first_type &k ) const
        {
            data_const_iterator b(data_.begin( ));
            data_const_iterator e(data_.end( ));

            return fr::utils::bin_search( b, e, k, key_iterator_access( ) );
        }

        data_iterator upper_bound( const first_type &key )
        {
            return fr::utils::bin_upper_bound( data_.begin( ), data_.end( ),
                                               key, key_iterator_access( ) );
        }

        data_const_iterator lower_bound( const first_type &key )
        {
            return fr::utils::bin_lower_bound( data_.begin( ), data_.end( ),
                                               key, key_iterator_access( ) );
        }

        template <typename ParentIter>
        class iter_impl:
              public std::iterator<std::random_access_iterator_tag, value_type>
        {
            friend class bin_map;
            typedef ParentIter iter;
            iter cont_;

            iter_impl( iter cont )
                :cont_(cont)
            { }

            template <typename OtherType>
            iter_impl( OtherType cont )
                :cont_(cont)
            { }

        public:

            iter_impl( const iter_impl &other )
                :cont_(other.cont_)
            { }


            template <typename OtherType>
            iter_impl( const iter_impl<OtherType> &other )
                :cont_(other.cont_)
            { }

            iter_impl( )
            { }

            const iter_impl& operator = (const iter_impl& other)
            {
                cont_ = other.cont_;
                return other;
            }

            iter_impl& operator = (iter_impl& other)
            {
                cont_ = other.cont_;
                return other;
            }

            template <typename OtherT>
            iter_impl& operator = (const iter_impl<OtherT>& other)
            {
                cont_ = other.cont_;
                return other;
            }

            iter_impl& operator ++ ( ) // prefix++
            {
                cont_++;
                return *this;
            }

            iter_impl  operator ++ (int) // postfix++
            {
                iter_impl tmp(*this);
                ++(*this);
                return tmp;
            }

            iter_impl& operator -- ( ) // prefix--
            {
                cont_--;
                return *this;
            }

            iter_impl operator -- (int) // postfix--
            {
                iter_impl tmp(*this);
                --(*this);
                return tmp;
            }

            void operator += ( const std::size_t& n )
            {
                cont_ += n;
            }

            void operator += ( const iter_impl& other )
            {
                cont_ += other.cont_;
            }

            iter_impl operator + ( const std::size_t& n )
            {
                iter_impl tmp(*this);
                tmp += n;
                return tmp;
            }

            iter_impl operator + ( const iter_impl& other ) const
            {
                iter_impl tmp(*this);
                tmp += other;
                return tmp;
            }

            void operator -= ( const std::size_t& n )
            {
                cont_ -= n;
            }

            void operator -= ( const iter_impl& other )
            {
                cont_ -= other.cont_;
            }

            iter_impl operator - ( const std::size_t& n ) const
            {
                iter_impl tmp(*this);
                tmp -= n;
                return tmp;
            }

            std::size_t operator - ( const iter_impl& other ) const
            {
                return cont_ - other.cont_;
            }

            bool operator < ( const iter_impl& other ) const
            {
                return cont_ < other.cont_;
            }

            bool operator <= ( const iter_impl& other ) const
            {
                return cont_ <= other.p_;
            }

            bool operator >  ( const iter_impl& other ) const
            {
                return cont_ > other.cont_;
            }

            bool operator >= ( const iter_impl& other ) const
            {
                return cont_ >=  other.cont_;
            }

            bool operator == ( const iter_impl& other ) const
            {
                return  cont_ == other.cont_;
            }

            bool operator != ( const iter_impl& other ) const
            {
                return  cont_ != other.cont_;
            }

            value_type& operator[ ] ( size_t n )
            {
                return *(*(cont_ + n));
            }

            value_type& operator[ ] ( ssize_t n )
            {
                return *(*(cont_ + n));
            }

            value_type& operator * ( )
            {
                return *(*(cont_));
            }

            value_type const & operator * ( ) const
            {
                return *(*(cont_));
            }

            value_type* operator -> ( )
            {
                return  *(cont_);
            }

            value_type const * operator -> ( ) const
            {
                return  *(cont_);
            }
        };


    public:

        typedef iter_impl<data_iterator>       iterator;
        typedef iter_impl<data_const_iterator> const_iterator;

        bin_map( bin_map &o )
        {
            insert( o.begin( ), o.end( ) );
        }

        bin_map & operator = ( const bin_map &o )
        {
            bin_map tmp(o.begin( ), o.end( ));
            swap( tmp );
        }

        bin_map( )
        { }

        ~bin_map( )
        {
            typedef data_iterator iter;
            for( iter b(data_.begin( )), e(data_.end( )); b!=e; ++b ) {
                delete (*b);
            }
        }

        void insert( const value_type &value )
        {
            operator [ ](value.first) = value.second;
        }

        void swap( bin_map &other )
        {
            data_.swap( other.data_ );
        }

        void insert( const_iterator begin, const_iterator end )
        {
            while( begin != end ) insert( *begin++ );
        }

        void erase( const first_type &key )
        {
            data_iterator f( search( key ) );
            if( f != data_.end( ) ) {
                delete (*f);
                data_.erase( f );
            }

        }

        second_type &operator[ ]( const first_type &key )
        {
            data_iterator f( lower_bound( key ) );
            if( f == data_.end( ) || (*f)->first != key ) {
                value_type *new_val( new value_type(
                                     std::make_pair( key, second_type( ) ) ) );
                f = data_.insert( f, new_val );
            }
            return (*f)->second;
        }

        iterator find( const first_type &key )
        {
            data_iterator f( search( key ) );
            return iterator( f );
        }

        const_iterator find( const first_type &key ) const
        {
            typename container_type::const_iterator f( search( key ) );
            return const_iterator( f );
        }

        iterator begin( )
        {
            return iterator( data_.begin( ) );
        }

        const_iterator begin( ) const
        {
            return const_iterator( data_.begin( ) );
        }

        iterator end( )
        {
            return iterator( data_.end( ) );
        }

        const_iterator end( ) const
        {
            return const_iterator( data_.end( ) );
        }
    };


}}

#endif // BINMAP_H
