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

#include <sstream>
#include "jsb_conversions.h"


///////////////////////// utils /////////////////////////

template <class... Fs>
struct overloaded;

template <class F0, class... Fs>
struct overloaded<F0, Fs...> : F0, overloaded<Fs...> {
    overloaded(F0 f0, Fs... rest) : F0(f0), overloaded<Fs...>(rest...) {} // NOLINT(google-explicit-constructor)

    using F0::               operator();
    using overloaded<Fs...>::operator();
};

template <class F0>
struct overloaded<F0> : F0 {
    overloaded(F0 f0) : F0(f0) {} // NOLINT(google-explicit-constructor)

    using F0::operator();
};

template <class... Fs>
auto make_overloaded(Fs... fs) { // NOLINT(readability-identifier-naming)
    return overloaded<Fs...>(fs...);
}

template <typename A, typename T, typename F>
typename std::enable_if<std::is_member_function_pointer<F>::value, bool>::type
set_member_field(se::Object *obj, T *to, const std::string &property, F f, se::Value &tmp) { // NOLINT
    bool ok = obj->getProperty(property.data(), &tmp, true);
    SE_PRECONDITION2(ok, false, "Property '%s' is not set", property.data());

    A m;
    ok = sevalue_to_native(tmp, &m, obj);
    SE_PRECONDITION2(ok, false, "Convert property '%s' failed", property.data());
    (to->*f)(m);
    return true;
}

template <typename A, typename T, typename F>
typename std::enable_if<std::is_member_object_pointer<F>::value, bool>::type
set_member_field(se::Object *obj, T *to, const std::string &property, F f, se::Value &tmp) { // NOLINT
    bool ok = obj->getProperty(property.data(), &tmp, true);
    SE_PRECONDITION2(ok, false, "Property '%s' is not set", property.data());

    ok = sevalue_to_native(tmp, &(to->*f), obj);
    SE_PRECONDITION2(ok, false, "Convert property '%s' failed", property.data());
    return true;
}

static bool isNumberString(const std::string &str) {
    for (const auto &c : str) { // NOLINT(readability-use-anyofallof) // remove after using c++20
        if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

namespace {

template <typename T>
bool std_vector_T_to_seval(const std::vector<T> &v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    assert(ret != nullptr);
    se::HandleObject obj(se::Object::createArrayObject(v.size()));
    bool             ok = true;

    uint32_t i = 0;
    for (const auto &value : v) {
        if (!obj->setArrayElement(i, se::Value(value))) {
            ok = false;
            ret->setUndefined();
            break;
        }
        ++i;
    }

    if (ok) {
        ret->setObject(obj);
    }

    return ok;
}

} // namespace

namespace {
enum class DataType {
    INT,
    FLOAT
};
bool uintptr_t_to_seval(uintptr_t v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    ret->setDouble(static_cast<double>(v));
    return true;
}

bool size_to_seval(size_t v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    ret->setSize(v);
    return true;
}

bool Vec2_to_seval(const cc::Vec2 &v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    assert(ret != nullptr);
    se::HandleObject obj(se::Object::createPlainObject());
    obj->setProperty("x", se::Value(v.x));
    obj->setProperty("y", se::Value(v.y));
    ret->setObject(obj);

    return true;
}

bool Vec3_to_seval(const cc::Vec3 &v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    assert(ret != nullptr);
    se::HandleObject obj(se::Object::createPlainObject());
    obj->setProperty("x", se::Value(v.x));
    obj->setProperty("y", se::Value(v.y));
    obj->setProperty("z", se::Value(v.z));
    ret->setObject(obj);

    return true;
}

bool Vec4_to_seval(const cc::Vec4 &v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    assert(ret != nullptr);
    se::HandleObject obj(se::Object::createPlainObject());
    obj->setProperty("x", se::Value(v.x));
    obj->setProperty("y", se::Value(v.y));
    obj->setProperty("z", se::Value(v.z));
    obj->setProperty("w", se::Value(v.w));
    ret->setObject(obj);

    return true;
}

bool Quaternion_to_seval(const cc::Quaternion &v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    assert(ret != nullptr);
    se::HandleObject obj(se::Object::createPlainObject());
    obj->setProperty("x", se::Value(v.x));
    obj->setProperty("y", se::Value(v.y));
    obj->setProperty("z", se::Value(v.z));
    obj->setProperty("w", se::Value(v.w));
    ret->setObject(obj);

    return true;
}

bool Mat4_to_seval(const cc::Mat4 &v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    assert(ret != nullptr);
    se::HandleObject obj(se::Object::createArrayObject(16));

    for (uint8_t i = 0; i < 16; ++i) {
        obj->setArrayElement(i, se::Value(v.m[i]));
    }

    ret->setObject(obj);
    return true;
}

bool Size_to_seval(const cc::Size &v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    assert(ret != nullptr);
    se::HandleObject obj(se::Object::createPlainObject());
    obj->setProperty("width", se::Value(v.width));
    obj->setProperty("height", se::Value(v.height));
    ret->setObject(obj);
    return true;
}

bool Rect_to_seval(const cc::Rect &v, se::Value *ret) { // NOLINT(readability-identifier-naming)
    assert(ret != nullptr);
    se::HandleObject obj(se::Object::createPlainObject());
    obj->setProperty("x", se::Value(v.origin.x));
    obj->setProperty("y", se::Value(v.origin.y));
    obj->setProperty("width", se::Value(v.size.width));
    obj->setProperty("height", se::Value(v.size.height));
    ret->setObject(obj);

    return true;
}
void toVec2(void *data, DataType type, se::Value *ret) {
    auto *   intptr   = static_cast<int32_t *>(data);
    auto *   floatptr = static_cast<float *>(data);
    cc::Vec2 vec2;
    if (DataType::INT == type) {
        vec2.x = static_cast<float>(intptr[0]);
        vec2.y = static_cast<float>(intptr[1]);
    } else {
        vec2.x = *floatptr;
        vec2.y = *(floatptr + 1);
    }

    Vec2_to_seval(vec2, ret);
}

void toVec3(void *data, DataType type, se::Value *ret) {
    auto *   intptr   = static_cast<int32_t *>(data);
    auto *   floatptr = static_cast<float *>(data);
    cc::Vec3 vec3;
    if (DataType::INT == type) {
        vec3.x = static_cast<float>(intptr[0]);
        vec3.y = static_cast<float>(intptr[1]);
        vec3.z = static_cast<float>(intptr[2]);
    } else {
        vec3.x = floatptr[0];
        vec3.y = floatptr[1];
        vec3.z = floatptr[2];
    }

    Vec3_to_seval(vec3, ret);
}

void toVec4(void *data, DataType type, se::Value *ret) {
    auto *   intptr   = static_cast<int32_t *>(data);
    auto *   floatptr = static_cast<float *>(data);
    cc::Vec4 vec4;
    if (DataType::INT == type) {
        vec4.x = static_cast<float>(intptr[0]);
        vec4.y = static_cast<float>(intptr[1]);
        vec4.z = static_cast<float>(intptr[2]);
        vec4.w = static_cast<float>(intptr[3]);
    } else {
        vec4.x = *floatptr;
        vec4.y = *(floatptr + 1);
        vec4.z = *(floatptr + 2);
        vec4.w = *(floatptr + 3);
    }

    Vec4_to_seval(vec4, ret);
}

void toMat(const float *data, int num, se::Value *ret) {
    se::HandleObject obj(se::Object::createPlainObject());

    char propName[4] = {0};
    for (int i = 0; i < num; ++i) {
        if (i < 10) {
            snprintf(propName, 3, "m0%d", i);
        }

        else {
            snprintf(propName, 3, "m%d", i);
        }

        obj->setProperty(propName, se::Value(*(data + i)));
    }
    ret->setObject(obj);
}
} // namespace

////////////////////////////////////////////////////////////////////////////
/////////////////sevalue to native//////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, cc::Vec4 *to, se::Object * /*unused*/) {
    SE_PRECONDITION2(from.isObject(), false, "Convert parameter to Vec4 failed!");
    se::Object *obj = from.toObject();
    se::Value   tmp;
    set_member_field<float>(obj, to, "x", &cc::Vec4::x, tmp);
    set_member_field<float>(obj, to, "y", &cc::Vec4::y, tmp);
    set_member_field<float>(obj, to, "z", &cc::Vec4::z, tmp);
    set_member_field<float>(obj, to, "w", &cc::Vec4::w, tmp);
    return true;
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, cc::Mat3 *to, se::Object * /*unused*/) {
    SE_PRECONDITION2(from.isObject(), false, "Convert parameter to Matrix3 failed!");
    se::Object *obj = from.toObject();

    if (obj->isTypedArray()) {
        // typed array
        SE_PRECONDITION2(obj->isTypedArray(), false, "Convert parameter to Matrix3 failed!");
        size_t   length = 0;
        uint8_t *ptr    = nullptr;
        obj->getTypedArrayData(&ptr, &length);

        memcpy(to->m, ptr, length);
    } else {
        bool        ok = false;
        se::Value   tmp;
        std::string prefix = "m";
        for (uint32_t i = 0; i < 9; ++i) {
            std::string name;
            if (i < 10) {
                name = prefix + "0" + std::to_string(i);
            } else {
                name = prefix + std::to_string(i);
            }
            ok = obj->getProperty(name.c_str(), &tmp, true);
            SE_PRECONDITION3(ok, false, *to = cc::Mat3::IDENTITY);

            if (tmp.isNumber()) {
                to->m[i] = tmp.toFloat();
            } else {
                SE_REPORT_ERROR("%u, not supported type in matrix", i);
                *to = cc::Mat3::IDENTITY;
                return false;
            }

            tmp.setUndefined();
        }
    }

    return true;
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, cc::Mat4 *to, se::Object * /*unused*/) {
    SE_PRECONDITION2(from.isObject(), false, "Convert parameter to Matrix4 failed!");
    se::Object *obj = from.toObject();

    if (obj->isTypedArray()) {
        // typed array
        SE_PRECONDITION2(obj->isTypedArray(), false, "Convert parameter to Matrix4 failed!");

        size_t   length = 0;
        uint8_t *ptr    = nullptr;
        obj->getTypedArrayData(&ptr, &length);

        memcpy(to->m, ptr, length);
    } else {
        bool        ok = false;
        se::Value   tmp;
        std::string prefix = "m";
        for (uint32_t i = 0; i < 16; ++i) {
            std::string name;
            if (i < 10) {
                name = prefix + "0" + std::to_string(i);
            } else {
                name = prefix + std::to_string(i);
            }
            ok = obj->getProperty(name.c_str(), &tmp, true);
            SE_PRECONDITION3(ok, false, *to = cc::Mat4::IDENTITY);

            if (tmp.isNumber()) {
                to->m[i] = tmp.toFloat();
            } else {
                SE_REPORT_ERROR("%u, not supported type in matrix", i);
                *to = cc::Mat4::IDENTITY;
                return false;
            }

            tmp.setUndefined();
        }
    }

    return true;
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, cc::Vec3 *to, se::Object * /*unused*/) {
    SE_PRECONDITION2(from.isObject(), false, "Convert parameter to Vec3 failed!");

    se::Object *obj = from.toObject();
    se::Value   tmp;
    set_member_field<float>(obj, to, "x", &cc::Vec3::x, tmp);
    set_member_field<float>(obj, to, "y", &cc::Vec3::y, tmp);
    set_member_field<float>(obj, to, "z", &cc::Vec3::z, tmp);
    return true;
}


// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, cc::Vec2 *to, se::Object * /*unused*/) {
    SE_PRECONDITION2(from.isObject(), false, "Convert parameter to Vec2 failed!");

    se::Object *obj = from.toObject();
    se::Value   tmp;
    set_member_field<float>(obj, to, "x", &cc::Vec2::x, tmp);
    set_member_field<float>(obj, to, "y", &cc::Vec2::y, tmp);
    return true;
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, cc::Size *to, se::Object * /*unused*/) {
    SE_PRECONDITION2(from.isObject(), false, "Convert parameter to Size failed!");

    se::Object *obj = from.toObject();
    se::Value   tmp;
    set_member_field<float>(obj, to, "width", &cc::Size::width, tmp);
    set_member_field<float>(obj, to, "height", &cc::Size::height, tmp);
    return true;
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, cc::Quaternion *to, se::Object * /*unused*/) {
    SE_PRECONDITION2(from.isObject(), false, "Convert parameter to Quaternion failed!");
    se::Object *obj = from.toObject();
    se::Value   tmp;
    set_member_field<float>(obj, to, "x", &cc::Quaternion::x, tmp);
    set_member_field<float>(obj, to, "y", &cc::Quaternion::y, tmp);
    set_member_field<float>(obj, to, "z", &cc::Quaternion::z, tmp);
    set_member_field<float>(obj, to, "w", &cc::Quaternion::w, tmp);
    return true;
}

//////////////////// geometry


////////////////////////// scene info


// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, std::variant<std::vector<float>, std::string> *to, se::Object * /*ctx*/) {
    if (from.isObject() && from.toObject()->isArray()) {
        uint32_t           len = 0;
        bool               ok  = from.toObject()->getArrayLength(&len);
        std::vector<float> arr;
        arr.resize(len);
        for (uint32_t i = 0; i < len; ++i) {
            se::Value e;
            ok = from.toObject()->getArrayElement(i, &e);
            if (ok) {
                if (e.isNumber()) {
                    arr[i] = e.toFloat();
                }
            }
        }
        *to = std::move(arr);
    } else if (from.isString()) {
        *to = from.toString();
    } else {
        CC_ASSERT(false);
    }
    return true;
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, std::vector<bool> *to, se::Object * /*ctx*/) {
    if (from.isNullOrUndefined()) {
        to->clear();
        return true;
    }

    se::Object *arr = from.toObject();
    uint32_t    size;
    se::Value   tmp;
    arr->getArrayLength(&size);
    to->resize(size);
    for (uint32_t i = 0; i < size; i++) {
        arr->getArrayElement(i, &tmp);
        (*to)[i] = tmp.toBoolean();
    }
    return true;
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool sevalue_to_native(const se::Value &from, std::vector<unsigned char> *to, se::Object * /*ctx*/) {
    if (from.isNullOrUndefined()) {
        to->clear();
        return true;
    }

    assert(from.isObject());
    se::Object *in = from.toObject();

    if (in->isTypedArray()) {
        uint8_t *data    = nullptr;
        size_t   dataLen = 0;
        in->getTypedArrayData(&data, &dataLen);
        to->resize(dataLen);
        to->assign(data, data + dataLen);
        return true;
    }

    if (in->isArrayBuffer()) {
        uint8_t *data    = nullptr;
        size_t   dataLen = 0;
        in->getArrayBufferData(&data, &dataLen);
        to->resize(dataLen);
        to->assign(data, data + dataLen);
        return true;
    }

    if (in->isArray()) {
        uint32_t len = 0;
        in->getArrayLength(&len);
        to->resize(len);
        se::Value ele;
        for (uint32_t i = 0; i < len; i++) {
            in->getArrayElement(i, &ele);
            (*to)[i] = ele.toUint8();
        }
        return true;
    }

    SE_LOGE("type error, ArrayBuffer/TypedArray/Array expected!");
    return false;
}

////////////////// custom types

// NOLINTNEXTLINE(readability-identifier-naming)
// NOLINTNEXTLINE(readability-identifier-naming)
bool nativevalue_to_se(const cc::Vec4 &from, se::Value &to, se::Object * /*unused*/) {
    return Vec4_to_seval(from, &to);
}

bool nativevalue_to_se(const cc::Vec2 &from, se::Value &to, se::Object * /*unused*/) {
    return Vec2_to_seval(from, &to);
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool nativevalue_to_se(const cc::Vec3 &from, se::Value &to, se::Object * /*unused*/) {
    return Vec3_to_seval(from, &to);
}


// NOLINTNEXTLINE(readability-identifier-naming)
bool nativevalue_to_se(const cc::Quaternion &from, se::Value &to, se::Object * /*ctx*/) {
    return Quaternion_to_seval(from, &to);
}

// NOLINTNEXTLINE(readability-identifier-naming)
bool nativevalue_to_se(const cc::Mat4 &from, se::Value &to, se::Object * /*ctx*/) {
    se::HandleObject obj(se::Object::createPlainObject());
    char             keybuf[8] = {0};
    for (auto i = 0; i < 16; i++) {
        snprintf(keybuf, sizeof(keybuf), "m%02d", i);
        obj->setProperty(keybuf, se::Value(from.m[i]));
    }
    to.setObject(obj);
    return true;
}