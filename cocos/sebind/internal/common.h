#pragma once

#include <array>
#include <tuple>
#include <cstdint>
#include <type_traits>

#include "conversions/jsb_conversions.h"
#include "conversions/jsb_global.h"

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

template <size_t M, size_t N>
struct AndAll {
    static constexpr bool cal(const std::array<bool, N> &bools) {
        return bools[M - 1] && AndAll<M - 1, N>::cal(bools);
    }
};

template <size_t N>
struct AndAll<0, N> {
    static constexpr bool cal(const std::array<bool, N> & /*unused*/) {
        return true;
    }
};

template <typename... ARGS, size_t... indexes>
// NOLINTNEXTLINE
bool convert_js_args_to_tuple(const se::ValueArray &jsArgs, std::tuple<ARGS...> &args, se::Object *thisObj, std::index_sequence<indexes...>) {
    constexpr static size_t ARG_N = sizeof...(ARGS);
    std::array<bool, ARG_N> all   = {(sevalue_to_native(jsArgs[indexes], &std::get<indexes>(args).data, thisObj), ...)};
    return AndAll<ARG_N, ARG_N>::cal(all);
}
template <size_t... indexes>
// NOLINTNEXTLINE
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
    bool construct(se::State &state) override {
        if ((sizeof...(ARGS)) != state.args().size()) {
            return false;
        }
        se::PrivateObjectBase                                     *self{nullptr};
        se::Object                                                *thisObj = state.thisObject();
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        const auto                                                &jsArgs = state.args();
        convert_js_args_to_tuple(jsArgs, args, thisObj, std::make_index_sequence<sizeof...(ARGS)>());
        self = constructWithTuple(args, std::make_index_sequence<sizeof...(ARGS)>());
        state.thisObject()->setPrivateObject(self);
        return true;
    }

    template <typename... ARGS_HT, size_t... indexes>
    se::PrivateObjectBase *constructWithTuple(std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...> /*unused*/) {
        return JSB_MAKE_PRIVATE_OBJECT(T, std::get<indexes>(args).value()...);
    }
};

template <typename T, typename... ARGS>
struct Constructor<T *(*)(ARGS...)> : ConstructorBase {
    using type = T *(*)(ARGS...);
    type func;
    bool construct(se::State &state) override {
        if ((sizeof...(ARGS)) != state.args().size()) {
            return false;
        }
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        const auto                                                &jsArgs  = state.args();
        se::Object                                                *thisObj = state.thisObject();
        convert_js_args_to_tuple(jsArgs, args, thisObj, std::make_index_sequence<sizeof...(ARGS)>());
        auto *ptr = constructWithTuple(args, std::make_index_sequence<sizeof...(ARGS)>());
        state.thisObject()->setPrivateData(ptr);
        return true;
    }

    template <typename... ARGS_HT, size_t... indexes>
    T *constructWithTuple(std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...> /*unused*/) {
        return (*func)(std::get<indexes>(args).value()...);
    }
};

template <typename T>
struct Finalizer : FinalizerBase {
    using type = void (*)(T *);
    type func;
    void finalize(void *ptr) override {
        (*func)(reinterpret_cast<T *>(ptr));
    }
};

template <typename T, typename R, typename... ARGS>
struct InstanceMethod<R (T::*)(ARGS...)> : InstanceMethodBase {
    using type                          = R (T::*)(ARGS...);
    using return_type                   = R;
    using class_type                    = std::remove_cv_t<T>;
    constexpr static size_t ARG_N       = sizeof...(ARGS);
    constexpr static bool   RETURN_VOID = std::is_same<void, R>::value;

    type func = nullptr;

    template <typename... ARGS_HT, size_t... indexes>
    R callWithTuple(T *self, std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...> /*unused*/) const {
        return ((reinterpret_cast<T *>(self))->*func)(std::get<indexes>(args).value()...);
    }

    bool invoke(se::State &state) const override {
        constexpr auto indexes{std::make_index_sequence<sizeof...(ARGS)>()};
        T             *self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object    *thisObject = state.thisObject();
        const auto    &jsArgs     = state.args();
        if (ARG_N != jsArgs.size()) {
            SE_LOGE("incorret argument size %d, expect %d\n", static_cast<int>(jsArgs.size()), static_cast<int>(ARG_N));
            return false;
        }
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        convert_js_args_to_tuple(jsArgs, args, thisObject, indexes);
        if constexpr (RETURN_VOID) {
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
    constexpr static size_t ARG_N       = sizeof...(ARGS);
    constexpr static bool   RETURN_VOID = std::is_same<void, R>::value;

    type func = nullptr;

    template <typename... ARGS_HT, size_t... indexes>
    R callWithTuple(T *self, std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...> /*unused*/) const {
        return (*func)(reinterpret_cast<T *>(self), std::get<indexes>(args).value()...);
    }

    bool invoke(se::State &state) const override {
        constexpr auto indexes{std::make_index_sequence<sizeof...(ARGS)>()};
        T             *self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object    *thisObject = state.thisObject();
        const auto    &jsArgs     = state.args();
        if (ARG_N != jsArgs.size()) {
            SE_LOGE("incorret argument size %d, expect %d\n", jsArgs.size(), ARG_N);
            return false;
        }
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        convert_js_args_to_tuple(jsArgs, args, thisObject, indexes);
        if constexpr (RETURN_VOID) {
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
        for (auto *method : functions) {
                                         if (method->arg_count == -1 || method->arg_count == argCount) {
                                             ret = method->invoke(state);
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
        T          *self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *thisObject = state.thisObject();
        return nativevalue_to_se((self->*func), state.rval(), thisObject);
    }

    bool set(se::State &state) const override {
        T          *self       = reinterpret_cast<T *>(state.nativeThisObject());
        se::Object *thisObject = state.thisObject();
        const auto &args       = state.args();
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

template <typename T, typename R, typename F>
struct AccessorSet<R (T::*)(F)> {
    using class_type          = T;
    using type                = R (T::*)(F);
    using value_type          = F;
    using ignored_return_type = R;
};

template <typename T, typename R, typename F>
struct AccessorSet<R (*)(T *, F)> {
    using class_type          = T;
    using type                = R (*)(T *, F);
    using value_type          = F;
    using ignored_return_type = R;
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

    constexpr static bool HAS_GETTER          = !std::is_same_v<std::nullptr_t, getter_type>;
    constexpr static bool HAS_SETTER          = !std::is_same_v<std::nullptr_t, setter_type>;
    constexpr static bool GETTER_IS_MEMBER_FN = HAS_GETTER && std::is_member_function_pointer<Getter>::value;
    constexpr static bool SETTER_IS_MEMBER_FN = HAS_SETTER && std::is_member_function_pointer<Setter>::value;

    static_assert(!HAS_GETTER || std::is_base_of<getter_class_type, T>::value);
    static_assert(!HAS_SETTER || std::is_base_of<setter_class_type, T>::value);

    setter_type setterPtr;
    getter_type getterPtr;

    bool get(se::State &state) const override {
        if constexpr (HAS_GETTER) {
            T          *self       = reinterpret_cast<T *>(state.nativeThisObject());
            se::Object *thisObject = state.thisObject();
            if constexpr (GETTER_IS_MEMBER_FN) {
                return nativevalue_to_se((self->*getterPtr)(), state.rval(), thisObject);
            } else {
                return nativevalue_to_se((*getterPtr)(self), state.rval(), thisObject);
            }
        }
        return false;
    }

    bool set(se::State &state) const override {
        if constexpr (HAS_SETTER) {
            T                                                              *self       = reinterpret_cast<T *>(state.nativeThisObject());
            se::Object                                                     *thisObject = state.thisObject();
            const auto                                                     &args       = state.args();
            HolderType<set_value_type, std::is_reference_v<set_value_type>> temp;
            sevalue_to_native(args[0], &(temp.data), thisObject);
            if constexpr (SETTER_IS_MEMBER_FN) {
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
    constexpr static size_t ARG_N       = sizeof...(ARGS);
    constexpr static bool   RETURN_VOID = std::is_same<void, R>::value;

    type func = nullptr;

    template <typename... ARGS_HT, size_t... indexes>
    R callWithTuple(std::tuple<ARGS_HT...> &args, std::index_sequence<indexes...> /*unused*/) const {
        return (*func)(std::get<indexes>(args).value()...);
    }

    bool invoke(se::State &state) const override {
        constexpr auto indexes{std::make_index_sequence<sizeof...(ARGS)>()};
        const auto    &jsArgs = state.args();
        if (ARG_N != jsArgs.size()) {
            SE_LOGE("incorret argument size %d, expect %d\n", jsArgs.size(), ARG_N);
            return false;
        }
        std::tuple<HolderType<ARGS, std::is_reference_v<ARGS>>...> args{};
        convert_js_args_to_tuple(jsArgs, args, nullptr, indexes);
        if constexpr (RETURN_VOID) {
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
        for (auto *method : functions) {
                                       if (method->arg_count == -1 || method->arg_count == argCount) {
                                           ret = method->invoke(state);
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

template <typename R, typename F>
struct SAccessorSet<R (*)(F)> {
    using type                = R (*)(F);
    using value_type          = F;
    using ignored_return_type = R;
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

    constexpr static bool HAS_GETTER = !std::is_same_v<std::nullptr_t, getter_type>;
    constexpr static bool HAS_SETTER = !std::is_same_v<std::nullptr_t, setter_type>;

    static_assert(HAS_GETTER || HAS_SETTER);

    setter_type setterPtr;
    getter_type getterPtr;

    bool get(se::State &state) const override {
        if constexpr (HAS_GETTER) {
            return nativevalue_to_se((*getterPtr)(), state.rval(), nullptr);
        }
        return false;
    }

    bool set(se::State &state) const override {
        if constexpr (HAS_SETTER) {
            const auto                                                     &args = state.args();
            HolderType<set_value_type, std::is_reference_v<set_value_type>> temp;
            sevalue_to_native(args[0], &(temp.data), nullptr);
            (*setterPtr)(temp.value());
            return true;
        }
        return false;
    }
};

// NOLINTNEXTLINE
class context_db_ {
public:
    // NOLINTNEXTLINE
    struct context_ {
        std::vector<std::tuple<std::string, std::unique_ptr<InstanceAttributeBase>>> properties;
        std::vector<std::tuple<std::string, std::unique_ptr<InstanceFieldBase>>>     fields;
        std::vector<std::tuple<std::string, std::unique_ptr<InstanceMethodBase>>>    functions;
        std::vector<std::tuple<std::string, std::unique_ptr<StaticMethodBase>>>      staticFunctions;
        std::vector<std::tuple<std::string, std::unique_ptr<StaticAttributeBase>>>   staticProperties;
        std::vector<std::unique_ptr<ConstructorBase>>                                constructors;
        std::vector<std::unique_ptr<FinalizerBase>>                                  finalizeCallbacks;
        std::string                                                                  className;
        se::Class                                                                   *kls = nullptr;
        // se::Object *                                                  nsObject    = nullptr;
        se::Object *parentProto = nullptr;
    };
    inline context_ *operator[](const std::string &key) {
        return this->operator[](key.c_str());
    }
    context_           *operator[](const char *key);
    static context_db_ &instance();
    static void         reset();

private:
    std::map<std::string, context_ *> _contexts;
};

template <typename T>
struct FunctionWrapper;

template <typename R, typename... ARGS>
struct FunctionWrapper<R *(*)(ARGS...)> {
    using type                    = R *(*)(ARGS...);
    static constexpr size_t ARG_N = sizeof...(ARGS);
};

} // namespace intl
} // namespace sebind