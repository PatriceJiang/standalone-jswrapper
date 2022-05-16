#pragma once

#include "class.inl"

namespace sebind {

template <typename T>
void genericFinalizer(se::PrivateObjectBase *privateObject) {
    using context_type = typename class_<T>::context_;
    if (privateObject == nullptr) {
        return;
    }
    auto *scriptEngine = se::ScriptEngine::getInstance();
    scriptEngine->_setGarbageCollecting(true);
    auto *self    = reinterpret_cast<context_type *>(privateObject->finalizerData);
    auto *thisPtr = privateObject->get<T>();
    for (auto &fin : self->finalizeCallbacks) {
        fin->finalize(thisPtr);
    }
    scriptEngine->_setGarbageCollecting(false);
}

// function wrappers for v8 
template <typename T>
void genericConstructor(const v8::FunctionCallbackInfo<v8::Value> &v8args) {
    using context_type      = typename class_<T>::context_;
    v8::Isolate *   isolate = v8args.GetIsolate();
    v8::HandleScope handleScope(isolate);
    bool            ret = false;
    se::ValueArray  args;
    args.reserve(10);
    se::internal::jsToSeArgs(v8args, args);
    auto *      self       = reinterpret_cast<context_type *>(v8args.Data().IsEmpty() ? nullptr : v8args.Data().As<v8::External>()->Value());
    se::Object *thisObject = se::Object::_createJSObject(self->kls, v8args.This());
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

    se::Value propertyVal;
    bool      found = false;
    found           = thisObject->getProperty("_ctor", &propertyVal);
    if (found) propertyVal.toObject()->call(args, thisObject);
}

template <typename ContextType>
void genericAccessorSet(v8::Local<v8::Name> /*prop*/, v8::Local<v8::Value> jsVal,
                        const v8::PropertyCallbackInfo<void> &v8args) {
    auto *attr = reinterpret_cast<ContextType *>(v8args.Data().IsEmpty() ? nullptr : v8args.Data().As<v8::External>()->Value());
    assert(attr);
    v8::Isolate *          isolate = v8args.GetIsolate();
    v8::HandleScope        handleScope(isolate);
    bool                   ret           = true;
    auto *                 privateObject = static_cast<se::PrivateObjectBase *>(se::internal::getPrivate(isolate, v8args.This(), 0));
    auto *                 thisObject    = reinterpret_cast<se::Object *>(se::internal::getPrivate(isolate, v8args.This(), 1));
    se::ValueArray &       args          = se::gValueArrayPool.get(1);
    se::CallbackDepthGuard depthGuard{args, se::gValueArrayPool._depth};
    se::Value &            data{args[0]};
    se::internal::jsToSeValue(isolate, jsVal, &data);
    se::State state(thisObject, privateObject, args);
    ret = attr->set(state);
    if (!ret) {
        SE_LOGE("[ERROR] Failed to invoke set %s, location: %s:%d\n", attr->attr_name.c_str(), __FILE__, __LINE__);
    }
}
template <typename ContextType>
void genericAccessorGet(v8::Local<v8::Name> /*prop*/,
                        const v8::PropertyCallbackInfo<v8::Value> &v8args) {
    auto *attr = reinterpret_cast<ContextType *>(v8args.Data().IsEmpty() ? nullptr : v8args.Data().As<v8::External>()->Value());
    assert(attr);
    v8::Isolate *   isolate = v8args.GetIsolate();
    v8::HandleScope handleScope(isolate);
    bool            ret           = true;
    auto *          privateObject = static_cast<se::PrivateObjectBase *>(se::internal::getPrivate(isolate, v8args.This(), 0));
    auto *          thisObject    = reinterpret_cast<se::Object *>(se::internal::getPrivate(isolate, v8args.This(), 1));
    se::State       state(thisObject, privateObject);
    ret = attr->get(state);
    if (!ret) {
        SE_LOGE("[ERROR] Failed to invoke %s, location: %s:%d\n", attr->attr_name.c_str(), __FILE__, __LINE__);
    }
    se::internal::setReturnValue(state.rval(), v8args);
}

void genericFunction(const v8::FunctionCallbackInfo<v8::Value> &v8args);

template <typename T>
bool class_<T>::install(se::Object *nsObject) {
    constexpr auto *cstp = &genericConstructor<T>;
    assert(nsObject);
    _installed = true;

    if (_ctx->constructors.empty()) {
        if constexpr (std::is_default_constructible<T>::value) {
            constructor(); // add default constructor
        }
    }
    _ctx->kls = se::Class::create(_ctx->className, nsObject, _ctx->parentProto, cstp, _ctx);

    auto *getter = &genericAccessorGet<intl::InstanceAttributeBase>;
    auto *setter = &genericAccessorSet<intl::InstanceAttributeBase>;
    for (auto &attr : _ctx->properties) {
        _ctx->kls->defineProperty(std::get<0>(attr).c_str(), getter, setter, std::get<1>(attr).get());
    }
    auto *fieldGetter = &genericAccessorGet<intl::InstanceFieldBase>;
    auto *fieldSetter = &genericAccessorSet<intl::InstanceFieldBase>;
    for (auto &field : _ctx->fields) {
        _ctx->kls->defineProperty(std::get<0>(field).c_str(), fieldGetter, fieldSetter, std::get<1>(field).get());
    }
    // defineFunctions
    {
        std::map<std::string, std::vector<intl::InstanceMethodBase *>> multimap;
        for (auto &method : _ctx->functions) {
            multimap[std::get<0>(method)].emplace_back(std::get<1>(method).get());
        }
        for (auto &method : multimap) {
            if (method.second.size() > 1) {
                auto *overloaded        = new intl::InstanceMethodOverloaded;
                overloaded->class_name  = _ctx->className;
                overloaded->method_name = method.first;
                for (auto *method : method.second) {
                    overloaded->functions.push_back(method);
                }
                _ctx->kls->defineFunction(method.first.c_str(), &genericFunction, overloaded);
            } else {
                _ctx->kls->defineFunction(method.first.c_str(), &genericFunction, method.second[0]);
            }
        }
    }
    // define static functions
    {
        std::map<std::string, std::vector<intl::StaticMethodBase *>> multimap;
        for (auto &method : _ctx->staticFunctions) {
            multimap[std::get<0>(method)].emplace_back(std::get<1>(method).get());
        }
        for (auto &method : multimap) {
            if (method.second.size() > 1) {
                auto *overloaded        = new intl::StaticMethodOverloaded;
                overloaded->class_name  = _ctx->className;
                overloaded->method_name = method.first;
                for (auto *method : method.second) {
                    overloaded->functions.push_back(method);
                }
                _ctx->kls->defineStaticFunction(method.first.c_str(), &genericFunction, overloaded);
            } else {
                _ctx->kls->defineStaticFunction(method.first.c_str(), &genericFunction, method.second[0]);
            }
        }
    }
    for (auto &prop : _ctx->staticProperties) {
        _ctx->kls->defineStaticProperty(std::get<0>(prop).c_str(), fieldGetter, fieldSetter, std::get<1>(prop).get());
    }
    _ctx->kls->install();
    return true;
}
}  // namespace sebind
