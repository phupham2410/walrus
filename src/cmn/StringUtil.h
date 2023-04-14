#ifndef __STRINGUTIL_H__
#define __STRINGUTIL_H__

#include "StdHeader.h"

class StringUtil
{
public:
    struct sUtilData
    {
        bool m_Initialized;

        std::string m_PathDelimiter;
    };

public:
    static void SystemInit();

public:
    // String processing
    static std::string& LeftTrim(std::string& inputString);
    static std::string& RightTrim(std::string& inputString);
    static std::string& Trim(std::string& inputString);
    
    static void Split(const std::string& inputString, char delim, std::vector<std::string>& itemArray);
    static void HardSplit(const std::string& inputString, char delim, std::vector<std::string>& itemArray);

public:
    // Group of functions for handling FileName
    static void GetFileName(const std::string& absName, std::string& fileName);
    static void GetPathName(const std::string& absName, std::string& pathName); // Exclude last delimiter
    static void GetBaseName(const std::string& absName, std::string& baseName);
    static void GetExtName (const std::string& absName, std::string& extName ); // Exclude "dot"

    static void GetAbsName (const std::string& pathName, const std::string& fileName, std::string& absName);
    static void GetFileName(const std::string& baseName, const std::string& extName, std::string& fileName);
};

#endif //__STRINGUTIL_H__
