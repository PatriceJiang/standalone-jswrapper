/****************************************************************************
 Copyright (c) 2010-2012 cc-x.org
 Copyright (c) 2011 Zynga Inc.
 Copyright (c) 2013-2016 Chukong Technologies Inc.
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

#import <Foundation/Foundation.h>

#include "FileUtils-apple.h"

#include <ftw.h>

#include <string>
#include <stack>

#include "base/Log.h"

namespace cc {

struct FileUtilsApple::IMPL {
    IMPL(NSBundle *bundle) : bundle_([NSBundle mainBundle]) {}
    void setBundle(NSBundle *bundle) {
        bundle_ = bundle;
    }
    NSBundle *getBundle() const {
        return bundle_;
    }

private:
    NSBundle *bundle_;
};

FileUtilsApple::FileUtilsApple() : pimpl_(new IMPL([NSBundle mainBundle])) {
}

FileUtilsApple::~FileUtilsApple() = default;

#if CC_FILEUTILS_APPLE_ENABLE_OBJC
void FileUtilsApple::setBundle(NSBundle *bundle) {
    pimpl_->setBundle(bundle);
}
#endif

#pragma mark - FileUtils

static NSFileManager *s_fileManager = [NSFileManager defaultManager];

FileUtils *FileUtils::getInstance() {
    if (FileUtils::sharedFileUtils == nullptr) {
        FileUtils::sharedFileUtils = new (std::nothrow) FileUtilsApple();
        if (!FileUtils::sharedFileUtils->init()) {
            delete FileUtils::sharedFileUtils;
            FileUtils::sharedFileUtils = nullptr;
            CC_LOG_DEBUG("ERROR: Could not init CCFileUtilsApple");
        }
    }
    return FileUtils::sharedFileUtils;
}

std::string FileUtilsApple::getWritablePath() const {
    if (_writablePath.length()) {
        return _writablePath;
    }

    // save to document folder
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    std::string strRet = [documentsDirectory UTF8String];
    strRet.append("/");
    return strRet;
}

bool FileUtilsApple::isFileExistInternal(const std::string &filePath) const {
    if (filePath.empty()) {
        return false;
    }

    bool ret = false;

    if (filePath[0] != '/') {
        std::string path;
        std::string file;
        size_t pos = filePath.find_last_of("/");
        if (pos != std::string::npos) {
            file = filePath.substr(pos + 1);
            path = filePath.substr(0, pos + 1);
        } else {
            file = filePath;
        }

        NSString *fullpath = [pimpl_->getBundle() pathForResource:[NSString stringWithUTF8String:file.c_str()]
                                                           ofType:nil
                                                      inDirectory:[NSString stringWithUTF8String:path.c_str()]];
        if (fullpath != nil) {
            ret = true;
        }
    } else {
        // Search path is an absolute path.
        if ([s_fileManager fileExistsAtPath:[NSString stringWithUTF8String:filePath.c_str()]]) {
            ret = true;
        }
    }

    return ret;
}

static int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    auto ret = remove(fpath);
    if (ret) {
        CC_LOG_INFO("Fail to remove: %s ", fpath);
    }

    return ret;
}

bool FileUtilsApple::removeDirectory(const std::string &path) {
    if (path.empty()) {
        CC_LOG_ERROR("Fail to remove directory, path is empty!");
        return false;
    }

    if (nftw(path.c_str(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS))
        return false;
    else
        return true;
}

std::string FileUtilsApple::getFullPathForDirectoryAndFilename(const std::string &directory, const std::string &filename) const {
    if (directory[0] != '/') {
        NSString *dirStr = [NSString stringWithUTF8String:directory.c_str()];
        // The following logic is used for remove the "../" in the directory
        // Because method "pathForResource" will return nil if the directory contains "../".
        auto theIdx = directory.find("..");
        if (theIdx != std::string::npos && theIdx > 0) {
            NSMutableArray<NSString *> *pathComps = [NSMutableArray arrayWithArray:[dirStr pathComponents]];
            NSUInteger idx = [pathComps indexOfObject:@".."];
            while (idx != NSNotFound && idx > 0) {       // if found ".." & it's not at the beginning of the string
                [pathComps removeObjectAtIndex:idx];     // remove the item ".."
                [pathComps removeObjectAtIndex:idx - 1]; // remove the item before ".."
                idx = [pathComps indexOfObject:@".."];   // find ".." again
            }
            dirStr = [NSString pathWithComponents:pathComps];
        }

        NSString *fullpath = [pimpl_->getBundle() pathForResource:[NSString stringWithUTF8String:filename.c_str()]
                                                           ofType:nil
                                                      inDirectory:dirStr];
        if (fullpath != nil) {
            return [fullpath UTF8String];
        }
    } else {
        std::string fullPath = directory + filename;
        // Search path is an absolute path.
        if ([s_fileManager fileExistsAtPath:[NSString stringWithUTF8String:fullPath.c_str()]]) {
            return fullPath;
        }
    }
    return "";
}

bool FileUtilsApple::createDirectory(const std::string &path) {
    CCASSERT(!path.empty(), "Invalid path");

    if (isDirectoryExist(path))
        return true;

    NSError *error;

    bool result = [s_fileManager createDirectoryAtPath:[NSString stringWithUTF8String:path.c_str()] withIntermediateDirectories:YES attributes:nil error:&error];

    if (!result && error != nil) {
        CC_LOG_ERROR("Fail to create directory \"%s\": %s", path.c_str(), [error.localizedDescription UTF8String]);
    }

    return result;
}

} // namespace cc
