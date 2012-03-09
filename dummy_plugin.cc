#include "plugin.hh"

namespace Tempus
{
    class MyPlugin : public Plugin
    {
    public:
	MyPlugin() : Plugin("myplugin")
	{
	}

	virtual ~MyPlugin()
	{
	}
    };
};

DECLARE_TEMPUS_PLUGIN( MyPlugin );

