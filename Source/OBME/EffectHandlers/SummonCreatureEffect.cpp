#include "OBME/EffectHandlers/SummonCreatureEffect.h"
#include "OBME/EffectHandlers/EffectHandler.rc.h"
#include "OBME/EffectSetting.h"

#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"

// global method for returning small enumerations as booleans
inline bool BoolEx(UInt8 value) {return *(bool*)&value;}
// send in lieu of WM_COMMAND to avoid problems with TESFormIDListView::DlgProc 
static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; 
static const char* kNoneEntry = " NONE ";

namespace OBME {

/*************************** SummonCreatureMgefHandler ********************************/
SummonCreatureMgefHandler::SummonCreatureMgefHandler(EffectSetting& effect) 
 : AssociatedItemMgefHandler(effect)
{}
// child Dialog for CS editing
#ifndef OBLIVION
void SummonCreatureMgefHandler::InitializeDialog(HWND dialog)
{
    HWND ctl = 0;
    // Associated Item  - Creatures, LeveledCreatures, and NPCs w/ 'Summonable' flag
    ctl = GetDlgItem(dialog,IDC_SUMN_ASSOCIATEDITEM);
    TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_NPC,true,true); // TODO - filter out NPCs w/o Summonable flag
    TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Creature,false,false); 
    TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_LeveledCreature,false,false);
}
#endif

/*************************** SummonCreatureEfitHandler ********************************/
/*************************** SummonCreatureEffect ********************************/
#ifdef OBLIVION
SummonCreatureEffect::SummonCreatureEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
: ::SummonCreatureEffect(caster,magicItem,(::EffectItem*)effectItem) 
{
}
::ActiveEffect* SummonCreatureEffect::Clone() const
{
    ::ActiveEffect* neweff = Make(caster,magicItem,(OBME::EffectItem*)effectItem); 
    CopyTo(*neweff); 
    return neweff;
}
::ActiveEffect* SummonCreatureEffect::Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
{
    return new SummonCreatureEffect(caster,magicItem,effectItem);
}
#endif
}   //  end namespace OBME