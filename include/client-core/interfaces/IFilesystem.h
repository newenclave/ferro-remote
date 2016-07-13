#ifndef FR_INTERFACE_FILESYSTEM_H
#define FR_INTERFACE_FILESYSTEM_H

#include "IBaseIface.h"

#include <string>
#include "vtrc-stdint.h"


namespace fr { namespace client {

namespace core {
    class client_core;
}

namespace interfaces { namespace filesystem {

    struct stat_data {          /*            man 2 stat            */
        vtrc::uint64_t dev;     /* ID of device containing file     */
        vtrc::uint64_t ino;     /* inode number                     */
        vtrc::uint32_t mode;    /* protection                       */
        vtrc::uint64_t nlink;   /* number of hard links             */
        vtrc::uint32_t uid;     /* user ID of owner                 */
        vtrc::uint32_t gid;     /* group ID of owner                */
        vtrc::uint64_t rdev;    /* device ID (if special file)      */
        vtrc::uint64_t size;    /* total size, in bytes             */
        vtrc::uint32_t blksize; /* blocksize for filesystem I/O     */
        vtrc::uint64_t blocks;  /* number of 512B blocks allocated  */
        vtrc::uint64_t atime;   /* time of last access              */
        vtrc::uint64_t mtime;   /* time of last modification        */
        vtrc::uint64_t ctime;   /* time of last status change       */
    };

    struct info_data {
        bool is_exist;
        bool is_directory;
        bool is_empty;
        bool is_regular;
        bool is_symlink;
    };

    struct update_time_buf {
        vtrc::uint64_t actime;
        vtrc::uint64_t modtime;
    };

    struct iterator_value {
        std::string path;
    };

    inline bool operator == ( const iterator_value &l, const iterator_value &r )
    { return  l.path == r.path; }

    struct directory_iterator_impl: public interfaces::base {

        virtual ~directory_iterator_impl( ) { }

        virtual void next( ) = 0;

        virtual bool end( ) const = 0;

        virtual directory_iterator_impl *clone( ) const = 0;

        virtual iterator_value &get( ) = 0;
        virtual const iterator_value &get( ) const = 0;

        virtual const info_data &info( ) const = 0;

    };

    typedef std::iterator < std::input_iterator_tag,
                            iterator_value
                          > directory_iterator_traits;


    class directory_iterator: public directory_iterator_traits {

        directory_iterator_impl *impl_;

    public:

        directory_iterator( directory_iterator_impl *impl );

        typedef directory_iterator_traits::value_type value_type;

        directory_iterator( );

        directory_iterator& operator = ( directory_iterator &other );
        ~directory_iterator( );

        directory_iterator& operator++ ( );
        directory_iterator& operator++ ( int );

        bool operator == (const directory_iterator& rhs) const;
        bool operator != (const directory_iterator& rhs) const;

        const value_type& operator *( ) const;
        const value_type* operator -> ( ) const;

    };

    struct iface: public interfaces::base {

        virtual ~iface( ) { }

        virtual bool exists( const std::string &path ) const = 0;

        virtual void stat( const std::string &path, stat_data &data ) const = 0;
        virtual void info( const std::string &path, info_data &data ) const = 0;

        virtual vtrc::uint64_t file_size( const std::string &path ) const = 0;

        virtual void rename( const std::string &old_path,
                             const std::string &new_path ) const = 0;

        virtual void cd( const std::string &path ) = 0;
        virtual const std::string &pwd(  ) const = 0;

        virtual void mkdir( const std::string &path )       const = 0;
        virtual void del( const std::string &path )         const = 0;
        virtual void remove_all( const std::string &path )  const = 0;

        virtual void truncate( const std::string &path,
                               vtrc::uint64_t )  const = 0;

        virtual void update_time( const std::string &path,
                                  vtrc::uint64_t actime,
                                  vtrc::uint64_t modtime )  const = 0;

        virtual void chmod( const std::string &path,
                            vtrc::uint32_t )  const = 0;

        virtual directory_iterator_impl *begin_iterate(
                                const std::string &path ) const = 0;
        virtual directory_iterator_impl *begin_iterate( ) const = 0;
        virtual directory_iterator begin( ) const = 0;
        virtual directory_iterator end( ) const = 0;

        virtual size_t read_file( const std::string &path,
                                  void *data, size_t max ) const = 0;
        virtual size_t write_file( const std::string &path,
                                   const void *data, size_t max ) const = 0;

    };

    typedef iface* iface_ptr;
    iface_ptr create( core::client_core &cl, const std::string &path );


}}}}

#endif // FR_INTERFACE_FILESYSTEM_H
