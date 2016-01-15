#include "plugin_factory.hh"
#ifdef _WIN32
#include <strsafe.h>
#endif

Tempus::PluginFactory* get_plugin_factory_instance_()
{
    return Tempus::PluginFactory::instance();
}

namespace Tempus {


PluginFactory* PluginFactory::instance()
{
    // On Windows, static and global variables are COPIED from the main module (EXE) to the other (DLL).
    // DLL have still access to the main EXE memory ...
    static PluginFactory* instance_ = 0;

    if ( 0 == instance_ ) {
#ifdef _WIN32
        // We test if we are in the main module (EXE) or not. If it is the case, a new Application is allocated.
        // It will also be returned by modules.
        PluginFactory * ( *main_get_instance )() = ( PluginFactory* (* )() )GetProcAddress( GetModuleHandle( NULL ), "get_plugin_factory_instance_" );
        instance_ = ( main_get_instance == &get_plugin_factory_instance_ )  ? new PluginFactory : main_get_instance();
#else
        instance_ = new PluginFactory();
#endif
    }

    return instance_;
}


#ifdef _WIN32
std::string win_error()
{
    // Retrieve the system error message for the last-error code
    struct RAII {
        LPVOID ptr;
        RAII( LPVOID p ):ptr( p ) {};
        ~RAII() {
            LocalFree( ptr );
        }
    };
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
        ( LPTSTR ) &lpMsgBuf,
        0, NULL );
    RAII lpMsgBufRAII( lpMsgBuf );

    // Display the error message and exit the process

    LPVOID lpDisplayBuf = ( LPVOID )LocalAlloc( LMEM_ZEROINIT,
                          ( lstrlen( ( LPCTSTR )lpMsgBuf ) + 40 ) * sizeof( TCHAR ) );
    RAII lpDisplayBufRAII( lpDisplayBuf );
    StringCchPrintf( ( LPTSTR )lpDisplayBuf,
                     LocalSize( lpDisplayBuf ) / sizeof( TCHAR ), TEXT( "failed with error %d: %s" ), dw, lpMsgBuf );
    return std::string( ( const char* )lpDisplayBuf );
}
#endif

PluginFactory::~PluginFactory()
{
    for ( DllMap::iterator i=dll_.begin(); i!=dll_.end(); i++ ) {
#ifdef _WIN32
        FreeLibrary( i->second.handle );
#else

        if ( dlclose( i->second.handle ) ) {
            CERR << "Error on dlclose " << dlerror() << std::endl;
        }

#endif
    }
}

void PluginFactory::load_( const std::string& dll_name )
{
    auto it = dll_.find( dll_name );
    if ( it != dll_.end() ) {
        return;    // already there
    }

    const std::string complete_dll_name = DLL_PREFIX + dll_name + DLL_SUFFIX;

    COUT << "Loading " << complete_dll_name << std::endl;

#ifdef _WIN32
#   define DLCLOSE FreeLibrary
#   define DLSYM GetProcAddress
#   define THROW_DLERROR( msg )  throw std::runtime_error( std::string( msg ) + "(" + win_error() + ")")
#else
#   define DLCLOSE dlclose
#   define DLSYM dlsym
#   define THROW_DLERROR( msg ) throw std::runtime_error( std::string( msg ) + "(" + std::string( dlerror() ) + ")" );
#endif
    struct RAII {
        RAII( HMODULE h ):h_( h ) {};
        ~RAII() {
            if ( h_ ) {
                DLCLOSE( h_ );
            }
        }
        HMODULE get() {
            return h_;
        }
        HMODULE release() {
            HMODULE p = NULL;
            std::swap( p,h_ );
            return p;
        }
    private:
        HMODULE h_;
    };

    RAII hRAII(
#ifdef _WIN32
        LoadLibrary( complete_dll_name.c_str() )
#else
        dlopen( complete_dll_name.c_str(), RTLD_NOW | RTLD_GLOBAL )
#endif
    );

    if ( !hRAII.get() ) {
        THROW_DLERROR( "cannot load " + complete_dll_name );
    }

    PluginCreationFct createFct;
    // this cryptic syntax is here to avoid a warning when converting
    // a pointer-to-function to a pointer-to-object
    *reinterpret_cast<void**>( &createFct ) = DLSYM( hRAII.get(), "createPlugin" );

    if ( !createFct ) {
        THROW_DLERROR( "no function createPlugin in " + complete_dll_name );
    }

    PluginOptionDescriptionFct option_description_fct;
    *reinterpret_cast<void**>( &option_description_fct ) = DLSYM( hRAII.get(), "optionDescriptions" );
    if ( !option_description_fct ) {
        THROW_DLERROR( "no function optionDescriptions in " + complete_dll_name );
    }

    PluginCapabilitiesFct capabilities_fct;
    *reinterpret_cast<void**>( &capabilities_fct ) = DLSYM( hRAII.get(), "pluginCapabilities" );
    if ( !capabilities_fct ) {
        THROW_DLERROR( "no function pluginCapabilities in " + complete_dll_name );
    }

    PluginNameFct name_fct;
    *reinterpret_cast<void**>( &name_fct ) = DLSYM( hRAII.get(), "pluginName" );
    if ( !name_fct ) {
        THROW_DLERROR( "no function pluginName in " + complete_dll_name );
    }

    const std::string name = ( *name_fct )();
    Dll dll;
    dll.handle = hRAII.release();
    dll.create_fct = createFct;
    dll.option_descriptions.reset( (*option_description_fct)() );
    dll.plugin_capabilities.reset( (*capabilities_fct)() );

    dll_.insert( std::make_pair( name, std::move(dll) ) );

    COUT << "loaded " << name << " from " << dll_name << "\n";
}

const PluginFactory::Dll* PluginFactory::test_if_loaded_( const std::string& dll_name ) const
{
    std::string loaded;

    for ( DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++ ) {
        loaded += " " + i->first;
    }

    DllMap::const_iterator dll = dll_.find( dll_name );

    if ( dll == dll_.end() ) {
        throw std::runtime_error( dll_name + " is not loaded (loaded:" + loaded+")" );
    }
    return &dll->second;
}

Plugin::OptionDescriptionList PluginFactory::option_descriptions( const std::string& dll_name ) const
{
    const Dll* dll = test_if_loaded_( dll_name );
    return *dll->option_descriptions;
}

Plugin::Capabilities PluginFactory::plugin_capabilities( const std::string& dll_name ) const
{
    const Dll* dll = test_if_loaded_( dll_name );
    return *dll->plugin_capabilities;
}

Plugin* PluginFactory::create_plugin( const std::string& dll_name, ProgressionCallback& progression, const VariantMap& options )
{
    auto it = dll_.find( dll_name );
    if ( it == dll_.end() ) {
        load_( dll_name );
        it = dll_.find( dll_name );
    }
    if ( !it->second.plugin ) {
        it->second.plugin.reset( (*it->second.create_fct)( progression, options ) );
    }
    return it->second.plugin.get();
}

Plugin* PluginFactory::plugin( const std::string& dll_name ) const
{
    const Dll* dll = test_if_loaded_( dll_name );
    return dll->plugin.get();
}

std::vector<std::string> PluginFactory::plugin_list() const
{
    std::vector<std::string> names;

    for ( DllMap::const_iterator i=dll_.begin(); i!=dll_.end(); i++ ) {
        names.push_back( i->first );
    }

    return names;
}

} // namespace Tempus
