
// clang-format off
#include "jsb_demo_auto.h"
#include "conversions/jsb_conversions.h"
#include "conversions/jsb_global.h"


#ifndef JSB_ALLOC
#define JSB_ALLOC(kls, ...) new (std::nothrow) kls(__VA_ARGS__)
#endif

#ifndef JSB_FREE
#define JSB_FREE(ptr) delete ptr
#endif
se::Object* __jsb_war_Tank_proto = nullptr; // NOLINT
se::Class* __jsb_war_Tank_class = nullptr;  // NOLINT

static bool js_war_Tank_fire(se::State& s) // NOLINT(readability-identifier-naming)
{
    auto* cobj = SE_THIS_OBJECT<war::Tank>(s);
    SE_PRECONDITION2(cobj, false, "js_war_Tank_fire : Invalid Native Object");
    const auto& args = s.args();
    size_t argc = args.size();
    CC_UNUSED bool ok = true;
    if (argc == 3) {
        HolderType<float, false> arg0 = {};
        HolderType<float, false> arg1 = {};
        HolderType<float, false> arg2 = {};
        ok &= sevalue_to_native(args[0], &arg0, s.thisObject());
        ok &= sevalue_to_native(args[1], &arg1, s.thisObject());
        ok &= sevalue_to_native(args[2], &arg2, s.thisObject());
        SE_PRECONDITION2(ok, false, "js_war_Tank_fire : Error processing arguments");
        cobj->fire(arg0.value(), arg1.value(), arg2.value());
        return true;
    }
    SE_REPORT_ERROR("wrong number of arguments: %d, was expecting %d", (int)argc, 3);
    return false;
}
SE_BIND_FUNC(js_war_Tank_fire)

SE_DECLARE_FINALIZE_FUNC(js_war_Tank_finalize)

static bool js_war_Tank_constructor(se::State& s) // NOLINT(readability-identifier-naming) constructor.c
{
    CC_UNUSED bool ok = true;
    const auto& args = s.args();
    std::string arg0;
    ok &= sevalue_to_native(args[0], &arg0, s.thisObject());
    SE_PRECONDITION2(ok, false, "js_war_Tank_constructor : Error processing arguments");
    auto *ptr = JSB_MAKE_PRIVATE_OBJECT(war::Tank, arg0);
    s.thisObject()->setPrivateObject(ptr);
    return true;
}
SE_BIND_CTOR(js_war_Tank_constructor, __jsb_war_Tank_class, js_war_Tank_finalize)

static bool js_war_Tank_finalize(se::State& s) // NOLINT(readability-identifier-naming)
{
    return true;
}
SE_BIND_FINALIZE_FUNC(js_war_Tank_finalize)

bool js_register_war_Tank(se::Object* obj) // NOLINT(readability-identifier-naming)
{
    auto* cls = se::Class::create("Tank", obj, nullptr, _SE(js_war_Tank_constructor));

    cls->defineFunction("fire", _SE(js_war_Tank_fire));
    cls->defineFinalizeFunction(_SE(js_war_Tank_finalize));
    cls->install();
    JSBClassType::registerClass<war::Tank>(cls);

    __jsb_war_Tank_proto = cls->getProto();
    __jsb_war_Tank_class = cls;


    se::ScriptEngine::getInstance()->clearException();
    return true;
}
bool register_all_war(se::Object* obj)    // NOLINT
{
    // Get the ns
    se::Value nsVal;
    if (!obj->getProperty("jswar", &nsVal, true))
    {
        se::HandleObject jsobj(se::Object::createPlainObject());
        nsVal.setObject(jsobj);
        obj->setProperty("jswar", nsVal);
    }
    se::Object* ns = nsVal.toObject();

    js_register_war_Tank(ns);
    return true;
}

// clang-format on