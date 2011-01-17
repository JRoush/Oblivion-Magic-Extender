#include "Utilities/Memaddr.h"

#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "OBME/EffectHandlers/GenericEffectHandler.h"
#include "OBME/EffectHandlers/ValueModifierEffectHandler.h"

#include <map>

namespace OBME {

/*************************** EffectHandlerBase ********************************/
// memory patch & hook addresses
memaddr ActiveEffect_RegisterLastCreateFunc_Hook    (0x009FEDF7,0x0);
// global map of handlers, initialized by dll loader
struct HandlerEntry
{
    // members
    UInt32              _ehCode;
    const char*         _ehName;              
    typedef EffectSettingHandler*   (* ESHCreateFunc)(EffectSetting& effect);
    ESHCreateFunc       _eshCreateFunc;
    typedef EffectItemHandler*      (* EIHCreateFunc)(EffectItem& item);
    EIHCreateFunc       _eihCreateFunc;
    #ifdef OBLIVION
    typedef ActiveEffectHandler*    (* AEHCreateFunc)(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);
    AEHCreateFunc       _aehCreateFunc;
    #endif
    // methods - constructor
    /*
    #ifndef OBLIVION
        HandlerEntry(UInt32 ehCode, const char* ehName, ESHCreateFunc eshCreateFunc, EIHCreateFunc eihCreateFunc, AEHCreateFunc aehCreateFunc) 
            : _ehCode(ehCode), _ehName(ehName), _eshCreateFunc(eshCreateFunc), _eihCreateFunc(eihCreateFunc), _aehCreateFunc(aehCreateFunc) {}
        #define HANDLERENTRY(code,name,ESHCreateFunc,EIHCreateFunc,AEHCreateFunc) HandlerEntry(code,name,ESHCreateFunc,EIHCreateFunc,AEHCreateFunc)
    #else
        HandlerEntry(UInt32 ehCode, const char* ehName, ESHCreateFunc eshCreateFunc, EIHCreateFunc eihCreateFunc) 
            : _ehCode(ehCode), _ehName(ehName), _eshCreateFunc(eshCreateFunc), _eihCreateFunc(eihCreateFunc) {}
        #define HANDLERENTRY(code,name,ESHCreateFunc,EIHCreateFunc,AEHCreateFunc) HandlerEntry(code,name,ESHCreateFunc,EIHCreateFunc)
    #endif
    */


};
class HandlerMap : public std::map<UInt32,HandlerEntry>
{
public:
    inline void insert(HandlerEntry& entry)
    {
        std::map<UInt32,HandlerEntry>::insert(std::pair<UInt32,HandlerEntry>(entry._ehCode,entry));
    }
    HandlerMap()
    {
        /*
        // don't put output statements here, as the output log may not yet be initialized        
        insert(HANDLERENTRY(EffectHandlerCode,"Default Handler",EffectSettingHandler::Make,EffectItemHandler::Make,ActiveEffectHandler::Make));
        //insert(HANDLERENTRY(ValueModifierEHCode,"Value Modifier Handler",ValueModifierESHandler::Make,ValueModifierEIHandler::Make,(void*)0));
        //insert(HandlerEntry('LPSD',"Dispel Handler",(void*)&GenericEffectSettingHandler::Make<'LPSD'>,0));
        */
    }

} g_handlerMap;
// methods
const char* EffectHandlerBase::HandlerName() const
{
    // lookup name in handler map
    HandlerMap::iterator it = g_handlerMap.find(HandlerCode());
    if (it != g_handlerMap.end()) return it->second._ehName;
    return 0; // unrecognized handler
}
UInt32 EffectHandlerBase::GetDefaultHandlerCode(UInt32 mgefCode)
{   
    if (!EffectSetting::IsMgefCodeVanilla(mgefCode)) 
    {
        _DMESSAGE("Default Handler for effect '%4.4s' {%08X} is 'ACTV' because code is not vanilla",&mgefCode,mgefCode);
        return 'VTCA'; // default handler
    }

    mgefCode = Swap32(mgefCode);
    UInt32 ehCode = 0;

    // compare first character
    switch (mgefCode >> 0x18)
    {
    case 'R': // rally, restore, resist, reflect & reanimate
    case 'W': // water walking, water breathing, weakness
         if (mgefCode != 'REAN') ehCode = 'MDAV';
         break;
    case 'Z':
        ehCode = 'SMAC';  // summon actor
        break;
    }
    // compare first two characters
    if (!ehCode)
    {
        switch (mgefCode >> 0x10)
        {
        case 'AB': // absorb
            ehCode = 'ABSB'; 
            break;
        case 'BA': // bound armor
        case 'BW': // bound weapon
        case 'MY': // mythic dawn gear
            ehCode = 'SMBO';
            break;
        case 'CU': // cure
            ehCode = 'CURE';
            break;
        case 'DG': // damage
        case 'DR': // drain
        case 'FO': // fortify
            ehCode = 'MDAV';
            break;
        }
    }
    // compare all characters
    if (!ehCode)
    {
        switch (mgefCode)
        {
        case 'FISH': // fire shield
        case 'LISH': // shock shield
        case 'FRSH': // frost shield
        case 'SHLD': // shield
            ehCode = 'SHLD';
            break;
        case 'POSN': // poison VFX effect
        case 'DISE': // disease VFX effect
            ehCode = 'ACTV';
            break;
        case 'BRDN': // burden
        case 'CHRM': // charm
        case 'DUMY': // Merhunes Dagon effect
        case 'FIDG': // fire damage
        case 'FRDG': // frost damage
        case 'FTHR': // feather
        case 'SABS': // absorb spell
        case 'SHDG': // shock damage
        case 'SLNC': // silence
        case 'STMA': // stunted magicka
            ehCode = 'MDAV';
            break;
        }
    }
    if (!ehCode) ehCode = mgefCode; // assume effect code is also handler code

    mgefCode = Swap32(mgefCode);
    ehCode = Swap32(ehCode);
    _DMESSAGE("Default Handler for effect '%4.4s' {%08X} is '%4.4s'",&mgefCode,mgefCode,&ehCode);
    return ehCode;
}
bool EffectHandlerBase::GetNextHandler(UInt32& ehCode, const char*& ehName)
{
    // cached iterator from last lookup
    static HandlerMap::iterator it = g_handlerMap.end();
    // get next entry
    if (ehCode == 0)
    {   
        // get first code
        it = g_handlerMap.begin();
    }
    else if (it != g_handlerMap.end() && it->first == ehCode)
    {
        // current code matches cached iterator, advance to next entry
        it++;
    }
    else
    {
        // cached iterator doesn't match current code, find code in map
        it = g_handlerMap.find(ehCode);
        if (it != g_handlerMap.end()) it++; // advance to next code
    }
    // return code, name
    if (it == g_handlerMap.end())
    {
        // end of map reached
        ehCode = 0;
        ehName = 0;
        return false;
    }
    else
    {
        // fetch code & name
        ehCode = it->second._ehCode;
        ehName = it->second._ehName;
        return true;
    }
}
// hooks
void EffectHandlerBase::Initialize()
{
    _MESSAGE("Initializing ...");
    #ifdef OBLIVION
    // Hook registration of active effect creation funcs, to populate our own creation func maps
    // ActiveEffect_RegisterLastCreateFunc_Hook.WriteRelJump(&ActiveEffect::PopulateCreatorMap);
    #endif
}
/*************************** EffectSettingHandler ********************************/
EffectSettingHandler* EffectSettingHandler::Create(UInt32 handlerCode, EffectSetting& effect)
{
    // lookup creator func in handler map
    HandlerMap::iterator it = g_handlerMap.find(handlerCode);
    if (it != g_handlerMap.end() && it->second._eshCreateFunc) return it->second._eshCreateFunc(effect);
    return 0; // unrecognized handler
}
// serialization
bool EffectSettingHandler::LoadHandlerChunk(TESFile& file)
{
    return true; // default handler stores no data in chunk
}
void EffectSettingHandler::SaveHandlerChunk()
{
    // default handler saves no data in chunk, and hence saves no chunk at all
}
void EffectSettingHandler::LinkHandler()
{
    // default handler has no fields to link
    // NOTE: the ValueModifierEffect handler will need to link the assoc AV on the parent effect item
    // however, the SummonEffect handler does *not* need to link the assoc form
}
// copy/compare
void EffectSettingHandler::CopyFrom(const EffectSettingHandler& copyFrom)
{
    // default handler has no fields to copy
}
bool EffectSettingHandler::CompareTo(const EffectSettingHandler& compareTo)
{
    if (HandlerCode() != compareTo.HandlerCode()) return true;
    
    // default handler has no other fields to compare
    return false;
}
// child Dialog for CS editing
#ifndef OBLIVION
INT EffectSettingHandler::DialogTemplateID()
{
    _DMESSAGE("");
    // default handler does not require a child dialog in MGEF editing
    return 0;
}
bool EffectSettingHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    _DMESSAGE("");
    // default handler process no messages
    return false;
}
void EffectSettingHandler::SetInDialog(HWND dialog)
{
    _DMESSAGE("");
    // default handler has no fields
    return;
}
void EffectSettingHandler::GetFromDialog(HWND dialog)
{
    _DMESSAGE("");
    // default handler has no fields
    return;
}
void EffectSettingHandler::CleanupDialog(HWND dialog)
{
    _DMESSAGE("");
    // default handler has no dialog
    return;
} 
#endif
/*************************** EffectItemHandler ********************************/
/*************************** ActiveEffectHandler ********************************/
#ifdef OBLIVION
bool ActiveEffectHandlerBase::FullyLocalized()
{
    return true;
}
ActiveEffectHandler::ActiveEffectHandler(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem) 
    : ActiveEffect(caster,magicItem,(::EffectItem*)effectItem) 
{
}
ActiveEffect* ActiveEffectHandler::Clone() const
{
    ActiveEffectHandler* neweff = Make(caster,magicItem,(OBME::EffectItem*)effectItem); 
    CopyTo(*neweff); 
    return neweff;
}
ActiveEffectHandler* ActiveEffectHandler::Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
{
    return new ActiveEffectHandler(caster,magicItem,effectItem);
}
#endif
}   //  end namespace OBME