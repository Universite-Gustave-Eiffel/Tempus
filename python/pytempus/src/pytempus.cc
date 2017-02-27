/**
 *   Copyright (C) 2012-2017 Oslandia <infos@oslandia.com>
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

#include <list>
#include <vector>

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>


#include <tempus/plugin.hh>
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
#define GET(CLASS, TYPE, name) \
    const TYPE& (CLASS::*name ## l_get)() const = &CLASS::name; \
    auto name ## _get = make_function(name ## l_get, bp::return_value_policy<bp::copy_const_reference>());

#define GET_NON_CONST(CLASS, TYPE, name) \
    TYPE (CLASS::*name ## l_get)() const = &CLASS::name; \
    auto name ## _get = make_function(name ## l_get, bp::return_value_policy<bp::return_by_value>());

#define GET_SET(CLASS, TYPE, name) \
    GET(CLASS, TYPE, name) \
    void (CLASS::*name ## _set)(const TYPE&) = &CLASS::set_ ## name;

#define GET_NON_CONST_SET(CLASS, TYPE, name) \
    GET_NON_CONST(CLASS, TYPE, name) \
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


template<typename K, typename V, typename T>
struct map_from_python {
    map_from_python() {
        bp::converter::registry::push_back(
            &convertible,
            &construct,
            bp::type_id<T>());
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
            std::cerr << "Invalid iterator" << std::endl;
            return;
        }

        void* storage = (
            (bp::converter::rvalue_from_python_storage<T>*)
            data)->storage.bytes;

        new (storage) T();
        data->convertible = storage;

        T* result = static_cast<std::map<K,V>*>(storage);

        construct_in_object(obj_ptr, result);
    }

    static void construct_in_object(PyObject* obj_ptr, T* result) {
        PyObject* keys = PyMapping_Keys(obj_ptr);
        PyObject* it = PyObject_GetIter(keys);

        PyObject* key;
        while ((key = PyIter_Next(it))) {
            PyObject* value = PyObject_GetItem(obj_ptr, key);
            if (!value) {
                throw std::runtime_error("Error: invalid value");
            }

            K k = bp::extract<K>(key);
            V v = bp::extract<V>(value);
            result->insert(std::make_pair(k, v));
        }
    }
};


template<typename K, typename V>
struct map_to_python{
    static PyObject* convert(const std::map<K, V>& v){
        bp::dict ret;
        for (auto it= v.begin(); it!=v.end(); ++it) {
            ret[it->first] = it->second;
        }
        return bp::incref(ret.ptr());
    }
};


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


/* actual binding code */
#include <tempus/plugin_factory.hh>

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

    auto
     options_fn = [options]() {
        bp::object do_not_delete_me_please (options());
        bp::incref(do_not_delete_me_please.ptr());

        // Aha, nice try. We can't use this ...
        // return bp::extract<Tempus::Plugin::Capabilities*>(do_not_delete_me_please);

        // because there's a of a subtle difference: extract<> needs a bp::class_<> declaration
        // to be able to use python->c++ converters we registered.
        // As OptionDescriptionList is a simple typedef to std::map<> + specific converters we need to
        // hack a bit to get boost|python to play nice with us...

        // So let's try to explicitely use the converter
        typedef map_from_python<std::string, Tempus::Plugin::OptionDescription, Tempus::Plugin::OptionDescriptionList> Conv;
        if (Conv::convertible(do_not_delete_me_please.ptr())) {
            Tempus::Plugin::OptionDescriptionList* result = new Tempus::Plugin::OptionDescriptionList();
            Conv::construct_in_object(do_not_delete_me_please.ptr(), result);
            return result;
        } else {
            throw std::runtime_error( "Cannot convert to C++ OptionDescriptionList" );
        }
    };

    auto
     caps_fn = [caps]() {
        bp::object do_not_delete_me_please (caps());
        bp::incref(do_not_delete_me_please.ptr());
        return bp::extract<Tempus::Plugin::Capabilities*>(do_not_delete_me_please);
    };

    auto
     name_fn = [name]() { return bp::extract<const char*>(name()); };


    plugin_factory->register_plugin_fn(create_fn, options_fn, caps_fn, name_fn);
}

void export_PluginFactory() {
    bp::class_<Tempus::PluginFactory, boost::noncopyable>("PluginFactory", bp::no_init)
        .def("plugin_list", &Tempus::PluginFactory::plugin_list)
        .def("create_plugin", &Tempus::PluginFactory::create_plugin, bp::return_value_policy<bp::reference_existing_object>())
        .def("register_plugin_fn", &register_plugin_fn)
        .def("plugin_capabilities", &Tempus::PluginFactory::plugin_capabilities)
        .def("option_descriptions", &Tempus::PluginFactory::option_descriptions)
        .def("instance", &Tempus::PluginFactory::instance, bp::return_value_policy<bp::reference_existing_object>()).staticmethod("instance")
    ;
    // .def("create_plugin_from_fn", &Tempus::PluginFactory::create_plugin_from_fn, bp::return_value_policy<bp::reference_existing_object>())
    // bp::
}

#include <tempus/progression.hh>
void export_ProgressionCallback() {
    bp::class_<Tempus::ProgressionCallback>("ProgressionCallback")
        .def("__call__", &Tempus::ProgressionCallback::operator())
    ;

    bp::class_<Tempus::TextProgression, bp::bases<Tempus::ProgressionCallback>>("TextProgression", bp::init<int>())
    ;
}

#include <tempus/variant.hh>


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
        if (
            #if PY_MAJOR_VERSION >= 3
            !PyUnicode_Check(obj_ptr) &&
            !PyLong_Check(obj_ptr) &&
            #else
            !PyBytes_Check(obj_ptr) &&
            !PyInt_Check(obj_ptr) &&
            #endif
            !PyBytes_Check(obj_ptr) &&
            !PyBool_Check(obj_ptr) &&
            !PyFloat_Check(obj_ptr)) {
            return nullptr;
        } else {
            return obj_ptr;
        }
    }

    static void construct(
        PyObject* obj_ptr,
        bp::converter::rvalue_from_python_stage1_data* data) {
        Tempus::Variant v = construct(obj_ptr);

        void* storage = (
            (bp::converter::rvalue_from_python_storage<Tempus::Variant>*)
            data)->storage.bytes;

        new (storage) Tempus::Variant(v);
        data->convertible = storage;
    }

    static Tempus::Variant construct(PyObject* obj_ptr) {
        #if PY_MAJOR_VERSION >= 3
        if (PyUnicode_Check(obj_ptr)) {
            return Tempus::Variant::from_string(std::string(PyUnicode_AsUTF8(obj_ptr)));
        #else
        if (PyBytes_Check(obj_ptr)) {
            return Tempus::Variant::from_string(std::string(PyString_AsString(obj_ptr)));
        #endif
        } else if (PyBool_Check(obj_ptr)) {
            return Tempus::Variant::from_bool(obj_ptr == Py_True);
        #if PY_MAJOR_VERSION >= 3
        } else if (PyLong_Check(obj_ptr)) {
        #else
        } else if (PyInt_Check(obj_ptr)) {
        #endif
            return Tempus::Variant::from_int(PyLong_AsLong(obj_ptr));
        } else if (PyFloat_Check(obj_ptr)) {
            return Tempus::Variant::from_float(PyFloat_AsDouble(obj_ptr));
        } else {
            throw std::runtime_error("Cannot create Variant from obj");
        }
    }
};

void export_Variant() {
    // We don't expose a Variant python class: instead we transparently handle conversions
    // from python-types to C++ variant
    Variant_from_python();
    bp::to_python_converter<Tempus::Variant, Variant_to_python>();

    map_from_python<std::string, Tempus::Variant, Tempus::VariantMap>();
    bp::to_python_converter<Tempus::VariantMap, map_to_python<std::string, Tempus::Variant>>();

}

void export_Request() {
    GET_NON_CONST_SET(Tempus::Request, Tempus::db_id_t, origin)
    GET_NON_CONST_SET(Tempus::Request, Tempus::db_id_t, destination)
    GET(Tempus::Request, Tempus::Request::StepList, steps)
    GET(Tempus::Request, std::vector<Tempus::db_id_t>, allowed_modes)
    GET(Tempus::Request, std::vector<Tempus::CostId>, optimizing_criteria)

    bp::scope request = bp::class_<Tempus::Request>("Request")
        .add_property("origin", origin_get, origin_set)
        .add_property("destination", destination_get, destination_set)
        .add_property("steps", steps_get)
        .add_property("allowed_modes", allowed_modes_get)
        .def("add_allowed_mode", &Tempus::Request::add_allowed_mode)
        .def("optimizing_criteria", &Tempus::Request::optimizing_criteria, bp::return_value_policy<bp::copy_const_reference>())
        .def("add_intermediary_step", &Tempus::Request::add_intermediary_step)
        .def("set_optimizing_criterion", static_cast<void(Tempus::Request::*)(unsigned, const Tempus::CostId&)>(&Tempus::Request::set_optimizing_criterion))
        .def("add_criterion", &Tempus::Request::add_criterion)
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

        virtual std::unique_ptr<Tempus::PluginRequest> request( const Tempus::VariantMap&) const {
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

void export_TransportMode() {
    bp::enum_<Tempus::TransportModeEngine>("TransportModeEngine")
        .value("EnginePetrol", Tempus::TransportModeEngine::EnginePetrol)
        .value("EngineGasoil", Tempus::TransportModeEngine::EngineGasoil)
        .value("EngineLPG", Tempus::TransportModeEngine::EngineLPG)
        .value("EngineElectricCar", Tempus::TransportModeEngine::EngineElectricCar)
        .value("EngineElectricCycle", Tempus::TransportModeEngine::EngineElectricCycle)
    ;

    bp::enum_<Tempus::TransportModeId>("TransportModeId")
        .value("TransportModeWalking", Tempus::TransportModeId::TransportModeWalking)
        .value("TransportModePrivateBicycle", Tempus::TransportModeId::TransportModePrivateBicycle)
        .value("TransportModePrivateCar", Tempus::TransportModeId::TransportModePrivateCar)
        .value("TransportModeTaxi", Tempus::TransportModeId::TransportModeTaxi)
    ;

    bp::enum_<Tempus::TransportModeTrafficRule>("TransportModeTrafficRule")
        .value("TrafficRulePedestrian", Tempus::TransportModeTrafficRule::TrafficRulePedestrian)
        .value("TrafficRuleBicycle", Tempus::TransportModeTrafficRule::TrafficRuleBicycle)
        .value("TrafficRuleCar", Tempus::TransportModeTrafficRule::TrafficRuleCar)
        .value("TrafficRuleTaxi", Tempus::TransportModeTrafficRule::TrafficRuleTaxi)
        .value("TrafficRuleCarPool", Tempus::TransportModeTrafficRule::TrafficRuleCarPool)
        .value("TrafficRuleTruck", Tempus::TransportModeTrafficRule::TrafficRuleTruck)
        .value("TrafficRuleCoach", Tempus::TransportModeTrafficRule::TrafficRuleCoach)
        .value("TrafficRulePublicTransport", Tempus::TransportModeTrafficRule::TrafficRulePublicTransport)
    ;

    bp::enum_<Tempus::TransportModeSpeedRule>("TransportModeSpeedRule")
        .value("SpeedRulePedestrian", Tempus::TransportModeSpeedRule::SpeedRulePedestrian)
        .value("SpeedRuleBicycle", Tempus::TransportModeSpeedRule::SpeedRuleBicycle)
        .value("SpeedRuleElectricCycle", Tempus::TransportModeSpeedRule::SpeedRuleElectricCycle)
        .value("SpeedRuleRollerSkate", Tempus::TransportModeSpeedRule::SpeedRuleRollerSkate)
        .value("SpeedRuleCar", Tempus::TransportModeSpeedRule::SpeedRuleCar)
        .value("SpeedRuleTruck", Tempus::TransportModeSpeedRule::SpeedRuleTruck)
    ;

    bp::enum_<Tempus::TransportModeTollRule>("TransportModeTollRule")
        .value("TollRuleLightVehicule", Tempus::TransportModeTollRule::TollRuleLightVehicule)
        .value("TollRuleIntermediateVehicule", Tempus::TransportModeTollRule::TollRuleIntermediateVehicule)
        .value("TollRule2Axles", Tempus::TransportModeTollRule::TollRule2Axles)
        .value("TollRule3Axles", Tempus::TransportModeTollRule::TollRule3Axles)
        .value("TollRuleMotorcycles", Tempus::TransportModeTollRule::TollRuleMotorcycles)
    ;

    bp::enum_<Tempus::TransportModeRouteType>("TransportModeRouteType")
        .value("RouteTypeTram", Tempus::TransportModeRouteType::RouteTypeTram)
        .value("RouteTypeSubway", Tempus::TransportModeRouteType::RouteTypeSubway)
        .value("RouteTypeRail", Tempus::TransportModeRouteType::RouteTypeRail)
        .value("RouteTypeBus", Tempus::TransportModeRouteType::RouteTypeBus)
        .value("RouteTypeFerry", Tempus::TransportModeRouteType::RouteTypeFerry)
        .value("RouteTypeCableCar", Tempus::TransportModeRouteType::RouteTypeCableCar)
        .value("RouteTypeSuspendedCar", Tempus::TransportModeRouteType::RouteTypeSuspendedCar)
        .value("RouteTypeFunicular", Tempus::TransportModeRouteType::RouteTypeFunicular)
    ;

    GET_SET(Tempus::TransportMode, std::string, name)
    GET_SET(Tempus::TransportMode, bool, is_public_transport)
    GET_SET(Tempus::TransportMode, Tempus::TransportModeSpeedRule, speed_rule);
    GET_SET(Tempus::TransportMode, unsigned, toll_rules);
    GET_SET(Tempus::TransportMode, Tempus::TransportModeEngine, engine_type);
    GET_SET(Tempus::TransportMode, Tempus::TransportModeRouteType, route_type);
    bp::class_<Tempus::TransportMode, bp::bases<Tempus::Base>>("TransportMode")
        .add_property("name", name_get, name_set)
        .add_property("is_public_transport", is_public_transport_get, is_public_transport_set)
        .add_property("speed_rule", speed_rule_get, speed_rule_set)
        .add_property("toll_rules", toll_rules_get, toll_rules_set)
        .add_property("engine_type", engine_type_get, engine_type_set)
        .add_property("route_type", route_type_get, route_type_set)
        .add_property("need_parking", &Tempus::TransportMode::need_parking, static_cast<void (Tempus::TransportMode::*)(const bool&)>(&Tempus::TransportMode::set_need_parking))
        .add_property("is_shared", &Tempus::TransportMode::is_shared, static_cast<void (Tempus::TransportMode::*)(const bool&)>(&Tempus::TransportMode::set_is_shared))
        .add_property("must_be_returned", &Tempus::TransportMode::must_be_returned, static_cast<void (Tempus::TransportMode::*)(const bool&)>(&Tempus::TransportMode::set_must_be_returned))
        .add_property("traffic_rules", &Tempus::TransportMode::traffic_rules, &Tempus::TransportMode::set_traffic_rules)
    ;
}

void export_RoutingData() {
    {
        GET(Tempus::MMVertex, Tempus::MMVertex::Type, type)
        GET(Tempus::MMVertex, Tempus::db_id_t, id)

        bp::scope s = bp::class_<Tempus::MMVertex>("MMVertex", bp::init<Tempus::MMVertex::Type, Tempus::db_id_t>())
            .def(bp::init<Tempus::db_id_t, Tempus::db_id_t>())
            .add_property("type", type_get)
            .add_property("id", id_get)
            .def("network_id", &Tempus::MMVertex::network_id, bp::return_value_policy<return_optional>())
        ;

        bp::enum_<Tempus::MMVertex::Type>("Type")
            .value("Road", Tempus::MMVertex::Road)
            .value("Transport", Tempus::MMVertex::Transport)
            .value("Poi", Tempus::MMVertex::Poi)
        ;
    }

    {
        GET(Tempus::MMEdge, Tempus::MMVertex, source)
        GET(Tempus::MMEdge, Tempus::MMVertex, target)
        bp::class_<Tempus::MMEdge>("MMEdge", bp::init<const Tempus::MMVertex&, const Tempus::MMVertex&>())
            .add_property("source", source_get)
            .add_property("target", target_get)
        ;
    }

    bp::class_<Tempus::RoutingData>("RoutingData", bp::init<const std::string&>())
        .def("name", &Tempus::RoutingData::name)
        .def("transport_mode", static_cast<boost::optional<Tempus::TransportMode> (Tempus::RoutingData::*)(Tempus::db_id_t) const>(&Tempus::RoutingData::transport_mode), bp::return_value_policy<return_optional>())
        .def("transport_mode", static_cast<boost::optional<Tempus::TransportMode> (Tempus::RoutingData::*)(const std::string&) const>(&Tempus::RoutingData::transport_mode), bp::return_value_policy<return_optional>())
        .def("metadata", static_cast<std::string (Tempus::RoutingData::*)(const std::string&) const>(&Tempus::RoutingData::metadata))
        .def("set_metadata", &Tempus::RoutingData::set_metadata)

    ;
}

static auto f_option_description_default_value_get(Tempus::Plugin::OptionDescription* o)
		{ return Variant_to_python::convert(o->default_value); }
		
static auto f_option_description_default_value_set(Tempus::Plugin::OptionDescription* o, bp::object a)
{ o->default_value = Variant_from_python::construct(a.ptr()); }

void export_Plugin() {
    bp::class_<Tempus::PluginRequest>("PluginRequest", bp::init<const Tempus::Plugin*, const Tempus::VariantMap&>())
        .def("process", adapt_unique_by_value(&Tempus::PluginRequest::process))
    ;


    {
        bp::scope s = bp::class_<PluginWrapper, boost::noncopyable>("Plugin", bp::init<const std::string& /*, const Tempus::VariantMap&*/>())
            .def("request", &PluginWrapper::request2, bp::return_value_policy<bp::manage_new_object>()) // for python plugin
            .def("request", adapt_unique_const(&Tempus::Plugin::request)) // for C++ plugin called from py
            .def("routing_data", bp::pure_virtual(&Tempus::Plugin::routing_data), bp::return_internal_reference<>())
            .add_property("name", &Tempus::Plugin::name)
            .def("declare_option", &Tempus::Plugin::declare_option).staticmethod("declare_option")
        ;

        GET_SET(Tempus::Plugin::Capabilities, bool, intermediate_steps)
        GET_SET(Tempus::Plugin::Capabilities, bool, depart_after)
        GET_SET(Tempus::Plugin::Capabilities, bool, arrive_before)
        GET(Tempus::Plugin::Capabilities, std::vector<Tempus::CostId>, optimization_criteria)
        bp::class_<Tempus::Plugin::Capabilities>("Capabilities")
            .add_property("intermediate_steps", intermediate_steps_get, intermediate_steps_set)
            .add_property("depart_after", depart_after_get, depart_after_set)
            .add_property("arrive_before", arrive_before_get, arrive_before_set)
            .add_property("optimization_criteria", optimization_criteria_get)
        ;
		

        bp::class_<Tempus::Plugin::OptionDescription>("OptionDescription")
            .def_readwrite("description", &Tempus::Plugin::OptionDescription::description)
            .add_property("default_value",
                // We don't want to expose Variant to python, so we instead use convertors here.
                // This allows to keep implicit conversion between VariantMap and dict.
                f_option_description_default_value_get,
				f_option_description_default_value_set)
            .def_readwrite("visible", &Tempus::Plugin::OptionDescription::visible)
        ;


    }
    map_from_python<std::string, Tempus::Plugin::OptionDescription, Tempus::Plugin::OptionDescriptionList>();
    bp::to_python_converter<Tempus::Plugin::OptionDescriptionList, map_to_python<std::string, Tempus::Plugin::OptionDescription> >();

    bp::def("load_routing_data", &Tempus::load_routing_data, bp::return_internal_reference<>());
}

static auto f_road_graph_get_item_vertex_(Tempus::Road::Graph* g, Tempus::Road::Vertex v) { return (*g)[v]; }
static auto f_road_graph_get_item_edge_(Tempus::Road::Graph* g, Tempus::Road::Edge e) { return (*g)[e]; }
static auto f_road_graph_vertex(Tempus::Road::Graph* g, Tempus::Road::Graph::vertices_size_type i) { return vertex(i, *g); }
static auto f_road_graph_num_egdes(Tempus::Road::Graph* g) { return num_edges(*g); }
static auto f_road_graph_edge_from_index(Tempus::Road::Graph* g, Tempus::Road::Graph::edges_size_type i) { return edge_from_index(i, *g); }
static auto f_road_graph_edge(Tempus::Road::Graph* g, Tempus::Road::Graph::edge_descriptor e) { return (*g)[e];}
static auto f_road_graph_source(Tempus::Road::Graph* g, Tempus::Road::Graph::edge_descriptor e) { return source(e, *g);}
static auto f_road_graph_target(Tempus::Road::Graph* g, Tempus::Road::Graph::edge_descriptor e) { return target(e, *g);}
static auto f_road_graph_num_vertices(Tempus::Road::Graph* g) { return num_vertices(*g); }

void export_RoadGraph() {
    bp::object roadModule(bp::handle<>(bp::borrowed(PyImport_AddModule("tempus.Road"))));
    bp::scope().attr("Road") = roadModule;
    bp::scope roadScope = roadModule;

    {
        bp::scope s = bp::class_<Tempus::Road::Graph>("Graph", bp::no_init)
        //                       ^
        // boost python can't wrap lambda, but can wrap func pointers...
        // (see http://stackoverflow.com/questions/18889028/a-positive-lambda-what-sorcery-is-this)
            .def("__getitem__", f_road_graph_get_item_vertex_)
            .def("__getitem__", f_road_graph_get_item_edge_)
            .def("vertex", f_road_graph_vertex)
            .def("num_edges", f_road_graph_num_egdes)
            .def("edge_from_index", f_road_graph_edge_from_index)
            .def("edge", f_road_graph_edge)

            .def("source", f_road_graph_source)
            .def("target", f_road_graph_target)
        ;


        bp::def("num_vertices", f_road_graph_num_vertices);
        // size_t (*nv)(const Tempus::Road::Graph&) = &boost::graph::num_vertices;
        // bp::def("num_vertices", nv);
    }


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

    GET_SET(Tempus::Road::Node, bool, is_bifurcation)
    GET_SET(Tempus::Road::Node, Tempus::Point3D, coordinates)
    bp::class_<Tempus::Road::Node, bp::bases<Tempus::Base>>("Node")
        .add_property("is_bifurcation", is_bifurcation_get, is_bifurcation_set)
        .add_property("coordinates", coordinates_get, coordinates_set)
    ;

    GET_SET(Tempus::Road::Section, float, length)
    GET_SET(Tempus::Road::Section, float, car_speed_limit)
    GET_SET(Tempus::Road::Section, uint16_t, traffic_rules)
    GET_SET(Tempus::Road::Section, uint16_t, parking_traffic_rules)
    GET_SET(Tempus::Road::Section, uint8_t, lane)
    GET_SET(Tempus::Road::Section, Tempus::Road::RoadType, road_type)
    bp::class_<Tempus::Road::Section, bp::bases<Tempus::Base>>("Section")
        .add_property("length", length_get, length_set)
        .add_property("car_speed_limit", car_speed_limit_get, car_speed_limit_set)
        .add_property("traffic_rules", traffic_rules_get, traffic_rules_set)
        .add_property("parking_traffic_rules", parking_traffic_rules_get, parking_traffic_rules_set)
        .add_property("lane", lane_get, lane_set)
        .add_property("road_type", road_type_get, road_type_set)
        .add_property("is_roundabout", &Tempus::Road::Section::is_roundabout, &Tempus::Road::Section::set_is_roundabout)
        .add_property("is_bridge", &Tempus::Road::Section::is_bridge, &Tempus::Road::Section::set_is_bridge)
        .add_property("is_tunnel", &Tempus::Road::Section::is_tunnel, &Tempus::Road::Section::set_is_tunnel)
        .add_property("is_ramp", &Tempus::Road::Section::is_ramp, &Tempus::Road::Section::set_is_ramp)
        .add_property("is_tollway", &Tempus::Road::Section::is_tollway, &Tempus::Road::Section::set_is_tollway)
    ;

    GET(Tempus::Road::Restriction, Tempus::Road::Restriction::EdgeSequence, road_edges)
    GET_SET(Tempus::Road::Restriction, Tempus::Road::Restriction::CostPerTransport, cost_per_transport)
    bp::class_<Tempus::Road::Restriction, bp::bases<Tempus::Base>>("Restriction", bp::no_init)
        .add_property("road_edges", road_edges_get)
        .add_property("cost_per_transport", cost_per_transport_get, cost_per_transport_set)
    ;
}

void export_Point() {
    {
        GET_SET(Tempus::Point2D, float, x)
        GET_SET(Tempus::Point2D, float, y)
        bp::class_<Tempus::Point2D>("Point2D")
            .def(bp::init<float, float>())
            .add_property("x",x_get, x_set)
            .add_property("y",y_get, y_set)
        ;
    }
    {
        GET_SET(Tempus::Point3D, float, x)
        GET_SET(Tempus::Point3D, float, y)
        GET_SET(Tempus::Point3D, float, z)
        bp::class_<Tempus::Point3D>("Point3D")
            .def(bp::init<float, float, float>())
            .add_property("x",x_get, x_set)
            .add_property("y",y_get, y_set)
            .add_property("z",z_get, z_set)
        ;
    }

    float (*dist_2d)(const Tempus::Point2D&, const Tempus::Point2D&) = &Tempus::distance;
    float (*dist_3d)(const Tempus::Point3D&, const Tempus::Point3D&) = &Tempus::distance;
    float (*dist2_2d)(const Tempus::Point2D&, const Tempus::Point2D&) = &Tempus::distance2;
    float (*dist2_3d)(const Tempus::Point3D&, const Tempus::Point3D&) = &Tempus::distance2;

    bp::def("distance", dist_2d);
    bp::def("distance", dist_3d);
    bp::def("distance2", dist2_2d);
    bp::def("distance2", dist2_3d);
}

void export_POI() {
    GET_SET(Tempus::POI, Tempus::POI::PoiType, poi_type)
    GET_SET(Tempus::POI, std::string, name)
    GET_SET(Tempus::POI, std::vector<Tempus::db_id_t>, parking_transport_modes)
    GET_SET(Tempus::POI, Tempus::Road::Edge, road_edge)
    GET_SET(Tempus::POI, Tempus::Point3D, coordinates)

    bp::scope s = bp::class_<Tempus::POI, bp::bases<Tempus::Base>>("POI")
        .add_property("poi_type", poi_type_get, poi_type_set)
        .add_property("name", name_get, name_set)
        .add_property("parking_transport_modes", parking_transport_modes_get, parking_transport_modes_set)
        .add_property("road_edge", road_edge_get, road_edge_set)
        .add_property("coordinates", coordinates_get, coordinates_set)
    ;

    bp::enum_<Tempus::POI::PoiType>("PoiType")
        .value("TypeCarPark", Tempus::POI::PoiType::TypeCarPark)
        .value("TypeSharedCarPoint", Tempus::POI::PoiType::TypeSharedCarPoint)
        .value("TypeCyclePark", Tempus::POI::PoiType::TypeCyclePark)
        .value("TypeSharedCyclePoint", Tempus::POI::PoiType::TypeSharedCyclePoint)
        .value("TypeUserPOI", Tempus::POI::PoiType::TypeUserPOI)
    ;
}

void export_Graph() {
    {
        bp::object modalModule(bp::handle<>(bp::borrowed(PyImport_AddModule("tempus.Multimodal"))));
        bp::scope().attr("Multimodal") = modalModule;
        bp::scope modalScope = modalModule;

        {
            bp::scope s = bp::class_<Tempus::Multimodal::Vertex>("Vertex")
                .def(bp::init<const Tempus::Multimodal::Graph&, Tempus::Road::Vertex, Tempus::Multimodal::Vertex::road_t>())
                .def(bp::init<const Tempus::Multimodal::Graph&, Tempus::PublicTransportGraphIndex, Tempus::PublicTransport::Vertex, Tempus::Multimodal::Vertex::pt_t>())
                .def(bp::init<const Tempus::Multimodal::Graph&, const Tempus::POIIndex, Tempus::Multimodal::Vertex::poi_t>())
                .add_property("type", &Tempus::Multimodal::Vertex::type)
                .def("is_null", &Tempus::Multimodal::Vertex::is_null)
                .add_property("road_vertex", &Tempus::Multimodal::Vertex::road_vertex)
                .add_property("pt_graph_idx", &Tempus::Multimodal::Vertex::pt_graph_idx)
                .add_property("pt_vertex", &Tempus::Multimodal::Vertex::pt_vertex)
                .add_property("poi_idx", &Tempus::Multimodal::Vertex::poi_idx)
                .add_property("hash", &Tempus::Multimodal::Vertex::hash)
                // .def("vertex", +[](Tempus::Multimodal::Graph* g, Tempus::Multimodal::Graph::vertices_size_type i) { return vertex(i, *g); })

                // .add_property("graph", &Tempus::Multimodal::Vertex::graph)
                // .add_property("road_graph", &Tempus::Multimodal::Vertex::road_graph)
            ;

            bp::enum_<Tempus::Multimodal::Vertex::VertexType>("VertexType")
                .value("Null", Tempus::Multimodal::Vertex::VertexType::Null)
                .value("Road", Tempus::Multimodal::Vertex::VertexType::Road)
                .value("PublicTransport", Tempus::Multimodal::Vertex::VertexType::PublicTransport)
                .value("Poi", Tempus::Multimodal::Vertex::VertexType::Poi)
            ;
        }

        bp::def("get_road_node", &Tempus::Multimodal::get_road_node);
        bp::def("get_pt_stop", &Tempus::Multimodal::get_pt_stop);
        bp::def("get_mm_vertex", &Tempus::Multimodal::get_mm_vertex);
        bp::def("num_vertices", &Tempus::Multimodal::num_vertices);
        bp::def("num_edges", &Tempus::Multimodal::num_edges);
        bp::def("source", &Tempus::Multimodal::source);
        bp::def("target", &Tempus::Multimodal::target);
        bp::def("vertices", &Tempus::Multimodal::vertices);
        bp::def("edges", &Tempus::Multimodal::edges);
        bp::def("out_edges", &Tempus::Multimodal::out_edges);
        bp::def("in_edges", &Tempus::Multimodal::in_edges);
        bp::def("out_degree", &Tempus::Multimodal::out_degree);
        bp::def("in_degree", &Tempus::Multimodal::in_degree);
        bp::def("degree", &Tempus::Multimodal::degree);
        bp::def("edge", &Tempus::Multimodal::edge);


        {
            GET(Tempus::Multimodal::Edge, Tempus::Multimodal::Vertex, source)
            GET(Tempus::Multimodal::Edge, Tempus::Multimodal::Vertex, target)
            GET(Tempus::Multimodal::Edge, Tempus::Road::Edge, road_edge)
            bp::scope s = bp::class_<Tempus::Multimodal::Edge>("Edge")
                .add_property("source", source_get)
                .add_property("target", target_get)
                .add_property("road_edge", road_edge_get)
                .add_property("connection_type", &Tempus::Multimodal::Edge::connection_type)
                .add_property("traffic_rules", &Tempus::Multimodal::Edge::traffic_rules)
            ;

            bp::enum_<Tempus::Multimodal::Edge::ConnectionType>("ConnectionType")
                .value("UnknownConnection", Tempus::Multimodal::Edge::ConnectionType::UnknownConnection)
                .value("Road2Road", Tempus::Multimodal::Edge::ConnectionType::Road2Road)
                .value("Road2Transport", Tempus::Multimodal::Edge::ConnectionType::Road2Transport)
                .value("Transport2Road", Tempus::Multimodal::Edge::ConnectionType::Transport2Road)
                .value("Transport2Transport", Tempus::Multimodal::Edge::ConnectionType::Transport2Transport)
                .value("Road2Poi", Tempus::Multimodal::Edge::ConnectionType::Road2Poi)
                .value("Poi2Road", Tempus::Multimodal::Edge::ConnectionType::Poi2Road)
            ;
        }


        {
            const Tempus::Road::Graph& (Tempus::Multimodal::Graph::*roadl_get)() const = &Tempus::Multimodal::Graph::road;
            auto road_get = bp::make_function(roadl_get, bp::return_value_policy<bp::copy_const_reference>());

            bp::scope s = bp::class_<Tempus::Multimodal::Graph, bp::bases<Tempus::RoutingData>, boost::noncopyable>("Graph", bp::no_init)
                .def("road", road_get)
                .def("pois", &Tempus::Multimodal::Graph::pois)
                .def("road_vertex_from_id", &Tempus::Multimodal::Graph::road_vertex_from_id, bp::return_value_policy<return_optional>())
                .def("road_edge_from_id", &Tempus::Multimodal::Graph::road_edge_from_id, bp::return_value_policy<return_optional>())
            ;
        }
    }

    #define BIND_ITERATOR(TYPE, NAME) \
        bp::class_<std::pair<TYPE, TYPE>>(NAME, bp::no_init) \
            .def("__iter__", bp::range(&std::pair<TYPE, TYPE>::first, &std::pair<TYPE,TYPE>::second));

    BIND_ITERATOR(Tempus::Multimodal::VertexIterator, "t_m_vi")
    BIND_ITERATOR(Tempus::Multimodal::OutEdgeIterator, "t_m_oei")
    BIND_ITERATOR(Tempus::Multimodal::InEdgeIterator, "t_m_iei")
    BIND_ITERATOR(Tempus::Multimodal::EdgeIterator, "t_m_ei")
}

void add_step_wrapper(Tempus::Roadmap* roadmap, Tempus::Roadmap::RoadStep obj)
{
    std::auto_ptr<Tempus::Roadmap::Step> s(obj.clone());
    roadmap->add_step(s);
}

void export_Roadmap() {
    {
        Tempus::Roadmap::StepConstIterator (Tempus::Roadmap::*lBegin)() const = &Tempus::Roadmap::begin;
        Tempus::Roadmap::StepConstIterator (Tempus::Roadmap::*lEnd)() const = &Tempus::Roadmap::end;
        bp::scope s = bp::class_<Tempus::Roadmap>("Roadmap")
            .def("__iter__", bp::range(lBegin, lEnd))
            .def("add_step", &add_step_wrapper) // Tempus::Roadmap::add_step)
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
                .def("set_cost", &Tempus::Roadmap::Step::set_cost)
            ;

            bp::enum_<Tempus::Roadmap::Step::StepType>("StepType")
                .value("RoadStep", Tempus::Roadmap::Step::RoadStep)
                .value("PublicTransportStep", Tempus::Roadmap::Step::PublicTransportStep)
                .value("RoadStepTransferStep", Tempus::Roadmap::Step::TransferStep)
            ;

        }


        {
            GET_SET(Tempus::Roadmap::RoadStep, Tempus::db_id_t, road_edge_id)
            GET_SET(Tempus::Roadmap::RoadStep, std::string, road_name)
            GET_SET(Tempus::Roadmap::RoadStep, double, distance_km)
            GET_SET(Tempus::Roadmap::RoadStep, Tempus::Roadmap::RoadStep::EndMovement, end_movement)

            bp::scope t = bp::class_<Tempus::Roadmap::RoadStep, bp::bases<Tempus::Roadmap::Step>, std::auto_ptr<Tempus::Roadmap::RoadStep>>("RoadStep")
                .add_property("road_edge_id", road_edge_id_get, road_edge_id_set)
                .add_property("road_name", road_name_get, road_name_set)
                .add_property("distance_km", distance_km_get, distance_km_set)
                .add_property("end_movement", end_movement_get, end_movement_set)
            ;

            bp::enum_<Tempus::Roadmap::RoadStep::EndMovement>("EndMovement")
                .value("GoAhead", Tempus::Roadmap::RoadStep::GoAhead)
                .value("TurnLeft", Tempus::Roadmap::RoadStep::TurnLeft)
                .value("TurnRight", Tempus::Roadmap::RoadStep::TurnRight)
                .value("UTurn", Tempus::Roadmap::RoadStep::UTurn)
                .value("RoundAboutEnter", Tempus::Roadmap::RoadStep::RoundAboutEnter)
                .value("FirstExit", Tempus::Roadmap::RoadStep::FirstExit)
                .value("SecondExit", Tempus::Roadmap::RoadStep::SecondExit)
                .value("ThirdExit", Tempus::Roadmap::RoadStep::ThirdExit)
                .value("FourthExit", Tempus::Roadmap::RoadStep::FourthExit)
                .value("FifthExit", Tempus::Roadmap::RoadStep::FifthExit)
                .value("SixthExit", Tempus::Roadmap::RoadStep::SixthExit)
                .value("YouAreArrived", Tempus::Roadmap::RoadStep::YouAreArrived)
            ;
        }

        {
            GET_SET(Tempus::Roadmap::PublicTransportStep, Tempus::db_id_t, network_id)
            GET_SET(Tempus::Roadmap::PublicTransportStep, double, wait)
            GET_SET(Tempus::Roadmap::PublicTransportStep, double, departure_time)
            GET_SET(Tempus::Roadmap::PublicTransportStep, double, arrival_time)
            GET_SET(Tempus::Roadmap::PublicTransportStep, Tempus::db_id_t, trip_id)
            GET_SET(Tempus::Roadmap::PublicTransportStep, Tempus::db_id_t, departure_stop)
            GET_SET(Tempus::Roadmap::PublicTransportStep, std::string, departure_name)
            GET_SET(Tempus::Roadmap::PublicTransportStep, Tempus::db_id_t, arrival_stop)
            GET_SET(Tempus::Roadmap::PublicTransportStep, std::string, arrival_name)
            GET_SET(Tempus::Roadmap::PublicTransportStep, std::string, route)

            bp::class_<Tempus::Roadmap::PublicTransportStep, bp::bases<Tempus::Roadmap::Step>>("PublicTransportStep")
                .add_property("network_id", network_id_get, network_id_set)
                .add_property("wait", wait_get, wait_set)
                .add_property("departure_time", departure_time_get, departure_time_set)
                .add_property("arrival_time", arrival_time_get, arrival_time_set)
                .add_property("trip_id", trip_id_get, trip_id_set)
                .add_property("departure_stop", departure_stop_get, departure_stop_set)
                .add_property("departure_name", departure_name_get, departure_name_set)
                .add_property("arrival_stop", arrival_stop_get, arrival_stop_set)
                .add_property("arrival_name", arrival_name_get, arrival_name_set)
                .add_property("route", route_get, route_set)
            ;
        }

        {
            GET_SET(Tempus::Roadmap::TransferStep, Tempus::db_id_t, final_mode)
            GET_SET(Tempus::Roadmap::TransferStep, std::string, initial_name)
            GET_SET(Tempus::Roadmap::TransferStep, std::string, final_name)
            bp::class_<Tempus::Roadmap::TransferStep, bp::bases<Tempus::Roadmap::Step, Tempus::MMEdge>>("TransferStep", bp::init<const Tempus::MMVertex&, const Tempus::MMVertex&>())
                .add_property("final_mode", final_mode_get, final_mode_set)
                .add_property("initial_name", initial_name_get, initial_name_set)
                .add_property("final_name", final_name_get, final_name_set)
            ;
        }
    }


    {
        GET(Tempus::ResultElement, Tempus::Roadmap, roadmap)
        GET(Tempus::ResultElement, Tempus::Isochrone, isochrone)

        bp::class_<Tempus::ResultElement>("ResultElement")
            .def(bp::init<const Tempus::Isochrone&>())
            .def(bp::init<const Tempus::Roadmap&>())
            .def("is_roadmap", &Tempus::ResultElement::is_roadmap)
            .def("is_isochrone", &Tempus::ResultElement::is_isochrone)
            .def("roadmap", roadmap_get)
            .def("isochrone", isochrone_get)
        ;
    }

    bp::def("get_total_costs", &Tempus::get_total_costs);

    {
        GET_SET(Tempus::IsochroneValue, float, x)
        GET_SET(Tempus::IsochroneValue, float, y)
        GET_SET(Tempus::IsochroneValue, int, mode)
        GET_SET(Tempus::IsochroneValue, float, cost)
        bp::class_<Tempus::IsochroneValue>("IsochroneValue", bp::init<float, float, int, float>())
            .add_property("x", x_get, x_set)
            .add_property("y", y_get, y_set)
            .add_property("mode", mode_get, mode_set)
            .add_property("cost", cost_get, cost_set)
        ;
    }
}

void export_Cost() {
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
        .value("FirstValue", Tempus::CostId::FirstValue)
        .value("LastValue", Tempus::CostId::LastValue)
    ;

    bp::def("cost_name", &Tempus::cost_name);
    bp::def("cost_unit", &Tempus::cost_unit);

    map_from_python<Tempus::CostId, double, Tempus::Costs>();
    bp::to_python_converter<Tempus::Costs, map_to_python<Tempus::CostId, double>>();
}

boost::optional<Tempus::Road::Edge> edge_wrapper(Tempus::Road::Vertex v1, Tempus::Road::Vertex v2, const Tempus::Road::Graph& road_graph) {
    Tempus::Road::Edge e;
    bool found = false;
    boost::tie( e, found ) = boost::edge( v1, v2, road_graph );
    if (found) {
        return e;
    } else {
        return boost::optional<Tempus::Road::Edge>();
    }
}

BOOST_PYTHON_MODULE(pytempus)
{
    bp::object package = bp::scope();
    package.attr("__path__") = "pytempus";

    #define VECTOR_SEQ_CONV(Type) custom_vector_from_seq<Type>();  bp::to_python_converter<std::vector<Type>, custom_vector_to_list<Type> >();
    #define LIST_SEQ_CONV(Type) custom_list_from_seq<Type>();  bp::to_python_converter<std::list<Type>, custom_list_to_list<Type> >();

    VECTOR_SEQ_CONV(std::string)
    VECTOR_SEQ_CONV(Tempus::Road::Edge)
    VECTOR_SEQ_CONV(Tempus::Request::Step)
    VECTOR_SEQ_CONV(Tempus::db_id_t)
    VECTOR_SEQ_CONV(Tempus::CostId)
    VECTOR_SEQ_CONV(Tempus::IsochroneValue)
    VECTOR_SEQ_CONV(Tempus::POI)
    LIST_SEQ_CONV(Tempus::ResultElement)

    bp::class_<Tempus::Base>("Base")
        .add_property("db_id", &Tempus::Base::db_id, &Tempus::Base::set_db_id)
    ;

    bp::def("edge", &edge_wrapper, bp::return_value_policy<return_optional>());

    export_PluginFactory();
    export_ProgressionCallback();
    export_Variant();
    export_Request();
    export_Plugin();
    export_TransportMode();
    export_RoutingData();
    export_Roadmap();
    export_Graph();
    export_RoadGraph();
    export_POI();
    export_Point();
    export_Cost();
}

//pytempus.obj : error LNK2001: unresolved external symbol "__declspec(dllimport) struct _object * __cdecl boost::python::detail::init_module(struct PyModuleDef &,void (__cdecl*)(void))" (__imp_?init_module@detail@python@boost@@YAPEAU_object@@AEAUPyModuleDef@@P6AXXZ@Z)
//pytempus.obj : error LNK2001: unresolved external symbol "class Tempus::RoutingData const volatile * __cdecl boost::get_pointer<class Tempus::RoutingData const volatile >(class Tempus::RoutingData const volatile *)" (??$get_pointer@$$CDVRoutingData@Tempus@@@boost@@YAPEDVRoutingData@Tempus@@PEDV12@@Z)
//pytempus.obj : error LNK2001: unresolved external symbol "class Tempus::Plugin const volatile * __cdecl boost::get_pointer<class Tempus::Plugin const volatile >(class Tempus::Plugin const volatile *)" (??$get_pointer@$$CDVPlugin@Tempus@@@boost@@YAPEDVPlugin@Tempus@@PEDV12@@Z)
//pytempus.obj : error LNK2001: unresolved external symbol "class Tempus::PluginRequest const volatile * __cdecl boost::get_pointer<class Tempus::PluginRequest const volatile >(class Tempus::PluginRequest const volatile *)" (??$get_pointer@$$CDVPluginRequest@Tempus@@@boost@@YAPEDVPluginRequest@Tempus@@PEDV12@@Z)
//pytempus.obj : error LNK2001: unresolved external symbol "struct Tempus::Roadmap::RoadStep const volatile * __cdecl boost::get_pointer<struct Tempus::Roadmap::RoadStep const volatile >(struct Tempus::Roadmap::RoadStep const volatile *)" (??$get_pointer@$$CDURoadStep@Roadmap@Tempus@@@boost@@YAPEDURoadStep@Roadmap@Tempus@@PEDU123@@Z)

namespace boost
{
    template <>
    Tempus::RoutingData const volatile * get_pointer<class Tempus::RoutingData const volatile >(
      class Tempus::RoutingData const volatile *c)
    {
        return c;
    }
    template <>
    Tempus::Plugin const volatile * get_pointer<class Tempus::Plugin const volatile >(
      class Tempus::Plugin const volatile *c)
    {
        return c;
    }
    template <>
    Tempus::PluginRequest const volatile * get_pointer<class Tempus::PluginRequest const volatile >(
      class Tempus::PluginRequest const volatile *c)
    {
        return c;
    }
    template <>
    Tempus::Roadmap::RoadStep const volatile * get_pointer<class Tempus::Roadmap::RoadStep const volatile >(
      class Tempus::Roadmap::RoadStep const volatile *c)
    {
        return c;
    }
}
