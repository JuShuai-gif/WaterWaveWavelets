#pragma once
#include "StringTools.h"
#include "md5/md5.h"
#include <sys/stat.h>
#include <algorithm>
/*该头文件是用来获取有关文件的信息*/
#ifdef WIN32
#include <direct.h>
#define NOMINMAX
#include "windows.h"
#include <commdlg.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
#endif

namespace Utils{
    class FileSystem
    {
    public:
        static std::string getFilePath(const std::string& path)
        {
            std::string npath = 
        }

        static std::string normalizePath(const std::string& path)
        {
            if(path.size() == 0)
                return path;
            std::string result = path;
            std::replace(result.begin(),result.end(),'\\','/');
            std::vector<std::string> tokens;
            StringTools::tokenize(result,tokens,"/");
            unsigned int index = 0;
            while (index < tokens.size())
            {
                if ((tokens[index] == "..")&& (index > 0))
                {
                    tokens.erase(tokens.begin() + index - 1,tokens.begin() + index +1);
                    index -= 2;
                }
                index++;
            }
            result = "";
            if (path[0] == '/')
                result = "/";
            
            
            

        }
    };
    

    

}