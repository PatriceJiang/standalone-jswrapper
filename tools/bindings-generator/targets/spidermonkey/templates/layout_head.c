
// clang-format off
\#include "${$out_file + '.h'}"
#if $macro_judgement
$macro_judgement
#end if
\#include "conversions/jsb_conversions.h"
\#include "conversions/jsb_global.h"

#if $cpp_headers
#for header in $cpp_headers
\#include "${header}"
#end for
#end if

#ifndef JSB_ALLOC
#define JSB_ALLOC(kls, ...) new (std::nothrow) kls(__VA_ARGS__)
#endif

#ifndef JSB_FREE
#define JSB_FREE(ptr) delete ptr
#endif
