
#include <cstdlib>
#include <sstream>
#include <string>
#include "FileUtils.h"
#include "conversions/jsb_global.h"
#include "jswrapper/SeApi.h"

#include "auto/jsb_demo_auto.h"

#if _WIN32
#include <direct.h> // _getcwd
#endif

int main(int argc, char **argv) {
    std::string scriptPath = "hello.js";

    if (argc > 1) {
        scriptPath = argv[1];
    }

    auto *engine = se::ScriptEngine::getInstance();
    auto *fu     = cc::FileUtils::getInstance();

    engine->addRegisterCallback(jsb_register_global_variables);
    engine->addRegisterCallback(register_all_war);

// TODO: replace search path
#if _WIN32
    char currentDir[256] = {0};
    _getcwd(currentDir, 255);
#else
    auto *currentDir = getenv("PWD");
#endif

    CC_LOG_DEBUG("current directory %s\n", currentDir);

    if (currentDir) {
        fu->addSearchPath(currentDir);
    }
    jsb_init_file_operation_delegate();

    engine->start();
    engine->evalString("console.log('begin execute')");
    auto ret = engine->runScript(scriptPath);
    engine->evalString("console.log('end')");
    if (!ret) return EXIT_FAILURE;

    se::ScriptEngine::destroyInstance();
    cc::FileUtils::destroyInstance();
    return 0;
}
