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
    COUT << "plouf1" << "\n";

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
    COUT << "plouf3" << "\n";
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
        .def("request", &PluginWrapper::request2, bp::return_value_policy<bp::manage_new_object>())
        .def("routing_data", bp::pure_virtual(&Tempus::Plugin::routing_data), bp::return_internal_reference<>())
        .add_property("name", &Tempus::Plugin::name)
    ;
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

extern void export_Graph();

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
}
