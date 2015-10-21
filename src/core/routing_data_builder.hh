#ifndef TEMPUS_DATA_BUILDER_HH
#define TEMPUS_DATA_BUILDER_HH

#include "routing_data.hh"
#include "variant.hh"
#include "progression.hh"

namespace Tempus
{

class RoutingDataBuilder
{
public:
    RoutingDataBuilder( const std::string& name ) : name_(name) {}
    virtual ~RoutingDataBuilder() {}
    virtual std::unique_ptr<RoutingData> pg_import( const std::string& pg_options, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) = 0;

    std::string name() const { return name_; }
private:
    const std::string name_;
};

class RoutingDataBuilderRegistry
{
public:
    void addBuilder( std::unique_ptr<RoutingDataBuilder> builder )
    {
        builders_[builder->name()] = std::move(builder);
    }

    RoutingDataBuilder* builder( const std::string& name )
    {
        auto it = builders_.find( name );
        if ( it != builders_.end() ) {
            return it->second.get();
        }
        return nullptr;
    }

    static RoutingDataBuilderRegistry& instance()
    {
        static RoutingDataBuilderRegistry b;
        return b;
    }
private:
    RoutingDataBuilderRegistry() {}
    std::map<std::string, std::unique_ptr<RoutingDataBuilder>> builders_;
};

};

#endif
