#pragma once
#include <vector>
#include <iostream>

namespace Utils{
    class StringTools
    {
    public:
        // 根据给定词进行划分
        static void tokenize(const std::string& str,std::vector<std::string>& tokens,const std::string& delimiters = " "){
            std::string::size_type lastPos = str.find_first_not_of(delimiters,0);
            std::string::size_type pos = str.find_first_of(delimiters,lastPos);

            while (std::string::npos!=pos || std::string::npos!=lastPos)
            {
                tokens.push_back(str.substr(lastPos,pos-lastPos));
                lastPos = str.find_first_not_of(delimiters,pos);
                pos = str.find_first_of(delimiters,lastPos);
            }
        }
    };
};