

![workflow result](https://github.com/PatriceJiang/standalone-jswrapper/actions/workflows/compile-platforms.yml/badge.svg)

## ScriptEngine 独立工程

最小化 ScriptEngine 依赖, 支持 Mac/Windows/Linux 平台. 

目录结构

- `cocos/` 为 `jswrapper` 依赖和代码, 需要和引擎同步更新.
- `jswrapper-adaptor/` 为 demo 工程的依赖, 包括转换函数的定义 和 FS 接口. 由用户工程维护.


## 使用方法

拷贝 `cocos/` `jswrapper-adaptor/` 和 `tools/` 目录到目标工程, 根据需要定制 `jswrapper-adaptor/`.

#### CMake 配置

参考 [CMakeLists.txt](./CMakeLists.txt)

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/cocos/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/jswrapper-adaptor/CMakeLists.txt)
# ...
target_link_libraries(demo ccbindings)
```

#### `.ini` 配置

参考 [`demo/demo.ini`](./demo/demo.ini). 

其中占位符 `%(configdir)s` 为配置所在目录

#### 生成绑定

执行命令 
```
python tools/tojs/genbindings --config demo/demo.ini 
```



