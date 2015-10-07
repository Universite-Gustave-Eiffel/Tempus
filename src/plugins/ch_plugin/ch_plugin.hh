#include "plugin.hh"


namespace Tempus
{

class CHPlugin : public Plugin {
public:

    static const OptionDescriptionList option_descriptions();
    static const PluginCapabilities plugin_capabilities();

    CHPlugin( const std::string& nname, const std::string& db_options ) : Plugin( nname, db_options ) {
    }

    virtual ~CHPlugin() {
    }

    static void post_build();
    virtual void pre_process( Request& request );
    virtual void process();

private:
    static void* spriv_;
};


} // namespace Tempus
