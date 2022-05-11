#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "jswrapper/SeApi.h"
#include "jswrapper/Value.h"

#include "conversions/jsb_conversions.h"

//#include "signature.h"

namespace setpl {

template <typename... ARGS>
struct TypeList;

struct ConstructorBase {
    size_t       arg_count              = 0;
    virtual bool construct(se::State &) = 0;
};

template <typename T>
struct Constructor;

template <typename... ARGS, size_t... indexes>
bool convert_js_args_to_tuple(const se::ValueArray &jsArgs, std::tuple<ARGS...> &args, se::Object *ctx, std::index_sequence<indexes...>) {
    std::array<bool, sizeof...(indexes)> all = {(sevalue_to_native(jsArgs[indexes], &std::get<indexes>(args).data, ctx), ...)};
    return true;
}

template <typename T, typename... ARGS>
struct Constructor<TypeList<T, ARGS...>> : ConstructorBase {
    bool construct(se::State &state) {
        if ((sizeof...(ARGS)) != state.args().size()) {
            return false;
        }

        if constexpr (sizeof...(ARGS) > 0) {
            std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
            auto &                                                     jsArgs = state.args();
            convert_js_args_to_tuple(jsArgs, args, nullptr, std::make_index_sequence<sizeof...(ARGS)>());
            T *self = constructWithTuple(args, std::make_index_sequence<sizeof...(ARGS)>());
            state.thisObject()->setPrivateData(self);
        } else {
            T *self = new T;
            state.thisObject()->setPrivateData(self);
        }
        return true;
    }

    template <typename... ARGS_HT, size_t... indexes>
    T *constructWithTuple(std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) {
        return new T(std::get<indexes>(args).value()...);
    }
};

struct InstanceMethodBase {
    std::string  class_name;
    std::string  method_name;
    std::string  signature;
    size_t       arg_count;
    virtual bool invoke(se::State &) const = 0;
};
template <typename T>
struct InstanceMethod;

template <typename T, typename R, typename... ARGS>
struct InstanceMethod<R (T::*)(ARGS...)> : InstanceMethodBase {
    using type                          = R (T::*)(ARGS...);
    using return_type                   = R;
    using class_type                    = T;
    constexpr static size_t argN        = sizeof...(ARGS);
    constexpr static bool   return_void = std::is_same<void, R>::value;

    type fnPtr = nullptr;

    //constexpr auto cal_signature() {
    //    return class_name + "::" + method_name + "/" + FunctionSignature<R, ARGS>::signature().data();
    //}

    //template <size_t... indexes>
    //bool convert_all(const se::ValueArray &jsArgs, std::tuple<ARGS...> &args, se::Object *ctx, std::index_sequence<indexes...>) const {
    //    std::array<bool, sizeof...(indexes)> all = {(sevalue_to_native(jsArgs[indexes], &std::get<indexes>(args), ctx), ...)};
    //    return true;
    //}

    template <typename... ARGS_HT, size_t... indexes>
    R callWithTuple(T *self, std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) const {
        return ((reinterpret_cast<T *>(self))->*fnPtr)(std::get<indexes>(args).value()...);
    }

    bool invoke(se::State &state) const override {
        constexpr auto                                             indexes{std::make_index_sequence<sizeof...(ARGS)>()};
        T *                                                        self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *                                               thisObject = state.thisObject();
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        const auto &                                               jsArgs = state.args();
        if (argN != jsArgs.size()) {
            SE_LOGE("incorret argument size %d, expect %d\n", jsArgs.size(), argN);
            return false;
        }
        convert_js_args_to_tuple(jsArgs, args, thisObject, indexes);
        if constexpr (return_void) {
            callWithTuple(self, args, indexes);
        } else {
            se::Value r;
            nativevalue_to_se(callWithTuple(self, args, indexes), r, thisObject);
        }

        return true;
    }
};

struct InstanceMethodOverloaded : InstanceMethodBase {
    std::vector<InstanceMethodBase *> methods;
    bool                              invoke(se::State &state) const override {
        auto argCount = state.args().size();
        for (auto *m : methods) {
            if (m->arg_count == argCount) {
                return m->invoke(state);
            }
        }
        return false;
    }
};

struct InstanceFieldBase {
    std::string  class_name;
    std::string  field_name;
    std::string  signature;
    std::string  name() { return class_name + "::" + field_name; }
    virtual bool get(se::State &) const = 0;
    virtual bool set(se::State &) const = 0;
};

template <typename T>
struct InstanceField;

template <typename T, typename F>
struct InstanceField<F(T::*)> : InstanceFieldBase {
    using type        = F(T::*);
    using class_type  = T;
    using return_type = std::remove_cv_t<F>;

    type fieldPtr;

    bool get(se::State &state) const override {
        T *         self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *thisObject = state.thisObject();
        return nativevalue_to_se((self->*fieldPtr), state.rval(), thisObject);
    }

    bool set(se::State &state) const override {
        T *         self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *thisObject = state.thisObject();
        auto &      args       = state.args();
        return sevalue_to_native(args[0], &(self->*fieldPtr), thisObject);
    }
};

struct InstanceAttributeBase {
    std::string  class_name;
    std::string  attr_name;
    std::string  signature;
    std::string  name() { return class_name + "::" + attr_name; }
    virtual bool get(se::State &) const = 0;
    virtual bool set(se::State &) const = 0;
};
template <typename T>
struct InstanceAttribute;

template <typename T, typename G, typename S>
struct AttributeAccessor;

template <typename T>
struct AccessorGet {
    using type = void;
    using return_type = void;
    using class_type = void;
};

template <typename T>
struct AccessorSet {
    using type = void;
    using value_type = void;
    using class_type = void;
};

template <>
struct AccessorGet<nullptr_t> {
    using class_type  = nullptr_t;
    using type        = nullptr_t;
    using return_type = nullptr_t;
};

template <>
struct AccessorSet<nullptr_t> {
    using class_type = nullptr_t;
    using type       = nullptr_t;
    using value_type = nullptr_t;
};

template <typename T, typename R>
struct AccessorGet<R(T::*)> {
    using class_type  = T;
    using type        = R(T::*);
    using return_type = R;
    static_assert(!std::is_void_v<R>);
};

template <typename T, typename _R, typename F>
struct AccessorSet<_R (T::*)(F)> {
    using class_type          = T;
    using type                = _R (T::*)(F);
    using value_type          = F;
    using ignored_return_type = _R;
};

template <typename T, typename Getter, typename Setter>
struct InstanceAttribute<AttributeAccessor<T, Getter, Setter>> : InstanceAttributeBase {
    using type              = T;
    using get_accessor      = typename AccessorGet<Getter>;
    using set_accessor      = typename AccessorSet<Setter>;
    using getter_type       = typename get_accessor::type;
    using setter_type       = typename set_accessor::type;
    using set_value_type    = std::remove_reference_t<std::remove_cv_t<typename set_accessor::value_type>>;
    using get_value_type    = std::remove_reference_t<std::remove_cv_t<typename get_accessor::return_type>>;
    using getter_class_type = typename get_accessor::class_type;
    using setter_class_type = typename set_accessor::class_type;

    constexpr static bool has_getter = !std::is_same_v<std::nullptr_t, getter_type>;
    constexpr static bool has_setter = !std::is_same_v<std::nullptr_t, setter_type>;

    static_assert(!has_getter || std::is_member_function_pointer<Getter>::value);
    static_assert(!has_setter || std::is_member_function_pointer<Setter>::value);
    static_assert(!has_getter || std::is_same<T, getter_class_type>::value);
    static_assert(!has_setter || std::is_same<T, setter_class_type>::value);

    setter_type setterPtr;
    getter_type getterPtr;

    bool get(se::State &state) const override {
        if constexpr (has_getter) {
            T *         self       = reinterpret_cast<T *>(state.nativeThisObject());
            se::Object *thisObject = state.thisObject();
            return nativevalue_to_se((self->*getterPtr)(), state.rval(), thisObject);
        }
        return false;
    }

    bool set(se::State &state) const override {
        if constexpr (has_setter) {
            T *                                                             self       = reinterpret_cast<T *>(state.nativeThisObject());
            se::Object *                                                    thisObject = state.thisObject();
            auto &                                                          args       = state.args();
            HolderType<set_value_type, std::is_reference_v<set_value_type>> temp;
            sevalue_to_native(args[0], &(temp.data), thisObject);
            (self->*setterPtr)(temp.value());
            return true;
        }
        return false;
    }
};

template <typename R, typename... ARGS>
struct StaticMethod {
    using type                          = R(ARGS...);
    using return_type                   = R;
    constexpr static size_t arg_count   = sizeof...(ARGS);
    constexpr static bool   return_void = std::is_same<void, R>::value;
};

template <typename A>
struct StaticAttribute {
    using type        = A;
    using return_type = A;
    using getter_type = A (*)();
    using setter_type = void (*)(A);
};

using SeCallbackType = bool(se::State &);

class context_db_ {
public:
    struct context_ {
        std::vector<std::tuple<std::string, InstanceAttributeBase *>> attributes;
        std::vector<std::tuple<std::string, InstanceFieldBase *>>     fields;
        std::vector<std::tuple<std::string, InstanceMethodBase *>>    methods;
        std::vector<std::tuple<std::string, InstanceMethodBase *>>    _staticMethods;
        std::vector<std::tuple<std::string, InstanceMethodBase *>>    _staticAttributes;
        std::vector<std::tuple<std::string, ConstructorBase *>>       constructors;
        std::string                                                   className;
        se::Class *                                                   kls         = nullptr;
        se::Object *                                                  nsObject    = nullptr;
        se::Object *                                                  parentProto = nullptr;
    };
    inline context_ *operator[](const std::string &key) {
        return this->operator[](key.c_str());
    }
    context_ *          operator[](const char *key);
    static context_db_ &instance();
    static void         reset();

private:
    std::map<std::string, context_ *> _contexts;
};

template <typename T>
class class_ {
public:
    using class_type = T;
    using context_   = context_db_::context_;

    class_(context_ *ctx) : _ctx(ctx) {}
    class_(const std::string &name);

    class_ &inherits(se::Object *parentProto);

    // set namespace object, parent
    class_ &ns(se::Object *nsObj);

    bool install();

    template <typename... ARGS>
    class_ &ctor();

    template <size_t N, typename Method>
    class_ &method(const char (&name)[N], Method method);

    template <size_t N, typename Field>
    class_ &field(const char (&name)[N], Field field);

    template <size_t N, typename Getter, typename Setter>
    class_ &attribute(const char (&name)[N], Getter getter, Setter setter);

    template <size_t N, typename R, typename... ARGS>
    class_ &static_method(const char (&name)[N], typename StaticMethod<R, ARGS...>::type method);

    template <size_t N, typename A>
    class_ &static_attribute(const char (&name)[N], typename StaticAttribute<A>::getter_type getter, typename StaticAttribute<A>::setter_type setter);

    se::Object *prototype() {
        return _ctx->kls->getProto();
    }

private:
    context_ *_ctx{nullptr};
    template <typename T>
    friend void genericConstructor(const v8::FunctionCallbackInfo<v8::Value> &_v8args);
};

template <typename T>
class_<T>::class_(const std::string &name) {
    _ctx            = context_db_::instance()[name];
    _ctx->className = name;
}

template <typename T>
class_<T> &class_<T>::inherits(se::Object *obj) {
    _ctx->parentProto = obj;
    return *this;
}

template <typename T>
class_<T> &class_<T>::ns(se::Object *nsobj) {
    _ctx->nsObject = nsobj;
    return *this;
}

template <typename T>
template <typename... ARGS>
class_<T> &class_<T>::ctor() {
    using CTYPE = Constructor<TypeList<T, ARGS...>>;
    //static_assert(!std::is_same_v<void, InstanceMethod<Method>::return_type>);
    auto *constructp      = new CTYPE();
    constructp->arg_count = sizeof...(ARGS);
    _ctx->constructors.emplace_back("_ctor", constructp);
    return *this;
}

template <typename T>
template <size_t N, typename Method>
class_<T> &class_<T>::method(const char (&name)[N], Method method) {
    using MTYPE = InstanceMethod<Method>;
    //static_assert(!std::is_same_v<void, InstanceMethod<Method>::return_type>);
    static_assert(std::is_same<MTYPE::class_type, T>::value);
    static_assert(std::is_member_function_pointer_v<Method>);
    auto *methodp        = new MTYPE();
    methodp->fnPtr       = method;
    methodp->method_name = name;
    methodp->class_name  = _ctx->className;
    methodp->arg_count   = MTYPE::argN;
    _ctx->methods.emplace_back(name, methodp);
    return *this;
}

template <typename T>
template <size_t N, typename Field>
class_<T> &class_<T>::field(const char (&name)[N], Field field) {
    static_assert(std::is_member_pointer<Field>::value);
    using FTYPE = InstanceField<Field>;
    static_assert(std::is_same<FTYPE::class_type, T>::value);
    auto *fieldp       = new FTYPE();
    fieldp->fieldPtr   = field;
    fieldp->field_name = name;
    fieldp->class_name = _ctx->className;
    _ctx->fields.emplace_back(name, fieldp);
    return *this;
}

template <typename T>
template <size_t N, typename Getter, typename Setter>
class_<T> &class_<T>::attribute(const char (&name)[N], Getter getter, Setter setter) {
    using ATYPE       = InstanceAttribute<AttributeAccessor<T, Getter, Setter>>;
    auto *attrp       = new ATYPE();
    attrp->getterPtr  = ATYPE::has_getter ? getter : nullptr;
    attrp->setterPtr  = ATYPE::has_setter ? setter : nullptr;
    attrp->class_name = _ctx->className;
    attrp->attr_name  = name;
    _ctx->attributes.emplace_back(name, attrp);
    return *this;
}

// v8 only
template <typename T>
void genericConstructor(const v8::FunctionCallbackInfo<v8::Value> &_v8args) {
    using context_type       = class_<T>::context_;
    v8::Isolate *   _isolate = _v8args.GetIsolate();
    v8::HandleScope _hs(_isolate);
    bool            ret = false;
    se::ValueArray  args;
    args.reserve(10);
    se::internal::jsToSeArgs(_v8args, args);
    auto *      self       = reinterpret_cast<context_type *>(_v8args.Data().IsEmpty() ? nullptr : _v8args.Data().As<v8::External>()->Value());
    se::Object *thisObject = se::Object::_createJSObject(self->kls, _v8args.This());
    //thisObject->_setFinalizeCallback(_SE(finalizeCb)); // TODO: common finalizer
    se::State state(thisObject, args);

    assert(!self->constructors.empty());
    for (auto &ctor : self->constructors) {
        if (std::get<1>(ctor)->arg_count == args.size()) {
            ret = std::get<1>(ctor)->construct(state);
            if (ret) break;
        }
    }
    if (!ret) {
        SE_LOGE("[ERROR] Failed to invoke %s, location: %s:%d\n", "constructor", __FILE__, __LINE__);
    }
    se::Value _property;
    bool      _found = false;
    _found           = thisObject->getProperty("_ctor", &_property);
    if (_found) _property.toObject()->call(args, thisObject);
}

template <typename ContextType>
void genericAccessorSet(v8::Local<v8::Name> property, v8::Local<v8::Value> _value,
                        const v8::PropertyCallbackInfo<void> &_v8args) {
    auto *attr = reinterpret_cast<ContextType *>(_v8args.Data().IsEmpty() ? nullptr : _v8args.Data().As<v8::External>()->Value());
    assert(attr);
    v8::Isolate *          _isolate = _v8args.GetIsolate();
    v8::HandleScope        _hs(_isolate);
    bool                   ret           = true;
    se::PrivateObjectBase *privateObject = static_cast<se::PrivateObjectBase *>(se::internal::getPrivate(_isolate, _v8args.This(), 0));
    se::Object *           thisObject    = reinterpret_cast<se::Object *>(se::internal::getPrivate(_isolate, _v8args.This(), 1));
    se::ValueArray &       args          = se::gValueArrayPool.get(1);
    se::CallbackDepthGuard depthGuard{args, se::gValueArrayPool._depth};
    se::Value &            data{args[0]};
    se::internal::jsToSeValue(_isolate, _value, &data);
    se::State state(thisObject, privateObject, args);
    ret = attr->set(state);
    if (!ret) {
        SE_LOGE("[ERROR] Failed to invoke set %s, location: %s:%d\n", attr->name().c_str(), __FILE__, __LINE__);
    }
}
template <typename ContextType>
void genericAccessorGet(v8::Local<v8::Name>                        property,
                        const v8::PropertyCallbackInfo<v8::Value> &_v8args) {
    auto *attr = reinterpret_cast<ContextType *>(_v8args.Data().IsEmpty() ? nullptr : _v8args.Data().As<v8::External>()->Value());
    assert(attr);
    v8::Isolate *          _isolate = _v8args.GetIsolate();
    v8::HandleScope        _hs(_isolate);
    bool                   ret           = true;
    se::PrivateObjectBase *privateObject = static_cast<se::PrivateObjectBase *>(se::internal::getPrivate(_isolate, _v8args.This(), 0));
    se::Object *           thisObject    = reinterpret_cast<se::Object *>(se::internal::getPrivate(_isolate, _v8args.This(), 1));
    se::State              state(thisObject, privateObject);
    ret = attr->get(state);
    if (!ret) {
        SE_LOGE("[ERROR] Failed to invoke %s, location: %s:%d\n", attr->name().c_str(), __FILE__, __LINE__);
    }
    se::internal::setReturnValue(state.rval(), _v8args);
}

void genericFunction(const v8::FunctionCallbackInfo<v8::Value> &_v8args);

template <typename T>
bool class_<T>::install() {
    constexpr auto *fn = &genericConstructor<T>;
    assert(_ctx->nsObject);

    if (_ctx->constructors.empty()) {
        if constexpr (std::is_default_constructible<T>::value) {
            ctor();
        }
    }

    _ctx->kls = se::Class::create(_ctx->className, _ctx->nsObject, _ctx->parentProto, fn, _ctx);

    auto *getter = &genericAccessorGet<InstanceAttributeBase>;
    auto *setter = &genericAccessorSet<InstanceAttributeBase>;
    for (auto &attr : _ctx->attributes) {
        _ctx->kls->defineProperty(std::get<0>(attr).c_str(), getter, setter, std::get<1>(attr));
    }
    auto *field_getter = &genericAccessorGet<InstanceFieldBase>;
    auto *field_setter = &genericAccessorSet<InstanceFieldBase>;
    for (auto &field : _ctx->fields) {
        _ctx->kls->defineProperty(std::get<0>(field).c_str(), field_getter, field_setter, std::get<1>(field));
    }
    {
        std::vector<InstanceMethodBase *>                        uniqueMethods;
        std::map<std::string, std::vector<InstanceMethodBase *>> multimap;
        for (auto &m : _ctx->methods) {
            multimap[std::get<0>(m)].emplace_back(std::get<1>(m));
        }
        for (auto &m : multimap) {
            if (m.second.size() > 1) {
                auto *overloaded        = new InstanceMethodOverloaded;
                overloaded->class_name  = _ctx->className;
                overloaded->method_name = m.first;
                for (auto *i : m.second) {
                    overloaded->methods.push_back(i);
                }
                _ctx->kls->defineFunction(m.first.c_str(), &genericFunction, overloaded);
            } else {
                _ctx->kls->defineFunction(m.first.c_str(), &genericFunction, m.second[0]);
            }
        }
    }

    _ctx->kls->install();
    return true;
}

} // namespace setpl