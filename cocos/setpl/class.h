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
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        auto &              jsArgs = state.args();
        convert_js_args_to_tuple(jsArgs, args, nullptr, std::make_index_sequence<sizeof...(ARGS)>());
        T *self = constructWithTuple(args, std::make_index_sequence<sizeof...(ARGS)>());
        state.thisObject()->setPrivateData(self);
        return true;
    }

    template <typename ...ARGS_HT, size_t... indexes>
    T *constructWithTuple(std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) {
        return new T(std::get<indexes>(args).value()...);
    }
};

struct InstanceMethodBase {
    std::string  class_name;
    std::string  method_name;
    std::string  signature;
    virtual bool invoke(se::State &) const = 0;
};
template <typename T>
struct InstanceMethod;

template <typename T, typename R, typename... ARGS>
struct InstanceMethod<R (T::*)(ARGS...)> : InstanceMethodBase {
    using type                          = R (T::*)(ARGS...);
    using return_type                   = R;
    constexpr static size_t arg_count   = sizeof...(ARGS);
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

    template <typename ...ARGS_HT,size_t... indexes>
    R callWithTuple(T *self, std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) const {
        return ((reinterpret_cast<T *>(self))->*fnPtr)(std::get<indexes>(args).value()...);
    }

    constexpr bool invoke(se::State &state) const override {
        constexpr auto      indexes{std::make_index_sequence<sizeof...(ARGS)>()};
        T *                 self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *        thisObject = state.thisObject();
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        const auto &        jsArgs = state.args();
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

struct InstanceFieldBase {
    std::string  class_name;
    std::string  field_name;
    std::string  signature;
    virtual bool get(se::State &) const = 0;
    virtual bool set(se::State &) const = 0;
};

template <typename T>
struct InstanceField;

template <typename T, typename F>
struct InstanceField<F(T::*)> : InstanceFieldBase {
    using type        = F(T::*);
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
    virtual bool get(se::State &) const = 0;
    virtual bool set(se::State &) const = 0;
};
template <typename T>
struct InstanceAttribute;

template <typename T, typename G, typename S>
struct AttributeAccessor;

template <typename T>
struct AccessorGet;

template <typename T>
struct AccessorSet;

template <typename T, typename R>
struct AccessorGet<R(T::*)> {
    using type        = R(T::*);
    using return_type = R;
};
template <typename T, typename F>
struct AccessorSet<void (T::*)(F)> {
    using type       = void (T::*)(F);
    using value_type = F;
};

template <typename T, typename Getter, typename Setter>
struct InstanceAttribute<AttributeAccessor<T, Getter, Setter>> : InstanceAttributeBase {
    using type           = T;
    using setter_type    = typename AccessorSet<Setter>::type;
    using getter_type    = typename AccessorGet<Getter>::type;
    using set_value_type = std::remove_reference_t<std::remove_cv_t<typename AccessorSet<Setter>::value_type>>;
    using get_value_type = std::remove_reference_t<std::remove_cv_t<typename AccessorGet<Getter>::return_type>>;

    setter_type setterPtr;
    getter_type getterPtr;

    bool get(se::State &state) const override {
        T *         self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *thisObject = state.thisObject();
        return nativevalue_to_se((self->*getterPtr)(), state.rval(), thisObject);
    }

    bool set(se::State &state) const override {
        T *                                                             self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *                                                    thisObject = state.thisObject();
        auto &                                                          args       = state.args();
        HolderType<set_value_type, std::is_reference_v<set_value_type>> temp;
        sevalue_to_native(args[0], &(temp.data), thisObject);
        (self->*setterPtr)(temp.value());
        return true;
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

template <typename T>
class class_ {
public:
    using class_type = T;

    class_(const std::string &name);

    class_ &inherits(class_ &superClass);
    bool    install();

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

private:
    std::vector<std::tuple<std::string, InstanceAttributeBase *>> _attributes;
    std::vector<std::tuple<std::string, InstanceFieldBase *>>     _fields;
    std::vector<std::tuple<std::string, InstanceMethodBase *>>    _methods;
    std::vector<std::tuple<std::string, InstanceMethodBase *>>    _staticMethods;
    std::vector<std::tuple<std::string, InstanceMethodBase *>>    _staticAttributes;
    std::vector<std::tuple<std::string, ConstructorBase *>>       _constructors;

    std::string _className;
    se::Class * _kls = nullptr;
};

template <typename T>
struct RegisterMethod;

template <typename T, typename R, typename... ARGS>
struct RegisterMethod<R (T::*)(ARGS...)> {
    using type = R (T::*)(ARGS...);
};

template <typename T>
template <typename... ARGS>
class_<T> &class_<T>::ctor() {
    using CTYPE = Constructor<TypeList<T, ARGS...>>;
    //static_assert(!std::is_same_v<void, InstanceMethod<Method>::return_type>);
    auto *constructp = new CTYPE();
    _constructors.emplace_back("_ctor", constructp);
    return *this;
}

template <typename T>
template <size_t N, typename Method>
class_<T> &class_<T>::method(const char (&name)[N], Method method) {
    using MTYPE = InstanceMethod<Method>;
    //static_assert(!std::is_same_v<void, InstanceMethod<Method>::return_type>);
    auto *methodp        = new MTYPE();
    methodp->fnPtr       = method;
    methodp->method_name = name;
    methodp->class_name  = _className;
    _methods.emplace_back(name, methodp);
    return *this;
}

template <typename T>
template <size_t N, typename Field>
class_<T> &class_<T>::field(const char (&name)[N], Field field) {
    using FTYPE        = InstanceField<Field>;
    auto *fieldp       = new FTYPE();
    fieldp->fieldPtr   = field;
    fieldp->field_name = name;
    fieldp->class_name = _className;
    _fields.emplace_back(name, fieldp);
    return *this;
}

template <typename T>
template <size_t N, typename Getter, typename Setter>
class_<T> &class_<T>::attribute(const char (&name)[N], Getter getter, Setter setter) {
    using ATYPE       = InstanceAttribute<AttributeAccessor<T, Getter, Setter>>;
    auto *attrp       = new ATYPE();
    attrp->getterPtr  = getter;
    attrp->setterPtr  = setter;
    attrp->class_name = _className;
    attrp->attr_name  = name;
    _attributes.emplace_back(name, attrp);
    return *this;
}

} // namespace setpl