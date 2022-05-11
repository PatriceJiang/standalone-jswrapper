#include "class.h"

namespace setpl {

context_db_::context_ *context_db_::operator[](const char *key) {
    auto it = _contexts.find(key);
    if (it == _contexts.end()) {
        auto *r = new context_;
        _contexts.emplace(key, r);
        return r;
    }
    return it->second;
}

context_db_ &context_db_::instance() {
    static context_db_ db;
    return db;
}

void context_db_::reset() {
    auto &i = instance();
    for (auto &it : i._contexts) {
        delete it.second;
    }
    i._contexts.clear();
}


void genericFunction(const v8::FunctionCallbackInfo<v8::Value> &_v8args) {
    void *              ctx    = _v8args.Data().IsEmpty() ? nullptr : _v8args.Data().As<v8::External>()->Value();
    InstanceMethodBase *method = reinterpret_cast<InstanceMethodBase *>(ctx);
    assert(ctx);
    bool                   ret      = false;
    v8::Isolate *          _isolate = _v8args.GetIsolate();
    v8::HandleScope        _hs(_isolate);
    se::ValueArray &       args = se::gValueArrayPool.get(_v8args.Length());
    se::CallbackDepthGuard depthGuard{args, se::gValueArrayPool._depth};
    se::internal::jsToSeArgs(_v8args, args);
    se::PrivateObjectBase *privateObject = static_cast<se::PrivateObjectBase *>(se::internal::getPrivate(_isolate, _v8args.This(), 0));
    se::Object *           thisObject    = reinterpret_cast<se::Object *>(se::internal::getPrivate(_isolate, _v8args.This(), 1));
    se::State              state(thisObject, privateObject, args);
    ret = method->invoke(state);
    if (!ret) {
        SE_LOGE("[ERROR] Failed to invoke %s, location: %s:%d\n", method->method_name.c_str(), __FILE__, __LINE__);
    }
    se::internal::setReturnValue(state.rval(), _v8args);
}


} // namespace setpl