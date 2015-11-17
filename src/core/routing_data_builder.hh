/**
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

#ifndef TEMPUS_DATA_BUILDER_HH
#define TEMPUS_DATA_BUILDER_HH

#include "routing_data.hh"
#include "variant.hh"
#include "progression.hh"
#include "db.hh"

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

///
/// Method to load metadata from the db
void load_metadata( RoutingData& rd, Db::Connection& conn );

///
/// Method to load transport modes from the db
RoutingData::TransportModes load_transport_modes( Db::Connection& conn );

} // namespace Tempus

#define REGISTER_BUILDER( ClassName ) \
    static bool ClassName ## _register() \
    { \
    std::unique_ptr<RoutingDataBuilder> builder( new ClassName() ); \
    RoutingDataBuilderRegistry::instance().addBuilder( std::move(builder) ); \
    return true; \
    }\
    bool ClassName ## _init_ = ClassName ## _register();


#endif
