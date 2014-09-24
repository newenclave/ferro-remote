
#include "client-core/interfaces/IFile.h"
#include "protocol/fs.pb.h"

#include "client-core/fr-client.h"

#include "vtrc-common/vtrc-stub-wrapper.h"
#include "vtrc-common/vtrc-rpc-channel.h"

namespace fr { namespace client { namespace interfaces {

    namespace {

        namespace fproto = proto::fs;
        namespace vcomm  = vtrc::common;
        typedef vcomm::rpc_channel* channel_ptr;

        typedef fproto::file::Stub              stub_type;
        typedef vcomm::stub_wrapper<stub_type>  client_type;

        fproto::handle open_file( client_type &cl, const std::string &path,
                                  unsigned flags, unsigned mode )
        {
            fproto::file_open_req req;
            fproto::handle        res;
            req.set_path( path );
            req.set_flags( flags );
            req.set_mode( mode );
            cl.call( &stub_type::open, &req, &res );

            return res;
        }

        struct file_impl: public file::iface {

            mutable client_type    client_;
            fproto::handle         hdl_;

            file_impl( core::client_core &ccore,
                       const std::string &path,
                       unsigned flags, unsigned mode )
                :client_(ccore.create_channel( ), true)
                ,hdl_(open_file(client_, path, flags, mode))
            { }

            int64_t seek( int64_t pos, file::seek_whence whence ) const override
            {
                fproto::file_set_position   req;
                fproto::file_position       res;
                req.mutable_hdl( )->set_value( hdl_.value( ) );
                req.set_position( pos );
                req.set_whence( whence );
                client_.call( &stub_type::seek, &req, &res );

                return res.position( );
            }

            int64_t tell( ) const override
            {
                fproto::file_position res;
                client_.call( &stub_type::tell, &hdl_, &res );
                return res.position( );
            }

            void flush( ) const override
            {
                client_.call_request( &stub_type::flush, &hdl_ );
            }

            size_t read( void *data, size_t length ) const override
            {
                if( 0 == length ) {
                    return 0;
                }
                fproto::file_data_block req_res;

                req_res.mutable_hdl( )->set_value( hdl_.value( ) );
                req_res.set_length( length );

                client_.call( &stub_type::read, &req_res, &req_res );

                if( req_res.data( ).size( ) == 0 ) {
                    return 0;
                }

                if( length > req_res.data( ).size( ) ) {
                    length = req_res.data( ).size( );
                }

                memcpy( data, &req_res.data( )[0], length );

                return length;
            }

            size_t write( const void *data, size_t length ) const  override
            {
                if( 0 == length ) {
                    return 0;
                }
                if( length > 44000 ) {
                    length = 44000;
                }
                fproto::file_data_block req_res;
                req_res.mutable_hdl( )->set_value( hdl_.value( ) );
                req_res.set_data( data, length );
                client_.call( &stub_type::write, &req_res, &req_res );

                return req_res.length( );
            }

        };
    }


    namespace file {
        iface_ptr create( core::client_core &cl,
                          const std::string &path, unsigned  flags )
        {
            return new file_impl( cl, path, flags, 0 );
        }

        iface_ptr create( core::client_core &cl,
                          const std::string &path,
                          unsigned flags, unsigned mode )
        {
            return new file_impl( cl, path, flags, mode );
        }
    }

}}}
