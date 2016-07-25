#ifndef TERANYINA_RESULT_HPP
#define TERANYINA_RESULT_HPP

#include <system_error>
#include <utility>
#include <memory>

#include "noexcept.hpp"

namespace fr {

    namespace detail {

        template <typename T>
        struct value_trait {
            typedef T value_type;

            static
            void copy( value_type &v, const value_type &from )
            {
                v = from;
            }

            static
            value_type create(  )
            {
                return std::move(value_type( ));
            }

            template <typename ...Args>
            static
            value_type create( Args&&...args )
            {
                return std::move(value_type(std::forward<Args>( args )... ));
            }

            static
            void destroy( value_type & )
            { }

            static
            T &value( value_type &v )
            {
                return v;
            }

            static
            const T &value( value_type const &v )
            {
                return v;
            }

        };

        template <typename T>
        struct shared_ptr_trait {

            typedef std::shared_ptr<T> value_type;
            static
            void copy( value_type &v, const value_type &from )
            {
                v = from;
            }

            static
            value_type create(  )
            {
                return std::make_shared<T>( );
            }

            template <typename ...Args>
            static
            value_type create( Args&&...args )
            {
                return std::make_shared<T>( std::forward<Args>( args )... );
            }

            static
            void destroy( value_type & )
            { }

            static
            T &value( value_type &v )
            {
                return *v;
            }

            static
            const T &value( value_type const &v )
            {
                return *v;
            }

        };

    }

    template <typename T, typename E,
              typename Trait = detail::shared_ptr_trait<T> >
    class result {

        typename Trait::value_type value_;
        std::shared_ptr<E const> error_;

    public:

        typedef T value_type;

        result( )
            :value_(Trait::create( ))
        { }

        result( const result &other )
            :value_(Trait::create( ))
        {
            Trait::copy( value_, other.value_ );
        }

        result( result &&other )
            :value_(std::move(other.value_))
            ,error_(std::move(other.error_))
        { }

        template <typename ...Args>
        result( Args&& ...args )
            :value_(Trait::create(std::forward<Args>(args)...))
        { }

        result &operator = ( const result &other )
        {
            error_ = other.error_;
            Trait::copy( value_, other.value_ );
            return *this;
        }

        result &operator = ( result &&other )
        {
            error_ = std::move(other.error_);
            value_ = std::move(other.value_);
            return *this;
        }

        ~result( )
        {
            Trait::destroy( value_ );
        }

        template <typename ...Args>
        static
        result ok( Args&& ...args )
        {
            return result( std::forward<Args>(args)... );
        }

        static
        result ok( )
        {
            return result( );
        }

        template <typename ...Args>
        static
        result fail( Args&& ...args )
        {
            result res;
            res.error_ = std::make_shared<E>( std::forward<Args>(args)... );
            return std::move( res );
        }

        static
        result fail( )
        {
            result res;
            res.error_ = std::make_shared<E>( );
            return std::move( res );
        }

        void swap( result &other )
        {
            std::swap( value_, other.value_ );
            std::swap( error_, other.error_ );
        }

        T &operator *( )
        {
            return Trait::value(value_);
        }

        const T &operator *( ) const
        {
            return Trait::value(value_);
        }

        T *operator -> ( )
        {
            return &Trait::value(value_);
        }

        const T *operator -> ( ) const
        {
            return &Trait::value(value_);
        }

        operator bool ( ) const
        {
            return error_.get( ) == nullptr;
        }

//        E *error( )
//        {
//            return error_.get( );
//        }

        const E *error( ) const
        {
            return error_.get( );
        }

    };

    template <typename T, typename E>
    std::ostream & operator << ( std::ostream &o, const result<T, E> &res )
    {
        if( res ) {
            o << "Ok: "   << *res;
        } else {
            o << "Fail: " << *res.error( );
        }
        return o;
    }

}

#endif // RESULT_HPP
