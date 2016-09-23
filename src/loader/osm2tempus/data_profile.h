#ifndef TEMPUS_DATA_PROFILE
#define TEMPUS_DATA_PROFILE

#include "osm2tempus.h"

#include <string>
#include <boost/variant.hpp>

///
/// Class used to describe additional data column and values
/// For a specific graph traversal application.
/// Each profile should derive from this base class and implement virtual methods
class DataProfile
{
public:
    ///
    /// Possible type of each column
    enum DataType
    {
        BooleanType,
        Int8Type,
        UInt8Type,
        Int16Type,
        UInt16Type,
        Int32Type,
        UInt32Type,
        Int64Type,
        UInt64Type,
        Float4Type,
        Float8Type,
        String
    };

    ///
    /// Storage of a column value
    using DataVariant = boost::variant<bool,
                                       int8_t,
                                       uint8_t,
                                       int16_t,
                                       uint16_t,
                                       int32_t,
                                       uint32_t,
                                       int64_t,
                                       uint64_t,
                                       float,
                                       double,
                                       std::string>;

    ///
    /// A column has a name and a data type
    struct Column
    {
        std::string name;
        DataType type;
    };

    ///
    /// Return the number of columns
    virtual int n_columns() const = 0;
    
    ///
    /// Return list of columns
    virtual std::vector<Column> columns() const = 0;

    ///
    /// Return additional values for a section
    virtual std::vector<DataVariant> section_additional_values( uint64_t way_id, uint64_t section_id, uint64_t node_from, uint64_t node_to, const std::vector<Point>& points, const osm_pbf::Tags& tags ) const = 0;
};

using DataProfileRegistry = std::map<std::string, DataProfile*>;

///
/// The global data profile registry
extern DataProfileRegistry g_data_profile_registry;

#define DECLARE_DATA_PROFILE(name, className) \
    static bool __initf_() { g_data_profile_registry[#name] = new className(); return true; } \
    static bool __init_ __attribute__((unused)) = __initf_()

#endif
