#include "CoreUtil.h"

using namespace std;
cCoreUtil::sUtilData cCoreUtil::s_Data;

void cCoreUtil::sUtilData::InitAttrNameMap()
{
    // Init AttributeName map
    typedef pair<U16, sATTRPARAM> tItemInfo;
    tItemInfo itemArray[] = {
        #define MAP_ITEM(code, val, show, name, note) tItemInfo(val, sATTRPARAM(name, note, show)),
        #include "AttrName.def"
        #undef MAP_ITEM
    };
    
    for (U32 i = 0, itemCount = sizeof (itemArray) / sizeof (itemArray[0]); i < itemCount; i++)
    {
        tItemInfo& info = itemArray[i];
        sATTRPARAM& param = info.second;

        if ( m_ShowAttributeName || param.Show )
        {
            m_AttrNameMap[info.first] = info.second;
        }
    }
}

void cCoreUtil::sUtilData::InitIdentifyKeyMap()
{
    typedef pair<eIDENTKEY, string> tItemInfo;
    tItemInfo itemArray[] = {
        #define MAP_ITEM(code, name) tItemInfo(code, name),
        #include "IdentifyKey.def"
        #undef MAP_ITEM
    };
    
    for (U32 i = 0, itemCount = sizeof (itemArray) / sizeof (itemArray[0]); i < itemCount; i++)
    {
        tItemInfo& info = itemArray[i];
        m_IdentifyKeyMap[info.second] = info.first;
    }
}

cCoreUtil::sUtilData::sUtilData()
{
    m_ShowAttributeName = false;

    InitAttrNameMap();
    InitIdentifyKeyMap();
}

bool cCoreUtil::LookupAttributeName(U16 id, string& name)
{   
    bool status = false;

    tATTRNAMEMAP::iterator iter = s_Data.m_AttrNameMap.find(id);
    if (iter != s_Data.m_AttrNameMap.end())
    {
        const sATTRPARAM& param = iter->second;
        name = param.Name;
        status = true;
    }
    
    return status;
}

bool cCoreUtil::LookupAttributeNote(U16 id, string& note)
{
    bool status = false;

    tATTRNAMEMAP::iterator iter = s_Data.m_AttrNameMap.find(id);
    if (iter != s_Data.m_AttrNameMap.end())
    {
        const sATTRPARAM& param = iter->second;
        note = param.Note;
        status = true;
    }

    return status;
}

bool cCoreUtil::LookupAttributeText(U16 id, string& name, string& note)
{
    bool status = false;

    tATTRNAMEMAP::iterator iter = s_Data.m_AttrNameMap.find(id);
    if (iter != s_Data.m_AttrNameMap.end())
    {
        const sATTRPARAM& param = iter->second;
        name = param.Name;
        note = param.Note;
        status = true;
    }

    return status;
}

bool cCoreUtil::LookupIdentifyKey(const string& text, eIDENTKEY& key)
{
    bool status = false;

    tIDENTKEYMAP::iterator iter = s_Data.m_IdentifyKeyMap.find(text);
    if (iter != s_Data.m_IdentifyKeyMap.end())
    {
        key = iter->second;
        status = true;
    }

    return status;
}

double cCoreUtil::LookupNandEndurance(const string& serialNum)
{
    // StorFly VSFB25PC064G-100
    // StorFly - VSFB25PC064G-100
    // P: 60000; // C: 3000

    double nandEndurance = 3000;

    size_t dashpos = serialNum.rfind("-");
    bool invalidSerial = (dashpos == string::npos) || (dashpos < 6);

    if(true == invalidSerial) return nandEndurance;

    char enduranceChar = serialNum[dashpos - 6];

    switch (enduranceChar)
    {
        case 'C': nandEndurance = 3000; break;
        case 'P': nandEndurance = 60000; break;
        default: break;
    }

    return nandEndurance;
}
