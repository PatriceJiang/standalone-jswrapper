#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "jswrapper/SeApi.h"
#include "jswrapper/Value.h"

#include "conversions/jsb_conversions.h"
#include "conversions/jsb_global.h"

//#include "signature.h"

namespace sebind {

using SeCallbackType  = bool(se::State &);
using SeCallbackFnPtr = bool (*)(se::State &);

namespace intl {

template <typename... ARGS>
struct TypeList;

template <typename T, typename... OTHERS>
struct Concat;

template <typename T, typename... OTHERS>
struct Concat<T, TypeList<OTHERS...>> {
    using type = TypeList<T, OTHERS...>;
};

template <typename... ARGS, size_t... indexes>
bool convert_js_args_to_tuple(const se::ValueArray &jsArgs, std::tuple<ARGS...> &args, se::Object *thisObj, std::index_sequence<indexes...>) {
    std::array<bool, sizeof...(indexes)> all = {(sevalue_to_native(jsArgs[indexes], &std::get<indexes>(args).data, thisObj), ...)};
    return true;
}
template <size_t... indexes>
bool convert_js_args_to_tuple(const se::ValueArray &jsArgs, std::tuple<> &args, se::Object *ctx, std::index_sequence<indexes...>) {
    return true;
}

struct ConstructorBase {
    size_t          arg_count = 0;
    SeCallbackFnPtr bfnPtr{nullptr};
    virtual bool    construct(se::State &state) {
        if (bfnPtr) return (*bfnPtr)(state);
        return false;
    }
};
struct InstanceMethodBase {
    std::string     class_name;
    std::string     method_name;
    size_t          arg_count;
    SeCallbackFnPtr bfnPtr{nullptr};
    virtual bool    invoke(se::State &state) const {
        if (bfnPtr) return (*bfnPtr)(state);
        return false;
    }
};
struct FinalizerBase {
    virtual void finalize(void *) = 0;
};

struct InstanceFieldBase {
    std::string     class_name;
    std::string     attr_name;
    SeCallbackFnPtr bfnSetPtr{nullptr};
    SeCallbackFnPtr bfnGetPtr{nullptr};
    virtual bool    get(se::State &state) const {
        if (bfnGetPtr) return (*bfnGetPtr)(state);
        return false;
    }
    virtual bool set(se::State &state) const {
        if (bfnSetPtr) return (*bfnSetPtr)(state);
        return false;
    }
};

struct InstanceAttributeBase {
    std::string     class_name;
    std::string     attr_name;
    SeCallbackFnPtr bfnSetPtr{nullptr};
    SeCallbackFnPtr bfnGetPtr{nullptr};
    virtual bool    get(se::State &state) const {
        if (bfnGetPtr) return (*bfnGetPtr)(state);
        return false;
    }
    virtual bool set(se::State &state) const {
        if (bfnSetPtr) return (*bfnSetPtr)(state);
        return false;
    }
};

struct StaticMethodBase {
    std::string     class_name;
    std::string     method_name;
    size_t          arg_count;
    SeCallbackFnPtr bfnPtr{nullptr};
    virtual bool    invoke(se::State &state) const {
        if (bfnPtr) return (*bfnPtr)(state);
        return false;
    }
};
struct StaticAttributeBase {
    std::string     class_name;
    std::string     attr_name;
    SeCallbackFnPtr bfnSetPtr{nullptr};
    SeCallbackFnPtr bfnGetPtr{nullptr};
    virtual bool    get(se::State &state) const {
        if (bfnGetPtr) return (*bfnGetPtr)(state);
        return false;
    }
    virtual bool set(se::State &state) const {
        if (bfnSetPtr) return (*bfnSetPtr)(state);
        return false;
    }
};

template <typename T>
struct Constructor;

template <typename T>
struct InstanceField;

template <typename T>
struct InstanceMethod;

template <typename T>
struct InstanceAttribute;

template <typename T>
struct StaticMethod;

template <typename T>
struct StaticAttribute;

template <typename T, typename... ARGS>
struct Constructor<TypeList<T, ARGS...>> : ConstructorBase {
    bool construct(se::State &state) {
        if ((sizeof...(ARGS)) != state.args().size()) {
            return false;
        }
        se::PrivateObjectBase *                                    self{nullptr};
        se::Object *                                               thisObj = state.thisObject();
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        auto &                                                     jsArgs = state.args();
        convert_js_args_to_tuple(jsArgs, args, thisObj, std::make_index_sequence<sizeof...(ARGS)>());
        self = constructWithTuple(args, std::make_index_sequence<sizeof...(ARGS)>());
        state.thisObject()->setPrivateObject(self);
        return true;
    }

    template <typename... ARGS_HT, size_t... indexes>
    se::PrivateObjectBase *constructWithTuple(std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) {
        return JSB_MAKE_PRIVATE_OBJECT(T, std::get<indexes>(args).value()...);
    }
};

template <typename T, typename... ARGS>
struct Constructor<T *(*)(ARGS...)> : ConstructorBase {
    using type = T *(*)(ARGS...);
    type func;
    bool construct(se::State &state) {
        if ((sizeof...(ARGS)) != state.args().size()) {
            return false;
        }
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        auto &                                                     jsArgs  = state.args();
        se::Object *                                               thisObj = state.thisObject();
        convert_js_args_to_tuple(jsArgs, args, thisObj, std::make_index_sequence<sizeof...(ARGS)>());
        auto *ptr = constructWithTuple(args, std::make_index_sequence<sizeof...(ARGS)>());
        state.thisObject()->setPrivateData(ptr);
        return true;
    }

    template <typename... ARGS_HT, size_t... indexes>
    T *constructWithTuple(std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) {
        return (*func)(std::get<indexes>(args).value()...);
    }
};

template <typename T>
struct Finalizer : FinalizerBase {
    using type = void (*)(T *);
    type func;
    void finalize(void *p) override {
        (*func)(reinterpret_cast<T *>(p));
    }
};

template <typename T, typename R, typename... ARGS>
struct InstanceMethod<R (T::*)(ARGS...)> : InstanceMethodBase {
    using type                          = R (T::*)(ARGS...);
    using return_type                   = R;
    using class_type                    = std::remove_cv_t<T>;
    constexpr static size_t argN        = sizeof...(ARGS);
    constexpr static bool   return_void = std::is_same<void, R>::value;

    type func = nullptr;

    template <typename... ARGS_HT, size_t... indexes>
    R callWithTuple(T *self, std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) const {
        return ((reinterpret_cast<T *>(self))->*func)(std::get<indexes>(args).value()...);
    }

    bool invoke(se::State &state) const override {
        constexpr auto indexes{std::make_index_sequence<sizeof...(ARGS)>()};
        T *            self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *   thisObject = state.thisObject();
        const auto &   jsArgs     = state.args();
        if (argN != jsArgs.size()) {
            SE_LOGE("incorret argument size %d, expect %d\n", static_cast<int>(jsArgs.size()), static_cast<int>(argN));
            return false;
        }
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        convert_js_args_to_tuple(jsArgs, args, thisObject, indexes);
        if constexpr (return_void) {
            callWithTuple(self, args, indexes);
        } else {
            nativevalue_to_se(callWithTuple(self, args, indexes), state.rval(), thisObject);
        }
        return true;
    }
};

template <typename T, typename R, typename... ARGS>
struct InstanceMethod<R (*)(T *, ARGS...)> : InstanceMethodBase {
    using type                          = R (*)(T *, ARGS...);
    using return_type                   = R;
    using class_type                    = std::remove_cv_t<T>;
    constexpr static size_t argN        = sizeof...(ARGS);
    constexpr static bool   return_void = std::is_same<void, R>::value;

    type func = nullptr;

    template <typename... ARGS_HT, size_t... indexes>
    R callWithTuple(T *self, std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) const {
        return (*func)(reinterpret_cast<T *>(self), std::get<indexes>(args).value()...);
    }

    bool invoke(se::State &state) const override {
        constexpr auto indexes{std::make_index_sequence<sizeof...(ARGS)>()};
        T *            self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *   thisObject = state.thisObject();
        const auto &   jsArgs     = state.args();
        if (argN != jsArgs.size()) {
            SE_LOGE("incorret argument size %d, expect %d\n", jsArgs.size(), argN);
            return false;
        }
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        convert_js_args_to_tuple(jsArgs, args, thisObject, indexes);
        if constexpr (return_void) {
            callWithTuple(self, args, indexes);
        } else {
            nativevalue_to_se(callWithTuple(self, args, indexes), state.rval(), thisObject);
        }
        return true;
    }
};

struct InstanceMethodOverloaded : InstanceMethodBase {
    std::vector<InstanceMethodBase *> functions;
    bool                              invoke(se::State &state) const override {
        bool ret      = false;
        auto argCount = state.args().size();
        for (auto *m : functions) {
            if (m->arg_count == argCount) {
                ret = m->invoke(state);
                if (ret) return true;
            }
        }
        return false;
    }
};

template <typename T, typename F>
struct InstanceField<F(T::*)> : InstanceFieldBase {
    using type        = F(T::*);
    using class_type  = T;
    using return_type = std::remove_cv_t<F>;

    type func;

    bool get(se::State &state) const override {
        T *         self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *thisObject = state.thisObject();
        return nativevalue_to_se((self->*func), state.rval(), thisObject);
    }

    bool set(se::State &state) const override {
        T *         self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *thisObject = state.thisObject();
        auto &      args       = state.args();
        return sevalue_to_native(args[0], &(self->*func), thisObject);
    }
};

template <typename T, typename G, typename S>
struct AttributeAccessor;

template <typename T>
struct AccessorGet {
    using type        = void;
    using return_type = void;
    using class_type  = void;
};

template <typename T>
struct AccessorSet {
    using type       = void;
    using value_type = void;
    using class_type = void;
};

template <>
struct AccessorGet<std::nullptr_t> {
    using class_type  = std::nullptr_t;
    using type        = std::nullptr_t;
    using return_type = std::nullptr_t;
};

template <>
struct AccessorSet<std::nullptr_t> {
    using class_type = std::nullptr_t;
    using type       = std::nullptr_t;
    using value_type = std::nullptr_t;
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

template <typename T, typename _R, typename F>
struct AccessorSet<_R (*)(T *, F)> {
    using class_type          = T;
    using type                = _R (*)(T *, F);
    using value_type          = F;
    using ignored_return_type = _R;
};
template <typename T, typename R>
struct AccessorGet<R (*)(T *)> {
    using class_type  = T;
    using type        = R (*)(T *);
    using return_type = R;
    static_assert(!std::is_void_v<R>);
};

template <typename T, typename Getter, typename Setter>
struct InstanceAttribute<AttributeAccessor<T, Getter, Setter>> : InstanceAttributeBase {
    using type              = T;
    using get_accessor      = AccessorGet<Getter>;
    using set_accessor      = AccessorSet<Setter>;
    using getter_type       = typename get_accessor::type;
    using setter_type       = typename set_accessor::type;
    using set_value_type    = std::remove_reference_t<std::remove_cv_t<typename set_accessor::value_type>>;
    using get_value_type    = std::remove_reference_t<std::remove_cv_t<typename get_accessor::return_type>>;
    using getter_class_type = typename get_accessor::class_type;
    using setter_class_type = typename set_accessor::class_type;

    constexpr static bool has_getter          = !std::is_same_v<std::nullptr_t, getter_type>;
    constexpr static bool has_setter          = !std::is_same_v<std::nullptr_t, setter_type>;
    constexpr static bool getter_is_member_fn = has_getter && std::is_member_function_pointer<Getter>::value;
    constexpr static bool setter_is_member_fn = has_setter && std::is_member_function_pointer<Setter>::value;

    static_assert(!has_getter || std::is_base_of<getter_class_type, T>::value);
    static_assert(!has_setter || std::is_base_of<setter_class_type, T>::value);

    setter_type setterPtr;
    getter_type getterPtr;

    bool get(se::State &state) const override {
        if constexpr (has_getter) {
            T *         self       = reinterpret_cast<T *>(state.nativeThisObject());
            se::Object *thisObject = state.thisObject();
            if constexpr (getter_is_member_fn) {
                return nativevalue_to_se((self->*getterPtr)(), state.rval(), thisObject);
            } else {
                return nativevalue_to_se((*getterPtr)(self), state.rval(), thisObject);
            }
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
            if constexpr (setter_is_member_fn) {
                (self->*setterPtr)(temp.value());
            } else {
                (*setterPtr)(self, temp.value());
            }
            return true;
        }
        return false;
    }
};

template <typename R, typename... ARGS>
struct StaticMethod<R (*)(ARGS...)> : StaticMethodBase {
    using type                          = R (*)(ARGS...);
    using return_type                   = R;
    constexpr static size_t argN        = sizeof...(ARGS);
    constexpr static bool   return_void = std::is_same<void, R>::value;

    type func = nullptr;

    template <typename... ARGS_HT, size_t... indexes>
    R callWithTuple(std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...>) const {
        return (*func)(std::get<indexes>(args).value()...);
    }

    bool invoke(se::State &state) const override {
        constexpr auto indexes{std::make_index_sequence<sizeof...(ARGS)>()};
        const auto &   jsArgs = state.args();
        if (argN != jsArgs.size()) {
            SE_LOGE("incorret argument size %d, expect %d\n", jsArgs.size(), argN);
            return false;
        }
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        convert_js_args_to_tuple(jsArgs, args, nullptr, indexes);
        if constexpr (return_void) {
            callWithTuple(args, indexes);
        } else {
            nativevalue_to_se(callWithTuple(args, indexes), state.rval(), nullptr);
        }
        return true;
    }
};

struct StaticMethodOverloaded : StaticMethodBase {
    std::vector<StaticMethodBase *> functions;
    bool                            invoke(se::State &state) const override {
        bool ret      = false;
        auto argCount = state.args().size();
        for (auto *m : functions) {
            if (m->arg_count == argCount) {
                ret = m->invoke(state);
                if (ret) return true;
            }
        }
        return false;
    }
};

template <typename T, typename G, typename S>
struct SAttributeAccessor;

template <typename T>
struct SAccessorGet {
    using type        = void;
    using return_type = void;
};

template <typename T>
struct SAccessorSet {
    using type       = void;
    using value_type = void;
};

template <>
struct SAccessorGet<std::nullptr_t> {
    using type        = std::nullptr_t;
    using return_type = std::nullptr_t;
};

template <>
struct SAccessorSet<std::nullptr_t> {
    using type       = std::nullptr_t;
    using value_type = std::nullptr_t;
};

template <typename R>
struct SAccessorGet<R(*)> {
    using type        = R(*);
    using return_type = R;
    static_assert(!std::is_void_v<R>);
};

template <typename _R, typename F>
struct SAccessorSet<_R (*)(F)> {
    using type                = _R (*)(F);
    using value_type          = F;
    using ignored_return_type = _R;
};

template <typename T, typename Getter, typename Setter>
struct StaticAttribute<SAttributeAccessor<T, Getter, Setter>> : StaticAttributeBase {
    using type           = T;
    using get_accessor   = SAccessorGet<Getter>;
    using set_accessor   = SAccessorSet<Setter>;
    using getter_type    = typename get_accessor::type;
    using setter_type    = typename set_accessor::type;
    using set_value_type = std::remove_reference_t<std::remove_cv_t<typename set_accessor::value_type>>;
    using get_value_type = std::remove_reference_t<std::remove_cv_t<typename get_accessor::return_type>>;

    constexpr static bool has_getter = !std::is_same_v<std::nullptr_t, getter_type>;
    constexpr static bool has_setter = !std::is_same_v<std::nullptr_t, setter_type>;

    static_assert(has_getter || has_setter);

    setter_type setterPtr;
    getter_type getterPtr;

    bool get(se::State &state) const override {
        if constexpr (has_getter) {
            return nativevalue_to_se((*getterPtr)(), state.rval(), nullptr);
        }
        return false;
    }

    bool set(se::State &state) const override {
        if constexpr (has_setter) {
            auto &                                                          args = state.args();
            HolderType<set_value_type, std::is_reference_v<set_value_type>> temp;
            sevalue_to_native(args[0], &(temp.data), nullptr);
            (*setterPtr)(temp.value());
            return true;
        }
        return false;
    }
};

class context_db_ {
public:
    struct context_ {
        std::vector<std::tuple<std::string, std::unique_ptr<InstanceAttributeBase>>> properties;
        std::vector<std::tuple<std::string, std::unique_ptr<InstanceFieldBase>>>     fields;
        std::vector<std::tuple<std::string, std::unique_ptr<InstanceMethodBase>>>    functions;
        std::vector<std::tuple<std::string, std::unique_ptr<StaticMethodBase>>>      staticFunctions;
        std::vector<std::tuple<std::string, std::unique_ptr<StaticAttributeBase>>>   staticProperties;
        std::vector<std::unique_ptr<ConstructorBase>>                                constructors;
        std::vector<std::unique_ptr<FinalizerBase>>                                  finalizeCallbacks;
        std::string                                                                  className;
        se::Class *                                                                  kls = nullptr;
        // se::Object *                                                  nsObject    = nullptr;
        se::Object *parentProto = nullptr;
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
struct FunctionWrapper;

template <typename R, typename... ARGS>
struct FunctionWrapper<R *(*)(ARGS...)> {
    using type                        = R *(*)(ARGS...);
    static constexpr size_t arg_count = sizeof...(ARGS);
};

} // namespace intl

template <typename T>
class class_ {
public:
    using class_type = T;
    using context_   = intl::context_db_::context_;

    class_(context_ *ctx) : _ctx(ctx) {}
    class_(const std::string &name);

    ~class_() {
        assert(_installed); // procedure `class_::install` has not been invoked?
    }

    class_ &inherits(se::Object *parentProto);

    bool install(se::Object *nsObject);

    // set namespace object, parent
    // class_ &namespaceObject(se::Object *nsObj);

    template <typename... ARGS>
    class_ &constructor();

    template <typename F>
    class_ &constructor(F);

    template <typename F>
    class_ &finalizer(F);

    template <typename Method>
    class_ &function(const std::string &name, Method method);

    template <typename Field>
    class_ &property(const std::string &name, Field field);

    template <typename Getter, typename Setter>
    class_ &property(const std::string &name, Getter getter, Setter setter);

    template <typename Method>
    class_ &staticFunction(const std::string &name, Method method);

    template <typename Getter, typename Setter>
    class_ &staticProperty(const std::string &name, Getter getter, Setter setter);

    class_ &constructor(SeCallbackFnPtr c);

    class_ &function(const std::string &name, SeCallbackFnPtr method);

    class_ &property(const std::string &name, SeCallbackFnPtr getter, SeCallbackFnPtr setter);

    class_ &staticFunction(const std::string &name, SeCallbackFnPtr c);

    class_ &staticProperty(const std::string &name, SeCallbackFnPtr getter, SeCallbackFnPtr setter);

    se::Object *prototype() {
        return _ctx->kls->getProto();
    }

private:
    bool      _installed{false};
    context_ *_ctx{nullptr};
    template <typename R>
    friend void genericConstructor(const v8::FunctionCallbackInfo<v8::Value> &);
};

template <typename T>
class_<T>::class_(const std::string &name) {
    _ctx            = intl::context_db_::instance()[name];
    _ctx->className = name;
}

template <typename T>
class_<T> &class_<T>::inherits(se::Object *obj) {
    assert(!_ctx->parentProto); // already set
    _ctx->parentProto = obj;
    return *this;
}

template <typename T>
template <typename... ARGS>
class_<T> &class_<T>::constructor() {
    using CTYPE = intl::Constructor<intl::TypeList<T, ARGS...>>;
    // static_assert(!std::is_same_v<void, InstanceMethod<Method>::return_type>);
    auto *constructp      = new CTYPE();
    constructp->arg_count = sizeof...(ARGS);
    _ctx->constructors.emplace_back(constructp);
    return *this;
}

template <typename T>
template <typename F>
class_<T> &class_<T>::constructor(F c) {
    using FTYPE           = intl::FunctionWrapper<F>;
    using CTYPE           = intl::Constructor<typename FTYPE::type>;
    auto *constructp      = new CTYPE();
    constructp->arg_count = FTYPE::arg_count;
    constructp->func      = c;
    _ctx->constructors.emplace_back(constructp);
    return *this;
}

template <typename T>
class_<T> &class_<T>::constructor(SeCallbackFnPtr c) {
    auto *constructp      = new intl::ConstructorBase();
    constructp->arg_count = -1;
    constructp->bfnPtr    = c;
    _ctx->constructors.emplace_back(constructp);
    return *this;
}

template <typename T>
template <typename F>
class_<T> &class_<T>::finalizer(F cb) {
    auto *f = new intl::Finalizer<T>();
    f->func = cb;
    _ctx->finalizeCallbacks.emplace_back(f);
    return *this;
}

template <typename T>
template <typename Method>
class_<T> &class_<T>::function(const std::string &name, Method method) {
    using MTYPE = intl::InstanceMethod<Method>;
    // static_assert(!std::is_same_v<void, InstanceMethod<Method>::return_type>);
    static_assert(std::is_base_of<typename MTYPE::class_type, T>::value);
    // static_assert(std::is_member_function_pointer_v<Method>);
    auto *methodp        = new MTYPE();
    methodp->method_name = name;
    methodp->class_name  = _ctx->className;
    methodp->arg_count   = MTYPE::argN;
    methodp->func        = method;
    _ctx->functions.emplace_back(name, methodp);
    return *this;
}

template <typename T>
class_<T> &class_<T>::function(const std::string &name, SeCallbackFnPtr c) {
    auto *methodp        = new intl::InstanceMethodBase();
    methodp->method_name = name;
    methodp->class_name  = _ctx->className;
    methodp->arg_count   = -1;
    methodp->bfnPtr      = c;
    _ctx->functions.emplace_back(name, methodp);
    return *this;
}

template <typename T>
template <typename Field>
class_<T> &class_<T>::property(const std::string &name, Field field) {
    static_assert(std::is_member_pointer<Field>::value);
    using FTYPE = intl::InstanceField<Field>;
    static_assert(std::is_base_of<typename FTYPE::class_type, T>::value);
    auto *fieldp       = new FTYPE();
    fieldp->func       = field;
    fieldp->attr_name  = name;
    fieldp->class_name = _ctx->className;
    _ctx->fields.emplace_back(name, fieldp);
    return *this;
}

template <typename T>
template <typename Getter, typename Setter>
class_<T> &class_<T>::property(const std::string &name, Getter getter, Setter setter) {
    using ATYPE       = intl::InstanceAttribute<intl::AttributeAccessor<T, Getter, Setter>>;
    auto *attrp       = new ATYPE();
    attrp->getterPtr  = ATYPE::has_getter ? getter : nullptr;
    attrp->setterPtr  = ATYPE::has_setter ? setter : nullptr;
    attrp->class_name = _ctx->className;
    attrp->attr_name  = name;
    _ctx->properties.emplace_back(name, attrp);
    return *this;
}

template <typename T>
class_<T> &class_<T>::property(const std::string &name, SeCallbackFnPtr getter, SeCallbackFnPtr setter) {
    auto *attrp       = new intl::InstanceAttributeBase();
    attrp->bfnGetPtr  = getter;
    attrp->bfnSetPtr  = setter;
    attrp->class_name = _ctx->className;
    attrp->attr_name  = name;
    _ctx->properties.emplace_back(name, attrp);
    return *this;
}

template <typename T>
template <typename Method>
class_<T> &class_<T>::staticFunction(const std::string &name, Method method) {
    using MTYPE          = intl::StaticMethod<Method>;
    auto *methodp        = new MTYPE();
    methodp->method_name = name;
    methodp->class_name  = _ctx->className;
    methodp->arg_count   = MTYPE::argN;
    methodp->func        = method;
    _ctx->staticFunctions.emplace_back(name, methodp);
    return *this;
}

template <typename T>
class_<T> &class_<T>::staticFunction(const std::string &name, SeCallbackFnPtr c) {
    auto *methodp        = new intl::StaticMethodBase();
    methodp->method_name = name;
    methodp->class_name  = _ctx->className;
    methodp->arg_count   = -1;
    methodp->bfnPtr      = c;
    _ctx->staticFunctions.emplace_back(name, methodp);
    return *this;
}

template <typename T>
template <typename Getter, typename Setter>
class_<T> &class_<T>::staticProperty(const std::string &name, Getter getter, Setter setter) {
    using ATYPE       = intl::StaticAttribute<intl::SAttributeAccessor<T, Getter, Setter>>;
    auto *attrp       = new ATYPE();
    attrp->getterPtr  = ATYPE::has_getter ? getter : nullptr;
    attrp->setterPtr  = ATYPE::has_setter ? setter : nullptr;
    attrp->class_name = _ctx->className;
    attrp->attr_name  = name;
    _ctx->staticProperties.emplace_back(name, attrp);
    return *this;
}

template <typename T>
class_<T> &class_<T>::staticProperty(const std::string &name, SeCallbackFnPtr getter, SeCallbackFnPtr setter) {
    auto *attrp       = new intl::StaticAttributeBase();
    attrp->bfnGetPtr  = getter;
    attrp->bfnSetPtr  = setter;
    attrp->class_name = _ctx->className;
    attrp->attr_name  = name;
    _ctx->staticProperties.emplace_back(name, attrp);
    return *this;
}

template <typename T>
void genericFinalizer(se::PrivateObjectBase *privateObject) {
    using context_type = typename class_<T>::context_;
    if (privateObject == nullptr)
        return;
    auto se = se::ScriptEngine::getInstance();
    se->_setGarbageCollecting(true);
    auto *self    = reinterpret_cast<context_type *>(privateObject->finalizerData);
    auto *thisPtr = privateObject->get<T>();
    for (auto &d : self->finalizeCallbacks) {
        d->finalize(thisPtr);
    }
    se->_setGarbageCollecting(false);
}

// v8 only
template <typename T>
void genericConstructor(const v8::FunctionCallbackInfo<v8::Value> &_v8args) {
    using context_type       = typename class_<T>::context_;
    v8::Isolate *   _isolate = _v8args.GetIsolate();
    v8::HandleScope _hs(_isolate);
    bool            ret = false;
    se::ValueArray  args;
    args.reserve(10);
    se::internal::jsToSeArgs(_v8args, args);
    auto *      self       = reinterpret_cast<context_type *>(_v8args.Data().IsEmpty() ? nullptr : _v8args.Data().As<v8::External>()->Value());
    se::Object *thisObject = se::Object::_createJSObject(self->kls, _v8args.This());
    if (!self->finalizeCallbacks.empty()) {
        auto *finalizer = &genericFinalizer<T>;
        thisObject->_setFinalizeCallback(finalizer);
    }
    se::State state(thisObject, args);

    assert(!self->constructors.empty());
    for (auto &ctor : self->constructors) {
        if (ctor->arg_count == -1 || ctor->arg_count == args.size()) {
            ret = ctor->construct(state);
            if (ret) break;
        }
    }
    if (!ret) {
        SE_LOGE("[ERROR] Failed to invoke %s, location: %s:%d\n", "constructor", __FILE__, __LINE__);
    }
    assert(ret); // construction failure is not allowed.
    if (!self->finalizeCallbacks.empty()) {
        state.thisObject()->getPrivateObject()->finalizerData = self;
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
        SE_LOGE("[ERROR] Failed to invoke set %s, location: %s:%d\n", attr->attr_name.c_str(), __FILE__, __LINE__);
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
        SE_LOGE("[ERROR] Failed to invoke %s, location: %s:%d\n", attr->attr_name.c_str(), __FILE__, __LINE__);
    }
    se::internal::setReturnValue(state.rval(), _v8args);
}

void genericFunction(const v8::FunctionCallbackInfo<v8::Value> &_v8args);

template <typename T>
bool class_<T>::install(se::Object *nsObject) {
    constexpr auto *fn = &genericConstructor<T>;
    assert(nsObject);
    _installed = true;

    if (_ctx->constructors.empty()) {
        if constexpr (std::is_default_constructible<T>::value) {
            constructor(); // add default constructor
        }
    }
    _ctx->kls = se::Class::create(_ctx->className, nsObject, _ctx->parentProto, fn, _ctx);

    auto *getter = &genericAccessorGet<intl::InstanceAttributeBase>;
    auto *setter = &genericAccessorSet<intl::InstanceAttributeBase>;
    for (auto &attr : _ctx->properties) {
        _ctx->kls->defineProperty(std::get<0>(attr).c_str(), getter, setter, std::get<1>(attr).get());
    }
    auto *field_getter = &genericAccessorGet<intl::InstanceFieldBase>;
    auto *field_setter = &genericAccessorSet<intl::InstanceFieldBase>;
    for (auto &field : _ctx->fields) {
        _ctx->kls->defineProperty(std::get<0>(field).c_str(), field_getter, field_setter, std::get<1>(field).get());
    }
    // defineFunctions
    {
        std::map<std::string, std::vector<intl::InstanceMethodBase *>> multimap;
        for (auto &m : _ctx->functions) {
            multimap[std::get<0>(m)].emplace_back(std::get<1>(m).get());
        }
        for (auto &m : multimap) {
            if (m.second.size() > 1) {
                auto *overloaded        = new intl::InstanceMethodOverloaded;
                overloaded->class_name  = _ctx->className;
                overloaded->method_name = m.first;
                for (auto *i : m.second) {
                    overloaded->functions.push_back(i);
                }
                _ctx->kls->defineFunction(m.first.c_str(), &genericFunction, overloaded);
            } else {
                _ctx->kls->defineFunction(m.first.c_str(), &genericFunction, m.second[0]);
            }
        }
    }
    // define static functions
    {
        std::map<std::string, std::vector<intl::StaticMethodBase *>> multimap;
        for (auto &m : _ctx->staticFunctions) {
            multimap[std::get<0>(m)].emplace_back(std::get<1>(m).get());
        }
        for (auto &m : multimap) {
            if (m.second.size() > 1) {
                auto *overloaded        = new intl::StaticMethodOverloaded;
                overloaded->class_name  = _ctx->className;
                overloaded->method_name = m.first;
                for (auto *i : m.second) {
                    overloaded->functions.push_back(i);
                }
                _ctx->kls->defineStaticFunction(m.first.c_str(), &genericFunction, overloaded);
            } else {
                _ctx->kls->defineStaticFunction(m.first.c_str(), &genericFunction, m.second[0]);
            }
        }
    }
    for (auto &prop : _ctx->staticProperties) {
        _ctx->kls->defineStaticProperty(std::get<0>(prop).c_str(), field_getter, field_setter, std::get<1>(prop).get());
    }
    _ctx->kls->install();
    return true;
}

} // namespace sebind