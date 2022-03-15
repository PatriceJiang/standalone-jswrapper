/****************************************************************************
 Copyright (c) 2021 Xiamen Yaji Software Co., Ltd.
 http://www.cocos.com
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated engine source code (the "Software"), a limited,
 worldwide, royalty-free, non-assignable, revocable and non-exclusive license
 to use Cocos Creator solely to develop games on your target platforms. You shall
 not use Cocos Creator software for developing other software or tools that's
 used for developing games. You are not granted to publish, distribute,
 sublicense, and/or sell copies of Cocos Creator.
 The software or tools in this License Agreement are licensed, not sold.
 Xiamen Yaji Software Co., Ltd. reserves all rights not expressly granted to you.
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
****************************************************************************/

#pragma once

#include <unordered_map>
#include <vector>
#include "base/Ptr.h"
#include "jswrapper/SeApi.h"

#include <optional>
#include <variant>

#include "math/Geometry.h"
#include "math/Mat3.h"
#include "math/Mat4.h"
#include "math/Quaternion.h"
#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Vec4.h"

////////////////////////////////////////////////////////////////////////////
/////////////////sevalue to native/////////////////////////////
////////////////////////////////////////////////////////////////////////////

inline bool sevalue_to_native(const se::Value &from, std::string *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    if (!from.isNullOrUndefined()) {
        *to = from.toString();
    } else {
        to->clear();
    }
    return true;
}

///// integers
inline bool sevalue_to_native(const se::Value &from, bool *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.isNullOrUndefined() ? false : (from.isNumber() ? from.toDouble() != 0 : from.toBoolean());
    return true;
}

inline bool sevalue_to_native(const se::Value &from, int32_t *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toInt32();
    return true;
}
inline bool sevalue_to_native(const se::Value &from, uint32_t *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toUint32();
    return true;
}

inline bool sevalue_to_native(const se::Value &from, int16_t *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toInt16();
    return true;
}
inline bool sevalue_to_native(const se::Value &from, uint16_t *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toUint16();
    return true;
}

inline bool sevalue_to_native(const se::Value &from, int8_t *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toInt8();
    return true;
}
inline bool sevalue_to_native(const se::Value &from, uint8_t *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toUint8();
    return true;
}

inline bool sevalue_to_native(const se::Value &from, uint64_t *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toUint64();
    return true;
}

inline bool sevalue_to_native(const se::Value &from, int64_t *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toInt64();
    return true;
}

#if CC_PLATFORM == CC_PLATFORM_MAC_IOS || CC_PLATFORM == CC_PLATFORM_MAC_OSX
inline bool sevalue_to_native(const se::Value &from, unsigned long *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    // on mac: unsiged long  === uintptr_t
    static_assert(sizeof(*to) == 8, "");
    *to = static_cast<unsigned long>(from.toUint64());
    return true;
}
#endif

inline bool sevalue_to_native(const se::Value &from, float *to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    *to = from.toFloat();
    return true;
}
inline bool sevalue_to_native(const se::Value &from, double *to, se::Object * /*unused*/) { // NOLINT(readability-identifier-naming)
    *to = from.toDouble();
    return true;
}

inline bool sevalue_to_native(const se::Value &from, se::Value *to, se::Object * /*unused*/) { // NOLINT(readability-identifier-naming)
    *to = from;
    return true;
}

bool sevalue_to_native(const se::Value &from, cc::Vec4 *to, se::Object * /*unused*/); // NOLINT(readability-identifier-naming)

bool sevalue_to_native(const se::Value &from, cc::Mat3 *to, se::Object * /*unused*/); // NOLINT(readability-identifier-naming)

bool sevalue_to_native(const se::Value &from, cc::Mat4 *to, se::Object * /*unused*/); // NOLINT(readability-identifier-naming)

bool sevalue_to_native(const se::Value &from, cc::Vec3 *to, se::Object * /*unused*/); // NOLINT(readability-identifier-naming)

bool sevalue_to_native(const se::Value &from, cc::Vec2 *to, se::Object * /*unused*/); // NOLINT(readability-identifier-naming)

bool sevalue_to_native(const se::Value &from, cc::Size *to, se::Object * /*unused*/); // NOLINT(readability-identifier-naming)

bool sevalue_to_native(const se::Value &from, cc::Quaternion *to, se::Object * /*unused*/); // NOLINT(readability-identifier-naming)

inline bool sevalue_to_native(const se::Value &from, std::vector<se::Value> *to, se::Object * /*unused*/) { // NOLINT(readability-identifier-naming)
    if (from.isNullOrUndefined()) {
        to->clear();
        return true;
    }
    assert(from.isObject() && from.toObject()->isArray());
    auto *array = from.toObject();
    to->clear();
    uint32_t size;
    array->getArrayLength(&size);
    for (uint32_t i = 0; i < size; i++) {
        se::Value ele;
        array->getArrayElement(i, &ele);
        to->emplace_back(ele);
    }
    return true;
}

inline bool sevalue_to_native(const se::Value &from, void **to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    assert(to != nullptr);
    if (from.isNumber() || from.isBigInt()) {
        // NOLINTNEXTLINE(performance-no-int-to-ptr)
        *to = reinterpret_cast<void *>(from.toUint64());
        return true;
    }
    if (from.isObject()) {
        *to = from.toObject()->getPrivateData();
        return true;
    }
    SE_LOGE("[warn] failed to convert to void *\n");
    return false;
}

inline bool sevalue_to_native(const se::Value &from, std::string **to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    **to = from.toString();
    return true;
}

bool sevalue_to_native(const se::Value &from, std::vector<bool> *to, se::Object * /*ctx*/); // NOLINT(readability-identifier-naming)

bool sevalue_to_native(const se::Value &from, std::vector<unsigned char> *to, se::Object * /*ctx*/); // NOLINT(readability-identifier-naming)

////////////////////////////////////////////////////////////////////////////
////////////////////nativevalue to se /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

inline bool nativevalue_to_se(const std::vector<int8_t> &from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    se::Object *array = se::Object::createTypedArray(se::Object::TypedArrayType::INT8, from.data(), from.size());
    to.setObject(array);
    array->decRef();
    return true;
}

inline bool nativevalue_to_se(const std::vector<uint8_t> &from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    se::Object *array = se::Object::createTypedArray(se::Object::TypedArrayType::UINT8, from.data(), from.size());
    to.setObject(array);
    array->decRef();
    return true;
}

inline bool nativevalue_to_se(int64_t from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setInt64(from);
    return true;
}

inline bool nativevalue_to_se(uint64_t from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setUint64(from);
    return true;
}

inline bool nativevalue_to_se(int32_t from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setInt32(from);
    return true;
}

inline bool nativevalue_to_se(uint32_t from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setUint32(from);
    return true;
}
inline bool nativevalue_to_se(int16_t from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setInt16(from);
    return true;
}
inline bool nativevalue_to_se(uint16_t from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setUint16(from);
    return true;
}

inline bool nativevalue_to_se(int8_t from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setInt8(from);
    return true;
}

inline bool nativevalue_to_se(uint8_t from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setUint8(from);
    return true;
}

inline bool nativevalue_to_se(float from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setFloat(from);
    return true;
}
inline bool nativevalue_to_se(double from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setDouble(from);
    return true;
}
inline bool nativevalue_to_se(bool from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setBoolean(from);
    return true;
}

inline bool nativevalue_to_se(const std::string &from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    to.setString(from);
    return true;
}
// template <>
// bool nativevalue_to_se(const cc::Color &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

bool nativevalue_to_se(const cc::Mat3 &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

bool nativevalue_to_se(const cc::Mat4 &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

// JSB_REGISTER_OBJECT_TYPE(cc::network::DownloaderHints);

bool nativevalue_to_se(const cc::Vec2 &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

bool nativevalue_to_se(const cc::Vec3 &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

bool nativevalue_to_se(const cc::Vec4 &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

bool nativevalue_to_se(const cc::Size &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

bool nativevalue_to_se(const cc::Quaternion &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

bool nativevalue_to_se(const cc::Rect &from, se::Value &to, se::Object *ctx); // NOLINT(readability-identifier-naming)

using void_p = void *;
inline bool nativevalue_to_se(const void_p &from, se::Value &to, se::Object * /*ctx*/) { // NOLINT(readability-identifier-naming)
    if (!from) {
        to.setUndefined();
    } else {
        auto ptr = reinterpret_cast<uintptr_t>(from);
        sizeof(from) == 8 ? to.setUint64(static_cast<uint64_t>(ptr)) : to.setUint32(static_cast<uint32_t>(ptr));
    }
    return true;
}
