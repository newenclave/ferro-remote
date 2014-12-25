#ifndef FR_PREFIX_TRIE_H
#define FR_PREFIX_TRIE_H

#include "bin-map.h"

namespace fr { namespace utils {

    template <typename CharType, typename InfoType>
    class prefix_iterator_shifter;

    template <typename CharType, typename InfoType>
    class prefix_trie {

    //    friend class prefix_iterator_shifter<CharType, InfoType>;

    public:

        typedef CharType char_type;
        typedef InfoType value_type;

        typedef prefix_trie &ref_type;
        typedef prefix_trie *ptr_type;

        typedef prefix_trie const &const_ref_type;
        typedef prefix_trie const *const_ptr_type;

        typedef fr::utils::bin_map<char_type, ptr_type>  prefix_list;
        typedef std::pair<char_type, ptr_type>           prefix_pair;

    private:

        bool        end_;
        value_type  info_;
        prefix_list children_;

        struct element_cleaner: public std::unary_function<prefix_pair, void> {
            void operator ( )( const prefix_pair &next ) const
            {
                delete next.second;
            }
        };

        prefix_trie( const prefix_trie& );
        prefix_trie & operator = ( const prefix_trie& );

    public:

        prefix_trie( )
            :end_(false)
            ,info_(value_type( ))
        { }

        explicit prefix_trie( const value_type &inf )
            :end_(false)
            ,info_(inf)
        { }

        virtual ~prefix_trie( ) /*throw()*/
        {
            std::for_each( children_.begin( ),
                           children_.end( ),
                           element_cleaner( ) );
        }

    public:

        const value_type &info( )  const { return info_;  }
        bool              end( )   const { return end_; }
        bool              empty( ) const { return children_.empty( ); }

        void swap( prefix_trie<char_type, value_type> &other )
        {
            children_.swap(   other.children_ );
            std::swap( info_, other.info_ );
            std::swap( end_,  other.end_ );
        }

    protected:

        const prefix_list &element_list( ) const { return children_; }

        static
        ptr_type equal_element( const prefix_list &elem_list, char_type sym )
        {
            typename prefix_list::const_iterator itr = elem_list.find( sym );
            return ( itr == elem_list.end( ) ? ptr_type( ) : itr->second );
        }

    public:

        ptr_type equal_element( char_type sym ) const
        {
            return equal_element( element_list( ), sym );
        }

        template <typename IterT>
        void push( IterT begin, const IterT &end,
                   const value_type &current_info )
        {
            ptr_type next = this;

            for( ;begin != end; ++begin ) {
                char_type sym = *begin;
                ptr_type tmp = next->equal_element(sym);
                if( NULL == tmp ) {
                    tmp = ptr_type( new prefix_trie( ) );
                    next->children_.insert( std::make_pair( sym, tmp ) );
                }
                next = tmp;
            }
            next->info_ = current_info;
            next->end_  = true;
        }

        template <typename T>
        void push( const T &input, const value_type &current_info)
        {
            push(input.begin( ), input.end( ), current_info);
        }
    };


    template <typename CharType, typename InfoType>
    class prefix_iterator_shifter: public prefix_trie<CharType, InfoType>
    {
        typedef prefix_trie<CharType, InfoType>             parent_type;
        typedef prefix_iterator_shifter<CharType, InfoType> this_type;

    public:

        typedef typename parent_type::char_type char_type;
        typedef typename parent_type::value_type value_type;

        prefix_iterator_shifter( ) { }

        explicit prefix_iterator_shifter( const value_type &inf )
            :parent_type(inf)
        { }

        virtual ~prefix_iterator_shifter( )
        { }

    public:

        using parent_type::info;

        /// here we shift iterators.
        /// get next info in "string"

        template <typename IterT>
        const value_type &next_info( IterT &b, const IterT &e ) const
        {
            return next_info(b, e, true);
        }

        template <typename IterT>
        const value_type &next_info( IterT &b, const IterT &e,
                                     bool greedy ) const
        {
            parent_type const * next = this;
            parent_type const * last = this;

            if( b == e ) {
                return info( );
            }

            IterT bb(b);
            std::advance( bb, 1 );

            for( ; b!=e; ++b ) {
                next = next->equal_element( *b );
                if( next == 0 ) break;
                if( next->end( ) ) {
                    last = next;
                    bb = b;
                    std::advance( bb, 1 );
                    if( !greedy ) {
                        break;
                    }
                }
            }
            b = bb;
            return last->info( );
        }

        template <typename IterT>
        const value_type &next_info_no_shift(IterT &b, const IterT &e) const
        {
            return next_info_no_shift(b, e, true);
        }

        template <typename IterT>
        const value_type &next_info_no_shift( IterT b, const IterT &e,
                                             bool greedy ) const
        {
            return next_info( b, e, greedy );
        }

    private:

        /// returns next info
        template <typename IterT, typename Func>
        const value_type &shift_while_true( IterT &b, const IterT &e,
                                            const value_type& inf,
                                            Func func ) const
        {
            while( b != e ) {
                IterT bb(b);
                const value_type &tmp = next_info( b, e );
                if( !func( inf, tmp ) ) {
                    b = bb;
                    return tmp;
                }
            }
            return info( );
        }

    public:

        template <typename IterT>
        const value_type &shift_while_not_equal( IterT &b, const IterT &e,
                                                 const value_type& inf ) const
        {
            return shift_while_true( b, e, inf,
                                     std::not_equal_to<value_type>( ) );
        }

        template <typename IterT>
        const value_type &shift_while_equal( IterT &b, const IterT &e,
                                             const value_type& inf ) const
        {
            return shift_while_true( b, e, inf, std::equal_to<value_type>( ) );
        }
    };


}}

#endif // PREFIXTRIE_H
