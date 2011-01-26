#include "Utilities/Memaddr.h"

#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "OBME/EffectHandlers/ValueModifierEffect.h"
#include "OBME/EffectHandlers/AssociatedItemEffect.h"
#include "OBME/EffectHandlers/SummonCreatureEffect.h"
#include "OBME/EffectHandlers/BoundItemEffect.h"
#include "OBME/EffectHandlers/ScriptEffect.h"
#include "OBME/EffectHandlers/DispelEffect.h"

#include <map>

namespace OBME {

/*************************** EffectHandler ********************************/
// memory patch & hook addresses
memaddr ActiveEffect_RegisterLastCreateFunc_Hook    (0x009FEDF7,0x0);
// global map of handlers, initialized by dll loader
struct HandlerEntry
{ 
    // members
    UInt32              _ehCode;
    const char*         _ehName;              
    MgefHandler*        (*_CreateMgefHandler)(EffectSetting& effect);
    EfitHandler*        (*_CreateEfitHandler)(EffectItem& item);
    #ifdef OBLIVION
    ::ActiveEffect*     (*_CreateActiveEffect)(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem);
    #endif
};
class HandlerMap : public std::map<UInt32,HandlerEntry>
{
public:

    template <class MgefHandlerT, class EfitHandlerT, class ActiveEffectT>
    inline void insert(UInt32 ehCode, const char* ehName)
    {
        HandlerEntry entry;
        entry._ehCode = ehCode;
        entry._ehName = ehName;
        entry._CreateMgefHandler = MgefHandlerT::Make;
        entry._CreateEfitHandler = EfitHandlerT::Make;
        #ifdef OBLIVION
        entry._CreateActiveEffect = ActiveEffectT::Make;
        #endif
        std::map<UInt32,HandlerEntry>::insert(std::pair<UInt32,HandlerEntry>(ehCode,entry));
    }

    HandlerMap()
    {
        // don't put output statements here, as the output log may not yet be initialized   
        insert<GenericMgefHandler<EffectHandler::kACTV>,GenericEfitHandler<EffectHandler::kACTV>,ActiveEffect>(EffectHandler::kACTV,"< Default >");
        insert<ScriptMgefHandler,ScriptEfitHandler,ScriptEffect>(EffectHandler::kSEFF,"Scripted");
        insert<GenericMgefHandler<EffectHandler::kSUDG>,GenericEfitHandler<EffectHandler::kSUDG>,ActiveEffect>(EffectHandler::kSUDG,"Sun Damage");
        insert<GenericMgefHandler<EffectHandler::kDEMO>,GenericEfitHandler<EffectHandler::kDEMO>,ActiveEffect>(EffectHandler::kDEMO,"Demoralize");
        insert<GenericMgefHandler<EffectHandler::kCOCR>,GenericEfitHandler<EffectHandler::kCOCR>,ActiveEffect>(EffectHandler::kCOCR,"Command Creature");
        insert<GenericMgefHandler<EffectHandler::kCOHU>,GenericEfitHandler<EffectHandler::kCOHU>,ActiveEffect>(EffectHandler::kCOHU,"Command Humanoid");
        insert<GenericMgefHandler<EffectHandler::kREAN>,GenericEfitHandler<EffectHandler::kREAN>,ActiveEffect>(EffectHandler::kREAN,"Reanimate");
        insert<GenericMgefHandler<EffectHandler::kTURN>,GenericEfitHandler<EffectHandler::kTURN>,ActiveEffect>(EffectHandler::kTURN,"Turn Undead");
        insert<GenericMgefHandler<EffectHandler::kVAMP>,GenericEfitHandler<EffectHandler::kVAMP>,ActiveEffect>(EffectHandler::kVAMP,"Vampirism");
        insert<GenericMgefHandler<EffectHandler::kLGHT>,GenericEfitHandler<EffectHandler::kLGHT>,ActiveEffect>(EffectHandler::kLGHT,"Light");
        insert<DispelMgefHandler,DispelEfitHandler,DispelEffect>(EffectHandler::kDSPL,"Dispel");
        insert<GenericMgefHandler<EffectHandler::kCURE>,GenericEfitHandler<EffectHandler::kCURE>,ActiveEffect>(EffectHandler::kCURE,"Cure");
        insert<GenericMgefHandler<EffectHandler::kDIAR>,GenericEfitHandler<EffectHandler::kDIAR>,ActiveEffect>(EffectHandler::kDIAR,"Disintegrate Armor");
        insert<GenericMgefHandler<EffectHandler::kDIWE>,GenericEfitHandler<EffectHandler::kDIWE>,ActiveEffect>(EffectHandler::kDIWE,"Disintegrate Weapon");
        insert<GenericMgefHandler<EffectHandler::kLOCK>,GenericEfitHandler<EffectHandler::kLOCK>,ActiveEffect>(EffectHandler::kLOCK,"Lock");
        insert<GenericMgefHandler<EffectHandler::kOPEN>,GenericEfitHandler<EffectHandler::kOPEN>,ActiveEffect>(EffectHandler::kOPEN,"Open");
        insert<GenericMgefHandler<EffectHandler::kSTRP>,GenericEfitHandler<EffectHandler::kSTRP>,ActiveEffect>(EffectHandler::kSTRP,"Soul Trap");
        insert<GenericValueModifierMgefHandler<EffectHandler::kMDAV>,GenericValueModifierEfitHandler<EffectHandler::kMDAV>,ValueModifierEffect>(EffectHandler::kMDAV,"Value Modifier");
        insert<GenericValueModifierMgefHandler<EffectHandler::kABSB>,GenericValueModifierEfitHandler<EffectHandler::kABSB>,ValueModifierEffect>(EffectHandler::kABSB,"Absorb");
        insert<GenericValueModifierMgefHandler<EffectHandler::kPARA>,GenericValueModifierEfitHandler<EffectHandler::kPARA>,ValueModifierEffect>(EffectHandler::kPARA,"Paralysis");
        insert<GenericValueModifierMgefHandler<EffectHandler::kSHLD>,GenericValueModifierEfitHandler<EffectHandler::kSHLD>,ValueModifierEffect>(EffectHandler::kSHLD,"Shield");
        insert<GenericValueModifierMgefHandler<EffectHandler::kCALM>,GenericValueModifierEfitHandler<EffectHandler::kCALM>,ValueModifierEffect>(EffectHandler::kCALM,"Calm");
        insert<GenericValueModifierMgefHandler<EffectHandler::kFRNZ>,GenericValueModifierEfitHandler<EffectHandler::kFRNZ>,ValueModifierEffect>(EffectHandler::kFRNZ,"Frenzy");
        insert<GenericValueModifierMgefHandler<EffectHandler::kCHML>,GenericValueModifierEfitHandler<EffectHandler::kCHML>,ValueModifierEffect>(EffectHandler::kCHML,"Chameleon");
        insert<GenericValueModifierMgefHandler<EffectHandler::kINVI>,GenericValueModifierEfitHandler<EffectHandler::kINVI>,ValueModifierEffect>(EffectHandler::kINVI,"Invisibility");
        insert<GenericValueModifierMgefHandler<EffectHandler::kDTCT>,GenericValueModifierEfitHandler<EffectHandler::kDTCT>,ValueModifierEffect>(EffectHandler::kDTCT,"Detect Life");
        insert<GenericValueModifierMgefHandler<EffectHandler::kNEYE>,GenericValueModifierEfitHandler<EffectHandler::kNEYE>,ValueModifierEffect>(EffectHandler::kNEYE,"Night Eye");
        insert<GenericValueModifierMgefHandler<EffectHandler::kDARK>,GenericValueModifierEfitHandler<EffectHandler::kDARK>,ValueModifierEffect>(EffectHandler::kDARK,"Darkness");
        insert<GenericValueModifierMgefHandler<EffectHandler::kTELE>,GenericValueModifierEfitHandler<EffectHandler::kTELE>,ValueModifierEffect>(EffectHandler::kTELE,"Telekinesis");
        insert<SummonCreatureMgefHandler,SummonCreatureEfitHandler,SummonCreatureEffect>(EffectHandler::kSMAC,"Summon Actor");
        insert<BoundItemMgefHandler,BoundItemEfitHandler,BoundItemEffect>(EffectHandler::kSMBO,"Bound Item");
    }

} g_handlerMap;
// methods
const char* EffectHandler::HandlerName() const
{
    // lookup name in handler map
    HandlerMap::iterator it = g_handlerMap.find(HandlerCode());
    if (it != g_handlerMap.end()) return it->second._ehName;
    return 0; // unrecognized handler
}
bool EffectHandler::GetNextHandler(UInt32& ehCode, const char*& ehName)
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
void EffectHandler::Initialize()
{
    _MESSAGE("Initializing ...");
    #ifdef OBLIVION
    // Hook registration of active effect creation funcs, to populate our own creation func maps
    // ActiveEffect_RegisterLastCreateFunc_Hook.WriteRelJump(&ActiveEffect::PopulateCreatorMap);
    #endif
}
/*************************** MgefHandler ********************************/
MgefHandler* MgefHandler::Create(UInt32 handlerCode, EffectSetting& effect)
{
    // lookup creator func in handler map
    HandlerMap::iterator it = g_handlerMap.find(handlerCode);
    if (it != g_handlerMap.end() && it->second._CreateMgefHandler) return it->second._CreateMgefHandler(effect);
    return 0; // unrecognized handler
}
// serialization
bool MgefHandler::LoadHandlerChunk(TESFile& file, UInt32 RecordVersion) { return true; } // default handler stores no data in chunk
void MgefHandler::SaveHandlerChunk() {} // default handler saves no data, and hence saves no chunk at all
void MgefHandler::LinkHandler() {} // default handler has no fields to link
void MgefHandler::UnlinkHandler() {}
// copy/compare
void MgefHandler::CopyFrom(const MgefHandler& copyFrom) {}   // default handler has no fields to copy
bool MgefHandler::CompareTo(const MgefHandler& compareTo)
{
    if (HandlerCode() != compareTo.HandlerCode()) return true;    
    // default handler has no other fields to compare
    return false;
}
#ifndef OBLIVION
// reference management in CS
void MgefHandler::RemoveFormReference(TESForm& form) {} // default handler has no form references
// child Dialog in CS
INT MgefHandler::DialogTemplateID() { return 0; }   // default handler has no dialog
void MgefHandler::InitializeDialog(HWND dialog) {}
bool MgefHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result) { return false; }
void MgefHandler::SetInDialog(HWND dialog) {}
void MgefHandler::GetFromDialog(HWND dialog) {}
void MgefHandler::CleanupDialog(HWND dialog) {} 
#endif  
/*************************** EfitHandler ********************************/
/*************************** ActiveEffect ********************************/
#ifdef OBLIVION
bool ActvHandler::FullyLocalized()
{
    return true;
}
ActiveEffect::ActiveEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem) 
: ::ActiveEffect(caster,magicItem,(::EffectItem*)effectItem) 
{
}
::ActiveEffect* ActiveEffect::Clone() const
{
    ::ActiveEffect* neweff = Make(caster,magicItem,(OBME::EffectItem*)effectItem); 
    CopyTo(*neweff); 
    return neweff;
}
::ActiveEffect* ActiveEffect::Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
{
    return new ActiveEffect(caster,magicItem,effectItem);
}
#endif
}   //  end namespace OBME