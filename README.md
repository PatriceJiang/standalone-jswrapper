
## ScriptEngine 独立工程

最小化 ScriptEngine 依赖, 支持 Mac/Windows/Linux 平台. 

## 使用方法

#### CMake 配置

参考 [CMakeLists.txt](./CMakeLists.txt)

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/cocos/CMakeLists.txt)
# ...
target_link_libraries(demo ccbindings)
```

#### 配置

参考 [`demo/demo.ini`](./demo/demo.ini). 

其中占位符 `%(configdir)s` 为配置所在目录

#### 生成绑定

执行命令 
```
python tools/tojs/genbindings --config demo/demo.ini 
```



