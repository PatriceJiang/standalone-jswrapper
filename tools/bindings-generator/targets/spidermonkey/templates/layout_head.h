// clang-format off
#pragma once
#if $macro_judgement
$macro_judgement
#end if
\#include <type_traits>
\#include "jswrapper/SeApi.h"
\#include "conversions/jsb_conversions.h"
#if $hpp_headers
#for header in $hpp_headers
\#include "${header}"
#end for
#end if

bool register_all_${prefix}(se::Object *obj);                   // NOLINT

// placeholder for jsb_register_types
