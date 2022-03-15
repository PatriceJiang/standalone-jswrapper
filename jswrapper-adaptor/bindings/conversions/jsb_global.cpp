/****************************************************************************
 Copyright (c) 2017-2022 Xiamen Yaji Software Co., Ltd.

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

#include "jsb_global.h"
#include "jsb_conversions.h"

#include "uv.h"

#include <chrono>
#include <regex>
#include <sstream>
#include "FileUtils.h"

using namespace cc; //NOLINT

namespace {

se::Object *                               __jsbObj = nullptr; //NOLINT
se::Object *                               __glObj  = nullptr; //NOLINT
std::unordered_map<std::string, se::Value> gModuleCache;

static bool require(se::State &s) { //NOLINT
    const auto &args = s.args();
    int         argc = static_cast<int>(args.size());
    assert(argc >= 1);
    assert(args[0].isString());

    return jsb_run_script(args[0].toString(), &s.rval());
}
SE_BIND_FUNC(require)

static bool doModuleRequire(const std::string &path, se::Value *ret, const std::string &prevScriptFileDir) { //NOLINT
    se::AutoHandleScope hs;
    assert(!path.empty());

    const auto &fileOperationDelegate = se::ScriptEngine::getInstance()->getFileOperationDelegate();
    assert(fileOperationDelegate.isValid());

    std::string fullPath;

    std::string pathWithSuffix = path;
    if (pathWithSuffix.rfind(".js") != (pathWithSuffix.length() - 3)) {
        pathWithSuffix += ".js";
    }
    std::string scriptBuffer = fileOperationDelegate.onGetStringFromFile(pathWithSuffix);

    if (scriptBuffer.empty() && !prevScriptFileDir.empty()) {
        std::string secondPath = prevScriptFileDir;
        if (secondPath[secondPath.length() - 1] != '/') {
            secondPath += "/";
        }

        secondPath += path;

        if (fileOperationDelegate.onCheckDirectoryExist(secondPath)) {
            if (secondPath[secondPath.length() - 1] != '/') {
                secondPath += "/";
            }
            secondPath += "index.js";
        } else {
            if (path.rfind(".js") != (path.length() - 3)) {
                secondPath += ".js";
            }
        }

        fullPath     = fileOperationDelegate.onGetFullPath(secondPath);
        scriptBuffer = fileOperationDelegate.onGetStringFromFile(fullPath);
    } else {
        fullPath = fileOperationDelegate.onGetFullPath(pathWithSuffix);
    }

    if (!scriptBuffer.empty()) {
        const auto &iter = gModuleCache.find(fullPath);
        if (iter != gModuleCache.end()) {
            *ret = iter->second;
            //                printf("Found cache: %s, value: %d\n", fullPath.c_str(), (int)ret->getType());
            return true;
        }
        std::string currentScriptFileDir = fileOperationDelegate.onGetFileDir(fullPath);

        // Add closure for evalutate the script
        char prefix[]    = "(function(currentScriptDir){ window.module = window.module || {}; var exports = window.module.exports = {}; ";
        char suffix[512] = {0};
        snprintf(suffix, sizeof(suffix), "\nwindow.module.exports = window.module.exports || exports;\n})('%s'); ", currentScriptFileDir.c_str());

        // Add current script path to require function invocation
        scriptBuffer = prefix + std::regex_replace(scriptBuffer, std::regex("([^A-Za-z0-9]|^)requireModule\\((.*?)\\)"), "$1requireModule($2, currentScriptDir)") + suffix;

        //            FILE* fp = fopen("/Users/james/Downloads/test.txt", "wb");
        //            fwrite(scriptBuffer.c_str(), scriptBuffer.length(), 1, fp);
        //            fclose(fp);

#if CC_PLATFORM == CC_PLATFORM_MAC_OSX || CC_PLATFORM == CC_PLATFORM_MAC_IOS
        std::string reletivePath = fullPath;
    #if CC_PLATFORM == CC_PLATFORM_MAC_OSX
        const std::string reletivePathKey = "/Contents/Resources";
    #else
        const std::string reletivePathKey = ".app";
    #endif

        size_t pos = reletivePath.find(reletivePathKey);
        if (pos != std::string::npos) {
            reletivePath = reletivePath.substr(pos + reletivePathKey.length() + 1);
        }
#else
        const std::string &reletivePath = fullPath;
#endif

        auto      se      = se::ScriptEngine::getInstance();
        bool      succeed = se->evalString(scriptBuffer.c_str(), scriptBuffer.length(), nullptr, reletivePath.c_str());
        se::Value moduleVal;
        if (succeed && se->getGlobalObject()->getProperty("module", &moduleVal) && moduleVal.isObject()) {
            se::Value exportsVal;
            if (moduleVal.toObject()->getProperty("exports", &exportsVal)) {
                if (ret != nullptr) {
                    *ret = exportsVal;
                }
                gModuleCache[fullPath] = std::move(exportsVal);
            } else {
                gModuleCache[fullPath] = se::Value::Undefined;
            }
            // clear module.exports
            moduleVal.toObject()->setProperty("exports", se::Value::Undefined);
        } else {
            gModuleCache[fullPath] = se::Value::Undefined;
        }
        assert(succeed);
        return succeed;
    }

    SE_LOGE("doModuleRequire %s, buffer is empty!\n", path.c_str());
    assert(false);
    return false;
}

static bool moduleRequire(se::State &s) { //NOLINT
    const auto &args = s.args();
    int         argc = static_cast<int>(args.size());
    assert(argc >= 2);
    assert(args[0].isString());
    assert(args[1].isString());

    return doModuleRequire(args[0].toString(), &s.rval(), args[1].toString());
}
SE_BIND_FUNC(moduleRequire)
} // namespace

bool jsb_run_script(const std::string &filePath, se::Value *rval /* = nullptr */) { //NOLINT
    se::AutoHandleScope hs;
    return se::ScriptEngine::getInstance()->runScript(filePath, rval);
}

bool jsb_run_script_module(const std::string &filePath, se::Value *rval /* = nullptr */) { //NOLINT
    return doModuleRequire(filePath, rval, "");
}

static bool jsc_garbageCollect(se::State &s) { //NOLINT
    se::ScriptEngine::getInstance()->garbageCollect();
    return true;
}
SE_BIND_FUNC(jsc_garbageCollect)

static bool getOrCreatePlainObject_r(const char *name, se::Object *parent, se::Object **outObj) { //NOLINT
    assert(parent != nullptr);
    assert(outObj != nullptr);
    se::Value tmp;

    if (parent->getProperty(name, &tmp) && tmp.isObject()) {
        *outObj = tmp.toObject();
        (*outObj)->incRef();
    } else {
        *outObj = se::Object::createPlainObject();
        parent->setProperty(name, se::Value(*outObj));
    }

    return true;
}

static bool js_performance_now(se::State &s) { //NOLINT
    auto now   = std::chrono::steady_clock::now();
    auto micro = std::chrono::duration_cast<std::chrono::microseconds>(now - se::ScriptEngine::getInstance()->getStartTime()).count();
    s.rval().setDouble(static_cast<double>(micro) * 0.001);
    return true;
}
SE_BIND_FUNC(js_performance_now)

static const char *BYTE_CODE_FILE_EXT = ".jsc"; //NOLINT

static std::string removeFileExt(const std::string &filePath) {
    size_t pos = filePath.rfind('.');
    if (0 < pos) {
        return filePath.substr(0, pos);
    }
    return filePath;
}

static int selectPort(int port) {
    struct sockaddr_in addr;
    static uv_tcp_t    server;
    uv_loop_t *        loop      = uv_loop_new();
    int                tryTimes  = 200;
    int                startPort = port;
    while (tryTimes-- > 0) {
        uv_tcp_init(loop, &server);
        uv_ip4_addr("0.0.0.0", startPort, &addr);
        uv_tcp_bind(&server, reinterpret_cast<const struct sockaddr *>(&addr), 0);
        int r = uv_listen(reinterpret_cast<uv_stream_t *>(&server), 5, nullptr);
        uv_close(reinterpret_cast<uv_handle_t *>(&server), nullptr);
        if (r) {
            SE_LOGD("Failed to listen port %d, error: %s. Try next port\n", startPort, uv_strerror(r));
            startPort += 1;
        } else {
            break;
        }
    }
    uv_loop_close(loop);
    return startPort;
}

void jsb_init_file_operation_delegate() { //NOLINT

    static se::ScriptEngine::FileOperationDelegate delegate;
    if (!delegate.isValid()) {
        delegate.onGetDataFromFile = [](const std::string &path, const std::function<void(const uint8_t *, size_t)> &readCallback) -> void {
            assert(!path.empty());

            Data fileData;

            //std::string byteCodePath = removeFileExt(path) + BYTE_CODE_FILE_EXT;
            //if (FileUtils::getInstance()->isFileExist(byteCodePath)) {
            //    fileData = FileUtils::getInstance()->getDataFromFile(byteCodePath);

            //    size_t   dataLen = 0;
            //    uint8_t *data    = xxtea_decrypt(fileData.getBytes(), static_cast<uint32_t>(fileData.getSize()),
            //                                  const_cast<unsigned char *>(xxteaKey.data()),
            //                                  static_cast<uint32_t>(xxteaKey.size()), reinterpret_cast<uint32_t *>(&dataLen));

            //    if (data == nullptr) {
            //        SE_REPORT_ERROR("Can't decrypt code for %s", byteCodePath.c_str());
            //        return;
            //    }

            //    if (ZipUtils::isGZipBuffer(data, dataLen)) {
            //        uint8_t *unpackedData;
            //        ssize_t  unpackedLen = ZipUtils::inflateMemory(data, dataLen, &unpackedData);

            //        if (unpackedData == nullptr) {
            //            SE_REPORT_ERROR("Can't decrypt code for %s", byteCodePath.c_str());
            //            return;
            //        }

            //        readCallback(unpackedData, unpackedLen);
            //        free(data);
            //        free(unpackedData);
            //    } else {
            //        readCallback(data, dataLen);
            //        free(data);
            //    }

            //    return;
            //}

            fileData = FileUtils::getInstance()->getDataFromFile(path);
            readCallback(fileData.getBytes(), fileData.getSize());
        };

        delegate.onGetStringFromFile = [](const std::string &path) -> std::string {
            assert(!path.empty());

            //std::string byteCodePath = removeFileExt(path) + BYTE_CODE_FILE_EXT;
            /*if (FileUtils::getInstance()->isFileExist(byteCodePath)) {
                Data fileData = FileUtils::getInstance()->getDataFromFile(byteCodePath);

                uint32_t dataLen;
                uint8_t *data = xxtea_decrypt(static_cast<uint8_t *>(fileData.getBytes()), static_cast<uint32_t>(fileData.getSize()),
                                              const_cast<unsigned char *>(xxteaKey.data()),
                                              static_cast<uint32_t>(xxteaKey.size()), &dataLen);

                if (data == nullptr) {
                    SE_REPORT_ERROR("Can't decrypt code for %s", byteCodePath.c_str());
                    return "";
                }

                if (ZipUtils::isGZipBuffer(data, dataLen)) {
                    uint8_t *unpackedData;
                    ssize_t  unpackedLen = ZipUtils::inflateMemory(data, dataLen, &unpackedData);
                    if (unpackedData == nullptr) {
                        SE_REPORT_ERROR("Can't decrypt code for %s", byteCodePath.c_str());
                        return "";
                    }

                    std::string ret(reinterpret_cast<const char *>(unpackedData), unpackedLen);
                    free(unpackedData);
                    free(data);

                    return ret;
                }
                std::string ret(reinterpret_cast<const char *>(data), dataLen);
                free(data);
                return ret;
            }*/

            if (FileUtils::getInstance()->isFileExist(path)) {
                return FileUtils::getInstance()->getStringFromFile(path);
            }
            SE_LOGE("ScriptEngine::onGetStringFromFile %s not found, possible missing file.\n", path.c_str());
            return "";
        };

        delegate.onGetFullPath = [](const std::string &path) -> std::string {
            assert(!path.empty());
            std::string byteCodePath = removeFileExt(path) + BYTE_CODE_FILE_EXT;
            if (FileUtils::getInstance()->isFileExist(byteCodePath)) {
                return FileUtils::getInstance()->fullPathForFilename(byteCodePath);
            }
            return FileUtils::getInstance()->fullPathForFilename(path);
        };

        delegate.onCheckFileExist = [](const std::string &path) -> bool {
            assert(!path.empty());
            return FileUtils::getInstance()->isFileExist(path);
        };

        delegate.onCheckDirectoryExist = [](const std::string &path) -> bool {
            assert(!path.empty());
            return FileUtils::getInstance()->isDirectoryExist(path);
        };

        delegate.onCreateDirectory = [](const std::string &path) -> bool {
            assert(!path.empty());
            return FileUtils::getInstance()->createDirectory(path);
        };

        delegate.onGetFileDir = [](const std::string &path) -> std::string {
            assert(!path.empty());
            return FileUtils::getInstance()->getFileDir(path);
        };

        delegate.onWriteFile = [](const std::string &content, const std::string &path) -> bool {
            return FileUtils::getInstance()->writeStringToFile(content, path);
        };

        assert(delegate.isValid());

        se::ScriptEngine::getInstance()->setFileOperationDelegate(delegate);
    }
}

bool jsb_enable_debugger(const std::string &debuggerServerAddr, uint32_t port, bool isWaitForConnect) { //NOLINT
    if (debuggerServerAddr.empty() || port == 0) {
        return false;
    }

    port = static_cast<uint32_t>(selectPort(static_cast<int>(port)));

    auto *se = se::ScriptEngine::getInstance();
    se->enableDebugger(debuggerServerAddr, port, isWaitForConnect);
    return true;
}

bool jsb_register_global_variables(se::Object *global) { //NOLINT

    global->defineFunction("require", _SE(require));
    global->defineFunction("requireModule", _SE(moduleRequire));

    getOrCreatePlainObject_r("jsb", global, &__jsbObj);

    auto glContextCls = se::Class::create("WebGLRenderingContext", global, nullptr, nullptr);
    glContextCls->install();

    __jsbObj->defineFunction("garbageCollect", _SE(jsc_garbageCollect));

    se::HandleObject performanceObj(se::Object::createPlainObject());
    performanceObj->defineFunction("now", _SE(js_performance_now));
    global->setProperty("performance", se::Value(performanceObj));

    se::ScriptEngine::getInstance()->clearException();

    se::ScriptEngine::getInstance()->addAfterCleanupHook([]() {
        gModuleCache.clear();

        SAFE_DEC_REF(__jsbObj);
        SAFE_DEC_REF(__glObj);
    });

    return true;
}
