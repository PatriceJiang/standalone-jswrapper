// clang-format off
#pragma once
#include <type_traits>
#include "jswrapper/SeApi.h"
#include "conversions/jsb_conversions.h"
#include "../Tank.h"

bool register_all_war(se::Object *obj);                   // NOLINT

JSB_REGISTER_OBJECT_TYPE(war::Tank);


extern se::Object *__jsb_war_Tank_proto; // NOLINT
extern se::Class * __jsb_war_Tank_class; // NOLINT

bool js_register_war_Tank(se::Object *obj); // NOLINT

SE_DECLARE_FUNC(js_war_Tank_fire);
SE_DECLARE_FUNC(js_war_Tank_Tank);
    // clang-format on