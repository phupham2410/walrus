#include "StringUtil.h"

using namespace std;

static string sPathDelim = "/\\";

string& StringUtil::LeftTrim(string &s)
{
    s.erase(s.begin(), find_if(s.begin(), s.end(), not1(ptr_fun<int, int>(isspace))));

    return s;
}

string& StringUtil::RightTrim(string& s)
{
    s.erase(find_if(s.rbegin(), s.rend(), not1(ptr_fun<int, int>(isspace))).base(), s.end());

    return s;
}

string& StringUtil::Trim(string& s)
{
    return LeftTrim(RightTrim(s));
}

void StringUtil::Split(const string& inputString, char delim, vector<string>& itemArray)
{
    itemArray.clear();

    string line;
    stringstream sstr(inputString);
    while(getline(sstr, line, delim))
    {
        itemArray.push_back(line);
    }
}

void StringUtil::HardSplit(const string& inputString, char delim, vector<string>& itemArray)
{
    itemArray.clear();

    string line;
    stringstream sstr(inputString);
    while(getline(sstr, line, delim))
    {
        if(0 == line.length()) continue;
        itemArray.push_back(Trim(line));
    }
}

// --------------------------------------------------------------------------------
// Handling FileName
// --------------------------------------------------------------------------------
void StringUtil::GetFileName(const string& absName, string& fileName)
{
    // Get fileName from absolute name

    // Search the last delimiter
    size_t delimpos = absName.find_last_of(sPathDelim);

    // Position after delimiter
    size_t filepos = (delimpos == string::npos) ? 0 : delimpos + 1;

    // Extract fileName
    fileName = absName.substr(filepos);
}

void StringUtil::GetPathName(const string& absName, string& pathName)
{
    // Get pathName from absolute name

    // Search the last delimiter
    size_t delimpos = absName.find_last_of(sPathDelim);
    
    // Extract pathName
    size_t pathcount = (delimpos == string::npos) ? 0 : delimpos;
    pathName = absName.substr(0, pathcount);
}

void StringUtil::GetBaseName(const string& absName, string& baseName)
{
    // Search the last delimiter
    size_t delimpos = absName.find_last_of(sPathDelim);

    // Position after delimiter
    size_t basepos = (delimpos == string::npos) ? 0 : delimpos + 1;

    // Search the last dot
    size_t dotpos = absName.find_last_of(".");

    // Extract baseName
    size_t basecount = ((dotpos == string::npos) ? absName.length() : dotpos) - basepos;
    baseName = absName.substr(basepos, basecount);
}

void StringUtil::GetExtName(const string& absName, string& extName)
{
    // Search the last delimiter
    size_t dotpos = absName.find_last_of(".");
    
    // Position after the dot
    size_t extpos = (dotpos == string::npos) ? string::npos : dotpos + 1;

    // Extract extName
    extName = absName.substr(extpos);
}

void StringUtil::GetAbsName (const string& pathName, const string& fileName, string& absName)
{
    absName = pathName + "/" + fileName;
}

void StringUtil::GetFileName(const string& baseName, const string& extName, string& fileName)
{
    fileName = baseName + "." + extName;
}
