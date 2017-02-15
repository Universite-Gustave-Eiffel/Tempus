/**
 *   Copyright (C) 2012-2015 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2015 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEMPUS_PLUGIN_FACTORY_HH
#define TEMPUS_PLUGIN_FACTORY_HH

#include <vector>
#include <string>
#include <boost/noncopyable.hpp>

#include "plugin.hh"

namespace Tempus
{
/**
 * Handles dll so that they are only loaded once and unloaded
 * at the program termination (static plugin_factory)
 */
struct PluginFactory: boost::noncopyable {
    ~PluginFactory();

    /**
     * list loaded plugins
     */
    std::vector<std::string> plugin_list() const;

    /**
     * Get descriptions of the options for a given plugin
     */
    Plugin::OptionDescriptionList option_descriptions( const std::string& dll_name ) const;

    /**
     * Get parameters (supported criteria, etc.) for a given plugin
     */
    Plugin::Capabilities plugin_capabilities( const std::string& dll_name ) const;

    /**
     * Load (and build) a plugin
     * throws std::runtime_error if dll cannot be found
     * The returned pointer must not be deleted by the caller
     */
    Plugin* create_plugin( const std::string& dll_name, ProgressionCallback& progression, const VariantMap& options = VariantMap() );

    /**
     * LReturn a preloaded plugin
     * throws std::runtime_error if dll cannot be found
     * The returned pointer must not be deleted by the caller
     */
    Plugin* plugin( const std::string& dll_name ) const;


    static PluginFactory* instance();

private:
    void load_( const std::string& dll_name );

    PluginFactory() {}

public:
    using PluginCreationFct = std::function<Plugin*(ProgressionCallback&, const VariantMap&)>;
    using PluginOptionDescriptionFct = std::function<const Plugin::OptionDescriptionList*()>;
    using PluginCapabilitiesFct = std::function<const Plugin::Capabilities*()>;
    using PluginNameFct = std::function<const char*()>;

    void register_plugin_fn(
        PluginCreationFct create_fn,
        PluginOptionDescriptionFct options_fn,
        PluginCapabilitiesFct capa_fn,
        PluginNameFct name_fn
        );

private:
    struct Dll {
        HMODULE handle;
        PluginCreationFct create_fct;
        std::unique_ptr<Plugin> plugin;
        std::unique_ptr<const Plugin::OptionDescriptionList> option_descriptions;
        std::unique_ptr<const Plugin::Capabilities> plugin_capabilities;
    };

    typedef std::map<std::string, Dll > DllMap;
    DllMap dll_;

    const Dll* test_if_loaded_( const std::string& ) const;
};
// use of volatile for shared resources is explaned here http://www.drdobbs.com/cpp/volatile-the-multithreaded-programmers-b/184403766


} // Tempus namespace

#ifdef _WIN32
/// The global instance accessor must be callable as a C function under Windows
extern "C" __declspec( dllexport ) Tempus::PluginFactory* get_plugin_factory_instance_();
#endif

///
/// Macro used inside plugins.
/// This way, the constructor will be called on library loading and the destructor on library unloading
#define DECLARE_TEMPUS_PLUGIN( name, type )                              \
    extern "C" EXPORT Tempus::Plugin* createPlugin( Tempus::ProgressionCallback& progression, const Tempus::VariantMap& options ) \
    { return new type( progression, options ); } \
    extern "C" EXPORT const char *    pluginName() { return name; } \
    extern "C" EXPORT const Tempus::Plugin::OptionDescriptionList* optionDescriptions( ) \
    { return new Tempus::Plugin::OptionDescriptionList(type::option_descriptions()); } \
    extern "C" EXPORT const Tempus::Plugin::Capabilities* pluginCapabilities( ) \
    { return new Tempus::Plugin::Capabilities(type::plugin_capabilities()); } \

#endif
