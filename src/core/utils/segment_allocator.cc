#include "segment_allocator.hh"

#include <sys/mman.h>
#include <errno.h>

#include <boost/interprocess/managed_external_buffer.hpp>
#include <boost/format.hpp>

namespace bi = boost::interprocess;

typedef bi::allocator<char, bi::managed_external_buffer::segment_manager> char_allocator_t;

namespace {

struct BufferedAlloc
{
    BufferedAlloc( void* addr, size_t size )
        : addr_((char*)addr), size_(size),
          enabled_(false),
          buffer_( bi::create_only, addr, size ), char_allocator_( buffer_.get_segment_manager() )
    {
    }

    bool contains( void* p )
    {
        return ((char*)p >= addr_) && ((char*)p <= addr_ + size_);
    }

    void* allocate( size_t n )
    {
        return char_allocator_.allocate(n).get();
    }

    void deallocate( void * const p )
    {
        char_allocator_.deallocate((char* const)p,0);
    }

    void set_enabled( bool e )
    {
        enabled_ = e;
    }

    bool enabled() const { return enabled_; }

    char* addr_;
    size_t size_;
    bool enabled_;
    bi::managed_external_buffer buffer_;
    char_allocator_t char_allocator_;
};

BufferedAlloc* _global_buffered_alloc = 0;

}

void* operator new(size_t s)
{
    if ( _global_buffered_alloc && _global_buffered_alloc->enabled() ) {
        return _global_buffered_alloc->allocate( s );
    }
    return malloc( s );
}

void operator delete( void* p ) noexcept
{
    if ( _global_buffered_alloc && _global_buffered_alloc->contains(p) ) {
        _global_buffered_alloc->deallocate(p);
    }
    else {
        free(p);
    }
}

namespace SegmentAllocator
{

void init( size_t m_size )
{
    std::cout << "Init segment of size " << m_size << std::endl;
    void* addr = mmap( 0, m_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
    if (addr == (void*)-1) {
        throw std::runtime_error( (boost::format("Can't mmap, errno: %1") % errno).str() );
    }
    _global_buffered_alloc = new BufferedAlloc( addr, m_size );
}

void release()
{
    if ( _global_buffered_alloc ) {
        munmap( _global_buffered_alloc->addr_, _global_buffered_alloc->size_ );
    }
    delete _global_buffered_alloc;
    _global_buffered_alloc = 0;
}

void enable( bool enabled )
{
    _global_buffered_alloc->set_enabled( enabled );
}

struct DumpHeader
{
    void* segment_addr;
    size_t segment_size;
    void* first_ptr;
};

void dump( const std::string& filename, void* first_ptr )
{
    if ( !_global_buffered_alloc ) {
        return;
    }
    const size_t page_size = sysconf(_SC_PAGE_SIZE);

    int fo = open( filename.c_str(), O_RDWR | O_CREAT, 0664 );
    if ( fo == -1 ) {
        throw std::runtime_error( "Cannot open dump file for writing" );
    }

    DumpHeader header;
    header.segment_addr = _global_buffered_alloc->addr_;
    header.segment_size = _global_buffered_alloc->size_;
    header.first_ptr = first_ptr;

    write( fo, &header, sizeof(header) );
    // padding to align on the size of a page
    lseek( fo, page_size, SEEK_SET );
    write( fo, header.segment_addr, header.segment_size );

    close( fo );
    std::cout << "Dumping in " << filename << " done" << std::endl;
}

void* init( const std::string& filename )
{
    int fd = open( filename.c_str(), O_RDONLY );
    if ( fd == -1 ) {
        throw std::runtime_error( "Cannot open dumpfile for reading" );
    }
    const size_t page_size = sysconf(_SC_PAGE_SIZE);

    DumpHeader header;
    read( fd, &header, sizeof(header) );
    std::cout << "Segment address: " << header.segment_addr << std::endl;
    std::cout << "Segment size: " << header.segment_size << std::endl;
    std::cout << "First ptr: " << header.first_ptr << std::endl;

    void* addr = mmap( header.segment_addr, header.segment_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
    if (addr == (void*)-1) {
        throw std::runtime_error( (boost::format("Can't mmap, errno: %1") % errno).str() );
    }
    else if ( addr != header.segment_addr ) {
        throw std::runtime_error( "Can't map to given address" );
    }

    lseek( fd, page_size, SEEK_SET );

    read( fd, addr, header.segment_size );

    close( fd );

    _global_buffered_alloc = 0;
    return header.first_ptr;
}

}
