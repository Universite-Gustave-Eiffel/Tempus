#include <list>
#include <vector>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>


#include "plugin.hh"
namespace bp = boost::python;

/* Helper to register std::vector<T> <-> python list[T] */
template<typename containedType>
struct custom_vector_to_list{
    static PyObject* convert(const std::vector<containedType>& v){
        bp::list ret; BOOST_FOREACH(const containedType& e, v) ret.append(e);
        return bp::incref(ret.ptr());
    }
};
template<typename containedType>
struct custom_vector_from_seq{
    custom_vector_from_seq(){ bp::converter::registry::push_back(&convertible,&construct,bp::type_id<std::vector<containedType> >()); }
    static void* convertible(PyObject* obj_ptr){
        // the second condition is important, for some reason otherwise there were attempted conversions of Body to list which failed afterwards.
        if(!PySequence_Check(obj_ptr) || !PyObject_HasAttrString(obj_ptr,"__len__")) return 0;
        return obj_ptr;
    }
    static void construct(PyObject* obj_ptr, bp::converter::rvalue_from_python_stage1_data* data){
         void* storage=((bp::converter::rvalue_from_python_storage<std::vector<containedType> >*)(data))->storage.bytes;
         new (storage) std::vector<containedType>();
         std::vector<containedType>* v=(std::vector<containedType>*)(storage);
         int l=PySequence_Size(obj_ptr); if(l<0) abort();
         v->reserve(l); for(int i=0; i<l; i++) { v->push_back(bp::extract<containedType>(PySequence_GetItem(obj_ptr,i))); }
         data->convertible=storage;
    }
};


/* Helper to register std::list<T> <-> python list[T] */
template<typename containedType>
struct custom_list_to_list{
    static PyObject* convert(const std::list<containedType>& v){
        bp::list ret; BOOST_FOREACH(const containedType& e, v) ret.append(e);
        return bp::incref(ret.ptr());
    }
};
template<typename containedType>
struct custom_list_from_seq{
    custom_list_from_seq(){
     bp::converter::registry::push_back(&convertible,&construct,bp::type_id<std::list<containedType> >()); }
    static void* convertible(PyObject* obj_ptr){
        // the second condition is important, for some reason otherwise there were attempted conversions of Body to list which failed afterwards.
        if(!PySequence_Check(obj_ptr) || !PyObject_HasAttrString(obj_ptr,"__len__")) return 0;
        return obj_ptr;
    }
    static void construct(PyObject* obj_ptr, bp::converter::rvalue_from_python_stage1_data* data){
         void* storage=((bp::converter::rvalue_from_python_storage<std::list<containedType> >*)(data))->storage.bytes;
         new (storage) std::list<containedType>();
         std::list<containedType>* v=(std::list<containedType>*)(storage);
         int l=PySequence_Size(obj_ptr); if(l<0) abort(); for(int i=0; i<l; i++) { v->push_back(bp::extract<containedType>(PySequence_GetItem(obj_ptr,i))); }
         data->convertible=storage;
    }
};

/* Helpers to declare local variables from get/set functions */
#define GET_SET(CLASS, TYPE, name) \
    const TYPE& (CLASS::*name ## l_get)() const = &CLASS::name; \
    auto name ## _get = make_function(name ## l_get, bp::return_value_policy<bp::copy_const_reference>()); \
    void (CLASS::*name ## _set)(const TYPE&) = &CLASS::set_ ## name;

/* Helpers to bind function returning std::unique_ptr */
template <typename T,
          typename C,
          typename ...Args>
bp::object adapt_unique(std::unique_ptr<T> (C::*fn)(Args...))
{
  return bp::make_function(
      [fn](C& self, Args... args) { return (self.*fn)(args...).release(); },
      bp::return_value_policy<bp::manage_new_object>(),
      boost::mpl::vector<T*, C&, Args...>()
    );
}

template <typename T,
          typename C,
          typename ...Args>
bp::object adapt_unique_const(std::unique_ptr<T> (C::*fn)(Args...) const)
{
  return bp::make_function(
      [fn](C& self, Args... args) { return (self.*fn)(args...).release(); },
      bp::return_value_policy<bp::manage_new_object>(),
      boost::mpl::vector<T*, C&, Args...>()
    );
}

template <typename T,
          typename C,
          typename ...Args>
bp::object adapt_unique_by_value(std::unique_ptr<T> (C::*fn)(Args...))
{
  return bp::make_function(
      [fn](C& self, Args... args) {
        std::unique_ptr<T> p = (self.*fn)(args...);
        T copy(*p);
        return copy;
      },
      bp::return_value_policy<bp::return_by_value>(),
      boost::mpl::vector<T, C&, Args...>()
    );
}

/* actual binding code */
#include "plugin_factory.hh"

void register_plugin_fn(
    Tempus::PluginFactory* plugin_factory,
    bp::object create,
    bp::object options,
    bp::object caps,
    bp::object name)
{
    auto
    create_fn = [create](Tempus::ProgressionCallback& p, const Tempus::VariantMap& opt) -> Tempus::Plugin* {
        bp::object do_not_delete_me_please (create(p, opt));
        bp::incref(do_not_delete_me_please.ptr());
        return bp::extract<Tempus::Plugin*>(do_not_delete_me_please);
    };

    #if 0
    auto
     options_fn = [options]() { return bp::extract<const Tempus::Plugin::OptionDescriptionList*>(options()); };

    auto
     caps_fn = [caps]() { return bp::extract<const Tempus::Plugin::Capabilities*>(caps()); };
    #endif

    auto
     name_fn = [name]() { return bp::extract<const char*>(name()); };


    Tempus::PluginFactory::instance()->register_plugin_fn(create_fn, nullptr /*options_fn*/, nullptr /*caps_fn*/, name_fn);
}

void export_PluginFactory() {
    bp::class_<Tempus::PluginFactory, boost::noncopyable>("PluginFactory", bp::no_init)
        .def("plugin_list", &Tempus::PluginFactory::plugin_list)
        .def("create_plugin", &Tempus::PluginFactory::create_plugin, bp::return_value_policy<bp::reference_existing_object>())
        .def("register_plugin_fn", &register_plugin_fn)
        .def("instance", &Tempus::PluginFactory::instance, bp::return_value_policy<bp::reference_existing_object>()).staticmethod("instance")
    ;
    // .def("create_plugin_from_fn", &Tempus::PluginFactory::create_plugin_from_fn, bp::return_value_policy<bp::reference_existing_object>())
    // bp::
}

#include "progression.hh"
void export_ProgressionCallback() {
    bp::class_<Tempus::ProgressionCallback>("ProgressionCallback")
        .def("__call__", &Tempus::ProgressionCallback::operator())
    ;
}

#include "variant.hh"


struct Variant_to_python{
    static PyObject* convert(const Tempus::Variant& v){
        bp::object ret;
        switch (v.type()) {
            case Tempus::VariantType::BoolVariant:
                ret = boost::python::object(v.as<bool>());
                break;
            case Tempus::VariantType::IntVariant:
                ret = boost::python::object(v.as<int64_t>());
                break;
            case Tempus::VariantType::FloatVariant:
                ret = boost::python::object(v.as<double>());
                break;
            case Tempus::VariantType::StringVariant:
                ret = boost::python::object(v.as<std::string>().c_str());
                break;
            default:
                return nullptr;
        }
        return bp::incref(ret.ptr());
    }
};


struct Variant_from_python {
    Variant_from_python() {
      bp::converter::registry::push_back(
        &convertible,
        &construct,
        bp::type_id<Tempus::Variant>());
    }

    static void* convertible(PyObject* obj_ptr) {
        if (!PyString_Check(obj_ptr) ||
            !PyBool_Check(obj_ptr) ||
            !PyInt_Check(obj_ptr) ||
            !PyFloat_Check(obj_ptr)) {
            return nullptr;
        } else {
            return obj_ptr;
        }
    }

    static void construct(
        PyObject* obj_ptr,
        bp::converter::rvalue_from_python_stage1_data* data) {

        Tempus::Variant v;
        if (PyString_Check(obj_ptr)) {
            v = Tempus::Variant::from_string(std::string(PyString_AsString(obj_ptr)));
        } else if (PyBool_Check(obj_ptr)) {
            v = Tempus::Variant::from_bool(obj_ptr == Py_True);
        } else if (PyInt_Check(obj_ptr)) {
            v = Tempus::Variant::from_int(PyInt_AsLong(obj_ptr));
        } else if (PyFloat_Check(obj_ptr)) {
            v = Tempus::Variant::from_float(PyFloat_AsDouble(obj_ptr));
        } else {
            return;
        }

        void* storage = (
            (bp::converter::rvalue_from_python_storage<Tempus::Variant>*)
            data)->storage.bytes;

        new (storage) Tempus::Variant(v);
        data->convertible = storage;
    }
};



struct VariantMap_from_python {
    VariantMap_from_python() {
      bp::converter::registry::push_back(
        &convertible,
        &construct,
        bp::type_id<Tempus::VariantMap>());
    }

    static void* convertible(PyObject* obj_ptr) {
        if (!PyMapping_Check(obj_ptr) || !PyMapping_Keys(obj_ptr)) {
            return nullptr;
        } else {
            return obj_ptr;
        }
    }

    static void construct(
        PyObject* obj_ptr,
        bp::converter::rvalue_from_python_stage1_data* data) {

        PyObject* keys = PyMapping_Keys(obj_ptr);
        PyObject* it = PyObject_GetIter(keys);
        if (!it) {
                // error
            std::cerr << "Invalid iterator" << std::endl;
            return;
        }

        void* storage = (
            (bp::converter::rvalue_from_python_storage<Tempus::VariantMap>*)
            data)->storage.bytes;

        new (storage) Tempus::VariantMap();
        data->convertible = storage;

        Tempus::VariantMap* result = static_cast<Tempus::VariantMap*>(storage);
        PyObject* key;
        while ((key = PyIter_Next(it))) {
            PyObject* value = PyObject_GetItem(obj_ptr, key);
            if (!value) {
                    // error
                std::cerr << "Error: invalid value for key " << PyString_AsString(key) << std::endl;
                return;
            }

            if (PyString_Check(value)) {
                result->insert(
                    std::make_pair(
                        PyString_AsString(key),
                        Tempus::Variant::from_string(std::string(PyString_AsString(value)))));
            } else if (PyBool_Check(value)) {
                result->insert(
                    std::make_pair(
                        PyString_AsString(key),
                        Tempus::Variant::from_bool(value == Py_True)));
            } else if (PyInt_Check(value)) {
                result->insert(
                    std::make_pair(
                        PyString_AsString(key),
                        Tempus::Variant::from_int(PyInt_AsLong(value))));
            } else if (PyFloat_Check(value)) {
                result->insert(
                    std::make_pair(
                        PyString_AsString(key),
                        Tempus::Variant::from_float(PyFloat_AsDouble(value))));
            } else {
                    // error
                std::cerr << "Error: invalid type for key " << PyString_AsString(key) << std::endl;
                return;
            }
        }
    }
};

struct VariantMap_to_python{
    static PyObject* convert(const Tempus::VariantMap& v){
        bp::dict ret;
        for (auto it= v.begin(); it!=v.end(); ++it) {
            ret[it->first] = it->second;
        }
        return bp::incref(ret.ptr());
    }
};


void export_Variant() {
    Variant_from_python();
    bp::to_python_converter<Tempus::Variant, Variant_to_python>();

    VariantMap_from_python();
    bp::to_python_converter<Tempus::VariantMap, VariantMap_to_python>();
}

void export_Request() {
    GET_SET(Tempus::Request, Tempus::db_id_t, origin)
    GET_SET(Tempus::Request, Tempus::db_id_t, destination)

    bp::scope request = bp::class_<Tempus::Request>("Request")
        .add_property("origin", origin_get, origin_set)
        .add_property("destination", destination_get, destination_set)
        .def("add_allowed_mode", &Tempus::Request::add_allowed_mode)
    ;

    {
        bp::enum_<Tempus::Request::TimeConstraint::TimeConstraintType>("TimeConstraintType")
            .value("NoConstraint", Tempus::Request::TimeConstraint::NoConstraint)
            .value("ConstraintBefore", Tempus::Request::TimeConstraint::ConstraintBefore)
            .value("ConstraintAfter", Tempus::Request::TimeConstraint::ConstraintAfter)
        ;

        GET_SET(Tempus::Request::TimeConstraint, Tempus::Request::TimeConstraint::TimeConstraintType, type)
        GET_SET(Tempus::Request::TimeConstraint, Tempus::DateTime, date_time)
        bp::class_<Tempus::Request::TimeConstraint>("TimeConstraint")
            .add_property("type", type_get, type_set)
            .add_property("date_time", date_time_get, date_time_set)
        ;
    }

    {
        GET_SET(Tempus::Request::Step, Tempus::db_id_t, location)
        GET_SET(Tempus::Request::Step, Tempus::Request::TimeConstraint, constraint)
        GET_SET(Tempus::Request::Step, bool, private_vehicule_at_destination)

        bp::class_<Tempus::Request::Step>("Step")
            .add_property("location",
                location_get,
                location_set)
            .add_property("constraint",
                constraint_get,
                constraint_set)
            .add_property("private_vehicule_at_destination",
                private_vehicule_at_destination_get,
                private_vehicule_at_destination_set)
        ;

    }
}

class PluginWrapper : public Tempus::Plugin, public bp::wrapper<Tempus::Plugin> {
    public:
        PluginWrapper(const std::string& name /*, const Tempus::VariantMap& opt= Tempus::VariantMap()*/) : Tempus::Plugin(name /*, opt*/) {}

        virtual std::unique_ptr<Tempus::PluginRequest> request( const Tempus::VariantMap& options = Tempus::VariantMap() ) const {
            std::cerr << "Do not call me" << std::endl;
            return std::unique_ptr<Tempus::PluginRequest>(nullptr);
        }

        Tempus::PluginRequest* request2(const Tempus::VariantMap& options = Tempus::VariantMap() ) const {
            // see return policy
            return request(options).release();
        }

        const Tempus::RoutingData* routing_data() const {
            return this->get_override("routing_data")();
        }
};

void export_Plugin() {
    bp::class_<Tempus::PluginRequest>("PluginRequest", bp::init<const Tempus::Plugin*, const Tempus::VariantMap&>())
        .def("process", adapt_unique_by_value(&Tempus::PluginRequest::process))
    ;

    bp::class_<Tempus::RoutingData>("RoutingData", bp::no_init)
    ;

    bp::class_<PluginWrapper, boost::noncopyable>("Plugin", bp::init<const std::string& /*, const Tempus::VariantMap&*/>())
        .def("request", &PluginWrapper::request2, bp::return_value_policy<bp::manage_new_object>()) // for python plugin
        .def("request", adapt_unique_const(&Tempus::Plugin::request)) // for C++ plugin called from py
        .def("routing_data", bp::pure_virtual(&Tempus::Plugin::routing_data), bp::return_internal_reference<>())
        .add_property("name", &Tempus::Plugin::name)
    ;

    bp::def("load_routing_data", &Tempus::load_routing_data, bp::return_internal_reference<>());
}

void export_RoadGraph() {
    bp::object roadModule(bp::handle<>(bp::borrowed(PyImport_AddModule("tempus.Road"))));
    bp::scope().attr("Road") = roadModule;
    bp::scope roadScope = roadModule;

    bp::class_<Tempus::Road::Graph>("Graph", bp::no_init)
        .def("num_vertices", +[](Tempus::Road::Graph* g) { return num_vertices(*g); })
    //                       ^
    // boost python can't wrap lambda, but can wrap func pointers...
    // (see http://stackoverflow.com/questions/18889028/a-positive-lambda-what-sorcery-is-this)
        .def("vertex", +[](Tempus::Road::Graph* g, Tempus::Road::Graph::vertices_size_type i) { return vertex(i, *g); })
        .def("num_edges", +[](Tempus::Road::Graph* g) { return num_edges(*g); })
        .def("edge_from_index", +[](Tempus::Road::Graph* g, Tempus::Road::Graph::edges_size_type i) { return edge_from_index(i, *g); })
        .def("edge", +[](Tempus::Road::Graph* g, Tempus::Road::Graph::edge_descriptor e) { return (*g)[e];})

        .def("source", +[](Tempus::Road::Graph* g, Tempus::Road::Graph::edge_descriptor e) { return source(e, *g);})
        .def("target", +[](Tempus::Road::Graph* g, Tempus::Road::Graph::edge_descriptor e) { return target(e, *g);})
    ;

    bp::class_<Tempus::Road::Graph::edge_descriptor>("edge_descriptor", bp::no_init)
    ;

    bp::enum_<Tempus::Road::RoadType>("RoadType")
        .value("RoadMotorway", Tempus::Road::RoadType::RoadMotorway)
        .value("RoadPrimary", Tempus::Road::RoadType::RoadPrimary)
        .value("RoadSecondary", Tempus::Road::RoadType::RoadSecondary)
        .value("RoadStreet", Tempus::Road::RoadType::RoadStreet)
        .value("RoadOther", Tempus::Road::RoadType::RoadOther)
        .value("RoadCycleWay", Tempus::Road::RoadType::RoadCycleWay)
        .value("RoadPedestrianOnly", Tempus::Road::RoadType::RoadPedestrianOnly)
    ;

    bp::class_<Tempus::Road::Node, bp::bases<Tempus::Base>>("Node")
    ;
    bp::class_<Tempus::Road::Section, bp::bases<Tempus::Base>>("Section")
    ;
    bp::class_<Tempus::Road::Restriction, bp::bases<Tempus::Base>>("Restriction", bp::no_init)
    ;
}

void export_BoostGraph() {

}

/** http://stackoverflow.com/questions/26497922/how-to-wrap-a-c-function-that-returns-boostoptionalt */

namespace detail {

/// @brief Type trait that determines if the provided type is
///        a boost::optional.
template <typename T>
struct is_optional : boost::false_type {};

template <typename T>
struct is_optional<boost::optional<T> > : boost::true_type {};

/// @brief Type used to provide meaningful compiler errors.
template <typename>
struct return_optional_requires_a_optional_return_type {};

/// @brief ResultConverter model that converts a boost::optional object to
///        Python None if the object is empty (i.e. boost::none) or defers
///        to Boost.Python to convert object to a Python object.
template <typename T>
struct to_python_optional
{
  /// @brief Only supports converting Boost.Optional types.
  /// @note This is checked at runtime.
  bool convertible() const { return detail::is_optional<T>::value; }

  /// @brief Convert boost::optional object to Python None or a
  ///        Boost.Python object.
  PyObject* operator()(const T& obj) const
  {
    namespace python = boost::python;
    python::object result =
      obj                      // If boost::optional has a value, then
        ? python::object(*obj) // defer to Boost.Python converter.
        : python::object();    // Otherwise, return Python None.

    // The python::object contains a handle which functions as
    // smart-pointer to the underlying PyObject.  As it will go
    // out of scope, explicitly increment the PyObject's reference
    // count, as the caller expects a non-borrowed (i.e. owned) reference.
    return python::incref(result.ptr());
  }

  /// @brief Used for documentation.
  const PyTypeObject* get_pytype() const { return 0; }
};

} // namespace detail

/// @brief Converts a boost::optional to Python None if the object is
///        equal to boost::none.  Otherwise, defers to the registered
///        type converter to returs a Boost.Python object.
struct return_optional
{
  template <class T> struct apply
  {
    // The to_python_optional ResultConverter only checks if T is convertible
    // at runtime.  However, the following MPL branch cause a compile time
    // error if T is not a boost::optional by providing a type that is not a
    // ResultConverter model.
    typedef typename boost::mpl::if_<
      detail::is_optional<T>,
      detail::to_python_optional<T>,
      detail::return_optional_requires_a_optional_return_type<T>
    >::type type;
  }; // apply
};   // return_optional

void export_Graph() {
    using namespace boost::python;

    {
        object modalModule(handle<>(borrowed(PyImport_AddModule("tempus.MultiModal"))));
        scope().attr("MultiModal") = modalModule;
        scope modalScope = modalModule;

        const Tempus::Road::Graph& (Tempus::Multimodal::Graph::*roadl_get)() const = &Tempus::Multimodal::Graph::road;
        auto road_get = make_function(roadl_get, return_value_policy<copy_const_reference>());

        class_<Tempus::Multimodal::Graph, bp::bases<Tempus::RoutingData>, boost::noncopyable>("MMGraph", no_init)
            .def("road", road_get)
            .def("road_vertex_from_id", &Tempus::Multimodal::Graph::road_vertex_from_id, bp::return_value_policy<return_optional>())
        ;
    }
}

void export_Roadmap() {
    {
        Tempus::Roadmap::StepConstIterator (Tempus::Roadmap::*lBegin)() const = &Tempus::Roadmap::begin;
        Tempus::Roadmap::StepConstIterator (Tempus::Roadmap::*lEnd)() const = &Tempus::Roadmap::end;
        bp::scope s = bp::class_<Tempus::Roadmap>("Roadmap")
            .def("__iter__", bp::range(lBegin, lEnd))
        ;

        {
            GET_SET(Tempus::Roadmap::Step, Tempus::Roadmap::Step::StepType, step_type)
            GET_SET(Tempus::Roadmap::Step, Tempus::db_id_t, transport_mode)
            GET_SET(Tempus::Roadmap::Step, std::string, geometry_wkb)


            bp::scope t = bp::class_<Tempus::Roadmap::Step>("Step", bp::init<Tempus::Roadmap::Step::StepType>())
                .add_property("step_type", step_type_get, step_type_set)
                .add_property("transport_mode", transport_mode_get, transport_mode_set)
                .add_property("geometry_wkb", geometry_wkb_get, geometry_wkb_set)
                .def("cost", &Tempus::Roadmap::Step::cost)
            ;

            bp::enum_<Tempus::Roadmap::Step::StepType>("StepType")
                .value("RoadStep", Tempus::Roadmap::Step::RoadStep)
                .value("PublicTransportStep", Tempus::Roadmap::Step::PublicTransportStep)
                .value("RoadStepTransferStep", Tempus::Roadmap::Step::TransferStep)
            ;

        }
    }

    const Tempus::Roadmap& (Tempus::ResultElement::*roadmapl_get)() const = &Tempus::ResultElement::roadmap;
    auto roadmap_get = bp::make_function(roadmapl_get, bp::return_value_policy<bp::copy_const_reference>());

    bp::class_<Tempus::ResultElement>("ResultElement")
        .def("is_roadmap", &Tempus::ResultElement::is_roadmap)
        .def("is_isochrone", &Tempus::ResultElement::is_isochrone)
        .def("roadmap", roadmap_get) //&Tempus::ResultElement::roadmap, return_value_policy<copy_const_reference>())
    ;

    bp::enum_<Tempus::CostId>("CostId")
        .value("CostDistance", Tempus::CostId::CostDistance)
        .value("CostDuration", Tempus::CostId::CostDuration)
        .value("CostPrice", Tempus::CostId::CostPrice)
        .value("CostCarbon", Tempus::CostId::CostCarbon)
        .value("CostCalories", Tempus::CostId::CostCalories)
        .value("CostNumberOfChanges", Tempus::CostId::CostNumberOfChanges)
        .value("CostVariability", Tempus::CostId::CostVariability)
        .value("CostPathComplexity", Tempus::CostId::CostPathComplexity)
        .value("CostElevation", Tempus::CostId::CostElevation)
        .value("CostSecurity", Tempus::CostId::CostSecurity)
        .value("CostLandmark", Tempus::CostId::CostLandmark)
    ;
}

BOOST_PYTHON_MODULE(tempus)
{
    bp::object package = bp::scope();
    package.attr("__path__") = "tempus";

    #define VECTOR_SEQ_CONV(Type) custom_vector_from_seq<Type>();  bp::to_python_converter<std::vector<Type>, custom_vector_to_list<Type> >();
    #define LIST_SEQ_CONV(Type) custom_list_from_seq<Type>();  bp::to_python_converter<std::list<Type>, custom_list_to_list<Type> >();

    VECTOR_SEQ_CONV(std::string)
    LIST_SEQ_CONV(Tempus::ResultElement)

    bp::class_<Tempus::Base>("Base")
        .add_property("db_id", &Tempus::Base::db_id, &Tempus::Base::set_db_id)
    ;

    export_PluginFactory();
    export_ProgressionCallback();
    export_Variant();
    export_Request();
    export_Plugin();
    export_Roadmap();
    export_Graph();
    export_RoadGraph();
    export_BoostGraph();
}
