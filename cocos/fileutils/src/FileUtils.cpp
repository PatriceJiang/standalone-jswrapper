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

#include "FileUtils.h"

#include <cstring>
#include <stack>

#include <cerrno>
#include <cstring>
#include <iostream>


#include <sys/stat.h>
#include <regex>

#include "base/Data.h"
#include "base/Log.h"

#include "tinydir.h"

namespace cc {

// Implement FileUtils
FileUtils *FileUtils::sharedFileUtils = nullptr;

void FileUtils::destroyInstance() {
    CC_SAFE_DELETE(FileUtils::sharedFileUtils);
}

void FileUtils::setDelegate(FileUtils *delegate) {
    delete FileUtils::sharedFileUtils;
    FileUtils::sharedFileUtils = delegate;
}

FileUtils::FileUtils() = default;

FileUtils::~FileUtils() = default;

bool FileUtils::writeStringToFile(const std::string &dataStr, const std::string &fullPath) {
    Data  data;
    char *dataP = const_cast<char *>(dataStr.data());
    data.fastSet(reinterpret_cast<unsigned char *>(dataP), dataStr.size());

    bool rv = writeDataToFile(data, fullPath);

    // need to give up buffer ownership for temp using, or double free will occur
    data.takeBuffer();
    return rv;
}

bool FileUtils::writeDataToFile(const Data &data, const std::string &fullPath) {
    size_t      size = 0;
    const char *mode = "wb";

    CCASSERT(!fullPath.empty() && data.getSize() != 0, "Invalid parameters.");

    auto *fileutils = FileUtils::getInstance();
    do {
        // Read the file from hardware
        FILE *fp = fopen(fileutils->getSuitableFOpen(fullPath).c_str(), mode);
        CC_BREAK_IF(!fp);
        size = data.getSize();

        fwrite(data.getBytes(), size, 1, fp);

        fclose(fp);

        return true;
    } while (false);

    return false;
}

bool FileUtils::init() {
    _searchPathArray.push_back(_defaultResRootPath);
    return true;
}

void FileUtils::purgeCachedEntries() {
    _fullPathCache.clear();
}

std::string FileUtils::getStringFromFile(const std::string &filename) {
    std::string s;
    getContents(filename, &s);
    return s;
}

Data FileUtils::getDataFromFile(const std::string &filename) {
    Data d;
    getContents(filename, &d);
    return d;
}

FileUtils::Status FileUtils::getContents(const std::string &filename, ResizableBuffer *buffer) {
    if (filename.empty()) {
        return Status::NOT_EXISTS;
    }

    auto *fs = FileUtils::getInstance();

    std::string fullPath = fs->fullPathForFilename(filename);
    if (fullPath.empty()) {
        return Status::NOT_EXISTS;
    }

    FILE *fp = fopen(fs->getSuitableFOpen(fullPath).c_str(), "rb");
    if (!fp) {
        return Status::OPEN_FAILED;
    }

#if defined(_MSC_VER)
    auto descriptor = _fileno(fp);
#else
    auto descriptor = fileno(fp);
#endif
    struct stat statBuf;
    if (fstat(descriptor, &statBuf) == -1) {
        fclose(fp);
        return Status::READ_FAILED;
    }
    auto size = static_cast<size_t>(statBuf.st_size);

    buffer->resize(size);
    size_t readsize = fread(buffer->buffer(), 1, size, fp);
    fclose(fp);

    if (readsize < size) {
        buffer->resize(readsize);
        return Status::READ_FAILED;
    }

    return Status::OK;
}

std::string FileUtils::getPathForFilename(const std::string &filename, const std::string &searchPath) const {
    std::string file{filename};
    std::string filePath;
    size_t      pos = filename.find_last_of('/');
    if (pos != std::string::npos) {
        filePath = filename.substr(0, pos + 1);
        file     = filename.substr(pos + 1);
    }

    // searchPath + file_path
    std::string path = searchPath;
    path.append(filePath);

    path = getFullPathForDirectoryAndFilename(path, file);

    return path;
}

std::string FileUtils::fullPathForFilename(const std::string &filename) const {
    if (filename.empty()) {
        return "";
    }

    if (isAbsolutePath(filename)) {
        return normalizePath(filename);
    }

    // Already Cached ?
    auto cacheIter = _fullPathCache.find(filename);
    if (cacheIter != _fullPathCache.end()) {
        return cacheIter->second;
    }

    std::string fullpath;

    for (const auto &searchIt : _searchPathArray) {
        fullpath = this->getPathForFilename(filename, searchIt);

        if (!fullpath.empty()) {
            // Using the filename passed in as key.
            _fullPathCache.insert(std::make_pair(filename, fullpath));
            return fullpath;
        }
    }

    // The file wasn't found, return empty string.
    return "";
}

std::string FileUtils::fullPathFromRelativeFile(const std::string &filename, const std::string &relativeFile) {
    return relativeFile.substr(0, relativeFile.rfind('/') + 1) + filename;
}

const std::vector<std::string> &FileUtils::getSearchPaths() const {
    return _searchPathArray;
}

const std::vector<std::string> &FileUtils::getOriginalSearchPaths() const {
    return _originalSearchPaths;
}

void FileUtils::setWritablePath(const std::string &writablePath) {
    _writablePath = writablePath;
}

const std::string &FileUtils::getDefaultResourceRootPath() const {
    return _defaultResRootPath;
}

void FileUtils::setDefaultResourceRootPath(const std::string &path) {
    if (_defaultResRootPath != path) {
        _fullPathCache.clear();
        _defaultResRootPath = path;
        if (!_defaultResRootPath.empty() && _defaultResRootPath[_defaultResRootPath.length() - 1] != '/') {
            _defaultResRootPath += '/';
        }

        // Updates search paths
        setSearchPaths(_originalSearchPaths);
    }
}

void FileUtils::setSearchPaths(const std::vector<std::string> &searchPaths) {
    bool existDefaultRootPath = false;
    _originalSearchPaths      = searchPaths;

    _fullPathCache.clear();
    _searchPathArray.clear();

    for (const auto &path : _originalSearchPaths) {
        std::string prefix;
        std::string fullPath;

        if (!isAbsolutePath(path)) { // Not an absolute path
            prefix = _defaultResRootPath;
        }
        fullPath = prefix + path;
        if (!path.empty() && path[path.length() - 1] != '/') {
            fullPath += "/";
        }
        if (!existDefaultRootPath && path == _defaultResRootPath) {
            existDefaultRootPath = true;
        }
        _searchPathArray.push_back(fullPath);
    }

    if (!existDefaultRootPath) {
        //CC_LOG_DEBUG("Default root path doesn't exist, adding it.");
        _searchPathArray.push_back(_defaultResRootPath);
    }
}

void FileUtils::addSearchPath(const std::string &searchpath, bool front) {
    std::string prefix;
    if (!isAbsolutePath(searchpath)) {
        prefix = _defaultResRootPath;
    }

    std::string path = prefix + searchpath;
    if (!path.empty() && path[path.length() - 1] != '/') {
        path += "/";
    }
    if (front) {
        _originalSearchPaths.insert(_originalSearchPaths.begin(), searchpath);
        _searchPathArray.insert(_searchPathArray.begin(), path);
    } else {
        _originalSearchPaths.push_back(searchpath);
        _searchPathArray.push_back(path);
    }
}

std::string FileUtils::getFullPathForDirectoryAndFilename(const std::string &directory, const std::string &filename) const {
    // get directory+filename, safely adding '/' as necessary
    std::string ret = directory;
    if (!directory.empty() && directory[directory.size() - 1] != '/') {
        ret += '/';
    }
    ret += filename;
    ret = normalizePath(ret);

    // if the file doesn't exist, return an empty string
    if (!isFileExistInternal(ret)) {
        ret = "";
    }
    return ret;
}

bool FileUtils::isFileExist(const std::string &filename) const {
    if (isAbsolutePath(filename)) {
        return isFileExistInternal(normalizePath(filename));
    }
    std::string fullpath = fullPathForFilename(filename);
    return !fullpath.empty();
}

bool FileUtils::isAbsolutePath(const std::string &path) const {
    return (path[0] == '/');
}

bool FileUtils::isDirectoryExist(const std::string &dirPath) const {
    CCASSERT(!dirPath.empty(), "Invalid path");

    if (isAbsolutePath(dirPath)) {
        return isDirectoryExistInternal(normalizePath(dirPath));
    }

    // Already Cached ?
    auto cacheIter = _fullPathCache.find(dirPath);
    if (cacheIter != _fullPathCache.end()) {
        return isDirectoryExistInternal(cacheIter->second);
    }

    std::string fullpath;
    for (const auto &searchIt : _searchPathArray) {
        // searchPath + file_path
        fullpath = fullPathForFilename(searchIt + dirPath);
        if (isDirectoryExistInternal(fullpath)) {
            _fullPathCache.insert(std::make_pair(dirPath, fullpath));
            return true;
        }
    }
    return false;
}

std::vector<std::string> FileUtils::listFiles(const std::string &dirPath) const {
    std::string              fullpath = fullPathForFilename(dirPath);
    std::vector<std::string> files;
    if (isDirectoryExist(fullpath)) {
        tinydir_dir dir;
#ifdef UNICODE
        unsigned int length = MultiByteToWideChar(CP_UTF8, 0, &fullpath[0], (int)fullpath.size(), NULL, 0);
        if (length != fullpath.size()) {
            return files;
        }
        std::wstring fullpathstr(length, 0);
        MultiByteToWideChar(CP_UTF8, 0, &fullpath[0], (int)fullpath.size(), &fullpathstr[0], length);
#else
        std::string fullpathstr = fullpath;
#endif
        if (tinydir_open(&dir, &fullpathstr[0]) != -1) {
            while (dir.has_next) {
                tinydir_file file;
                if (tinydir_readfile(&dir, &file) == -1) {
                    // Error getting file
                    break;
                }

#ifdef UNICODE
                std::wstring path = file.path;
                length            = WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)path.size(), NULL, 0, NULL, NULL);
                std::string filepath;
                if (length > 0) {
                    filepath.resize(length);
                    WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)path.size(), &filepath[0], length, NULL, NULL);
                }
#else
                std::string filepath = file.path;
#endif
                if (file.is_dir) {
                    filepath.append("/");
                }
                files.push_back(filepath);

                if (tinydir_next(&dir) == -1) {
                    // Error getting next file
                    break;
                }
            }
        }
        tinydir_close(&dir);
    }
    return files;
}

void FileUtils::listFilesRecursively(const std::string &dirPath, std::vector<std::string> *files) const { // NOLINT(misc-no-recursion)
    std::string fullpath = fullPathForFilename(dirPath);
    if (!fullpath.empty() && isDirectoryExist(fullpath)) {
        tinydir_dir dir;
#ifdef UNICODE
        unsigned int length = MultiByteToWideChar(CP_UTF8, 0, &fullpath[0], (int)fullpath.size(), NULL, 0);
        if (length != fullpath.size()) {
            return;
        }
        std::wstring fullpathstr(length, 0);
        MultiByteToWideChar(CP_UTF8, 0, &fullpath[0], (int)fullpath.size(), &fullpathstr[0], length);
#else
        std::string fullpathstr = fullpath;
#endif
        if (tinydir_open(&dir, &fullpathstr[0]) != -1) {
            while (dir.has_next) {
                tinydir_file file;
                if (tinydir_readfile(&dir, &file) == -1) {
                    // Error getting file
                    break;
                }

#ifdef UNICODE
                std::wstring path = file.path;
                length            = WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)path.size(), NULL, 0, NULL, NULL);
                std::string filepath;
                if (length > 0) {
                    filepath.resize(length);
                    WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)path.size(), &filepath[0], length, NULL, NULL);
                }
#else
                std::string filepath = file.path;
#endif
                if (file.name[0] != '.') {
                    if (file.is_dir) {
                        filepath.append("/");
                        files->push_back(filepath);
                        listFilesRecursively(filepath, files);
                    } else {
                        files->push_back(filepath);
                    }
                }

                if (tinydir_next(&dir) == -1) {
                    // Error getting next file
                    break;
                }
            }
        }
        tinydir_close(&dir);
    }
}

#if (CC_PLATFORM == CC_PLATFORM_WINDOWS) || (CC_PLATFORM == CC_PLATFORM_WINRT)
// windows os implement should override in platform specific FileUtiles class
bool FileUtils::isDirectoryExistInternal(const std::string &dirPath) const {
    CCASSERT(false, "FileUtils not support isDirectoryExistInternal");
    return false;
}

bool FileUtils::createDirectory(const std::string &path) {
    CCASSERT(false, "FileUtils not support createDirectory");
    return false;
}

bool FileUtils::removeDirectory(const std::string &path) {
    CCASSERT(false, "FileUtils not support removeDirectory");
    return false;
}

bool FileUtils::removeFile(const std::string &path) {
    CCASSERT(false, "FileUtils not support removeFile");
    return false;
}

bool FileUtils::renameFile(const std::string &oldfullpath, const std::string &newfullpath) {
    CCASSERT(false, "FileUtils not support renameFile");
    return false;
}

bool FileUtils::renameFile(const std::string &path, const std::string &oldname, const std::string &name) {
    CCASSERT(false, "FileUtils not support renameFile");
    return false;
}

std::string FileUtils::getSuitableFOpen(const std::string &filenameUtf8) const {
    CCASSERT(false, "getSuitableFOpen should be override by platform FileUtils");
    return filenameUtf8;
}

long FileUtils::getFileSize(const std::string &filepath) {
    CCASSERT(false, "getFileSize should be override by platform FileUtils");
    return 0;
}

#else
    // default implements for unix like os
    #include <dirent.h>
    #include <sys/types.h>
    #include <cerrno>

    // android doesn't have ftw.h
    #if (CC_PLATFORM != CC_PLATFORM_ANDROID)
        #include <ftw.h>
    #endif

bool FileUtils::isDirectoryExistInternal(const std::string &dirPath) const {
    struct stat st;
    if (stat(dirPath.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

bool FileUtils::createDirectory(const std::string &path) {
    CCASSERT(!path.empty(), "Invalid path");

    if (isDirectoryExist(path)) {
        return true;
    }

    // Split the path
    size_t                   start = 0;
    size_t                   found = path.find_first_of("/\\", start);
    std::string              subpath;
    std::vector<std::string> dirs;

    if (found != std::string::npos) {
        while (true) {
            subpath = path.substr(start, found - start + 1);
            if (!subpath.empty()) {
                dirs.push_back(subpath);
            }
            start = found + 1;
            found = path.find_first_of("/\\", start);
            if (found == std::string::npos) {
                if (start < path.length()) {
                    dirs.push_back(path.substr(start));
                }
                break;
            }
        }
    }

    DIR *dir = nullptr;

    // Create path recursively
    subpath = "";
    for (const auto &iter : dirs) {
        subpath += iter;
        dir = opendir(subpath.c_str());

        if (!dir) {
            // directory doesn't exist, should create a new one

            int ret = mkdir(subpath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
            if (ret != 0 && (errno != EEXIST)) {
                // current directory can not be created, sub directories can not be created too
                // should return
                return false;
            }
        } else {
            // directory exists, should close opened dir
            closedir(dir);
        }
    }
    return true;
}

namespace {
    #if (CC_PLATFORM != CC_PLATFORM_ANDROID)
int unlinkCb(const char *fpath, const struct stat * /*sb*/, int /*typeflag*/, struct FTW * /*ftwbuf*/) {
    int rv = remove(fpath);

    if (rv) {
        perror(fpath);
    }

    return rv;
}
    #endif
} // namespace

bool FileUtils::removeDirectory(const std::string &path) {
    #if !defined(CC_TARGET_OS_TVOS)

        #if (CC_PLATFORM != CC_PLATFORM_ANDROID)
    return nftw(path.c_str(), unlinkCb, 64, FTW_DEPTH | FTW_PHYS) != -1;
        #else
    std::string command = "rm -r ";
    // Path may include space.
    command += "\"" + path + "\"";

    return (system(command.c_str()) >= 0);
        #endif // (CC_PLATFORM != CC_PLATFORM_ANDROID)

    #else
    return false;
    #endif // !defined(CC_TARGET_OS_TVOS)
}

bool FileUtils::removeFile(const std::string &path) {
    return remove(path.c_str()) == 0;
}

bool FileUtils::renameFile(const std::string &oldfullpath, const std::string &newfullpath) {
    CCASSERT(!oldfullpath.empty(), "Invalid path");
    CCASSERT(!newfullpath.empty(), "Invalid path");

    int errorCode = rename(oldfullpath.c_str(), newfullpath.c_str());

    if (0 != errorCode) {
        CC_LOG_ERROR("Fail to rename file %s to %s !Error code is %d", oldfullpath.c_str(), newfullpath.c_str(), errorCode);
        return false;
    }
    return true;
}

bool FileUtils::renameFile(const std::string &path, const std::string &oldname, const std::string &name) {
    CCASSERT(!path.empty(), "Invalid path");
    std::string oldPath = path + oldname;
    std::string newPath = path + name;

    return this->renameFile(oldPath, newPath);
}

std::string FileUtils::getSuitableFOpen(const std::string &filenameUtf8) const {
    return filenameUtf8;
}

long FileUtils::getFileSize(const std::string &filepath) { //NOLINT(google-runtime-int)
    CCASSERT(!filepath.empty(), "Invalid path");

    std::string fullpath{filepath};
    if (!isAbsolutePath(filepath)) {
        fullpath = fullPathForFilename(filepath);
        if (fullpath.empty()) {
            return 0;
        }
    }

    struct stat info;
    // Get data associated with "crt_stat.c":
    int result = stat(fullpath.c_str(), &info);

    // Check if statistics are valid:
    if (result != 0) {
        // Failed
        return -1;
    }
    return static_cast<long>(info.st_size); //NOLINT(google-runtime-int)
}
#endif

std::string FileUtils::getFileExtension(const std::string &filePath) const {
    std::string fileExtension;
    size_t      pos = filePath.find_last_of('.');
    if (pos != std::string::npos) {
        fileExtension = filePath.substr(pos, filePath.length());

        std::transform(fileExtension.begin(), fileExtension.end(), fileExtension.begin(), ::tolower);
    }

    return fileExtension;
}

std::string FileUtils::getFileDir(const std::string &path) {
    std::string ret;
    size_t      pos = path.rfind('/');
    if (pos != std::string::npos) {
        ret = path.substr(0, pos);
    }

    normalizePath(ret);

    return ret;
}

std::string FileUtils::normalizePath(const std::string &path) {
    std::string ret;
    // Normalize: remove . and ..
    ret = std::regex_replace(path, std::regex("/\\./"), "/");
    ret = std::regex_replace(ret, std::regex("/\\.$"), "");

    size_t pos;
    while ((pos = ret.find("..")) != std::string::npos && pos > 2) {
        size_t prevSlash = ret.rfind('/', pos - 2);
        if (prevSlash == std::string::npos) {
            break;
        }

        ret = ret.replace(prevSlash, pos - prevSlash + 2, "");
    }
    return ret;
}

} // namespace cc
