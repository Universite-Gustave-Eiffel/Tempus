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

    virtual std::unique_ptr<RoutingData> pg_import( const std::string& pg_options, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const;
    virtual void pg_export( const RoutingData* rd, const std::string& pg_options, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const;

    virtual std::unique_ptr<RoutingData> file_import( const std::string& filename, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const;
    virtual void file_export( const RoutingData* rd, const std::string& filename, ProgressionCallback& progression, const VariantMap& options = VariantMap() ) const;

    std::string name() const { return name_; }
private:
    const std::string name_;
};

class RoutingDataBuilderRegistry
{
public:
    void addBuilder( std::unique_ptr<RoutingDataBuilder> builder );

    const RoutingDataBuilder* builder( const std::string& name );

    std::vector<std::string> builder_list() const;

    static RoutingDataBuilderRegistry& instance();

private:
    RoutingDataBuilderRegistry() {}
    std::map<std::string, std::unique_ptr<RoutingDataBuilder>> builders_;
};

} // namespace Tempus

#define REGISTER_BUILDER( ClassName ) \
    static bool register_me() \
    { \
    std::unique_ptr<RoutingDataBuilder> builder( new ClassName() ); \
    RoutingDataBuilderRegistry::instance().addBuilder( std::move(builder) ); \
    return true; \
    }\
    bool init_ = register_me();


#endif
