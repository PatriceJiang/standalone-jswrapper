#pragma once

#include "jswrapper/SeApi.h"
#include "conversions/jsb_conversions.h"
#include "Tank.h"

JSB_REGISTER_OBJECT_TYPE(war::Tank);
JSB_REGISTER_OBJECT_TYPE(war::Weapon);

bool js_register_war_Tank(se::Object *obj); // NOLINT
