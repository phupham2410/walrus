#ifndef __COREUTIL_H__
#define __COREUTIL_H__

#include "StdMacro.h"
#include "StdHeader.h"

enum eAttributeID
{
    ATTR_MAX_ERASE = 0xA5,
    ATTR_AVE_ERASE = 0xA7,
    ATTR_TEMPERATURE = 0xC2,
    ATTR_LBA_WRITTEN = 0xF1,
    ATTR_LBA_READ = 0xF2,
    ATTR_LIFE_LEFT = 0xF8,
    ATTR_SPARE_BLOCK = 0xF9,
    ATTR_ECC_ERROR = 0xC3,
    ATTR_CRC_ERROR = 0xC7,
    ATTR_PROGRAM_FAIL = 0xB5,
    ATTR_ERASE_FAIL = 0xB6,
    ATTR_NAND_ENDURANCE = 0xA8,
};

enum eIDENTKEY
{
    #define MAP_ITEM(code, name) code,
    #include "IdentifyKey.def"
    #undef MAP_ITEM
};

class cCoreUtil
{
public:    
    struct sATTRPARAM
    {
        bool Show;
        std::string Name;
        std::string Note;
        sATTRPARAM() {}
        sATTRPARAM(const std::string& name, const std::string& note, bool show) {Name = name; Note = note; Show = show; }
    };

    typedef std::map<U8, sATTRPARAM> tATTRNAMEMAP;
    typedef std::map<std::string, eIDENTKEY> tIDENTKEYMAP;

    struct sUtilData
    {
        bool m_ShowAttributeName;
        tATTRNAMEMAP m_AttrNameMap;
        tIDENTKEYMAP m_IdentifyKeyMap;
        
        sUtilData();
        void InitAttrNameMap();
        void InitIdentifyKeyMap();
    };
    
public:
    static bool LookupAttributeName(U8 id, std::string& name);
    static bool LookupAttributeNote(U8 id, std::string& note);
    static bool LookupAttributeText(U8 id, std::string&, std::string&);   // name:note
    static void LookupAttributeList(std::vector<std::string>& attrList);            // name
    static void LookupAttributeList(std::vector<std::pair<U8, std::string> >& attrList); // id:name
    static double LookupNandEndurance(const std::string& serialNum);
    static bool LookupIdentifyKey(const std::string& str, eIDENTKEY& key);
    
public:

private:
    static sUtilData s_Data;
};

#endif // __COREUTIL_H__
