#include "OBME/EffectHandlers/AssociatedItemEffect.h"
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

/*************************** AssociatedItemMgefHandler ********************************/
AssociatedItemMgefHandler::AssociatedItemMgefHandler(EffectSetting& effect) 
 : MgefHandler(effect)
{}
// serialization
void AssociatedItemMgefHandler::LinkHandler()
{
    #ifndef OBLIVION
    // add a cross ref for the associated form
    if (TESForm* assocItem = TESForm::LookupByFormID(parentEffect.mgefParam)) assocItem->AddCrossReference(&parentEffect);
    #endif
}
void AssociatedItemMgefHandler::UnlinkHandler()
{
    #ifndef OBLIVION
    // remove cross ref for the associated form
    if (TESForm* assocItem = TESForm::LookupByFormID(parentEffect.mgefParam)) assocItem->RemoveCrossReference(&parentEffect);
    #endif
}
// copy/compare
void AssociatedItemMgefHandler::CopyFrom(const MgefHandler& copyFrom)
{
    const AssociatedItemMgefHandler* src = dynamic_cast<const AssociatedItemMgefHandler*>(&copyFrom);
    if (!src) return; // wrong polymorphic type

    // copy mgefParam, manage references
    #ifndef OBLIVION
    if (TESForm* form = TESForm::LookupByFormID(parentEffect.mgefParam)) form->RemoveCrossReference(&parentEffect);
    if (TESForm* form = TESForm::LookupByFormID(src->parentEffect.mgefParam)) form->AddCrossReference(&parentEffect);
    #endif
    parentEffect.mgefParam = src->parentEffect.mgefParam;
}
bool AssociatedItemMgefHandler::CompareTo(const MgefHandler& compareTo)
{   
    // define failure codes - all of which must evaluate to boolean true
    enum
    {
        kCompareSuccess         = 0,
        kCompareFail_General    = 1,        
        kCompareFail_Base,
        kCompareFail_Polymorphic,
    };

    if (MgefHandler::CompareTo(compareTo)) return BoolEx(kCompareFail_Base);
    const AssociatedItemMgefHandler* src = dynamic_cast<const AssociatedItemMgefHandler*>(&compareTo);
    if (!src) return BoolEx(kCompareFail_Polymorphic); // wrong polymorphic type

    // handlers are identical
    return BoolEx(kCompareSuccess);
}
#ifndef OBLIVION
// reference management in CS
void AssociatedItemMgefHandler::RemoveFormReference(TESForm& form) 
{
    // clear mgefParam if it matches formID of ref
    if (parentEffect.mgefParam == form.formID) parentEffect.mgefParam = 0;
}
// child Dialog in CS
INT AssociatedItemMgefHandler::DialogTemplateID() { return IDD_MGEF_SUMN; }
void AssociatedItemMgefHandler::SetInDialog(HWND dialog)
{
    HWND ctl = 0;
    // Associated Item 
    ctl = GetDlgItem(dialog,IDC_SUMN_ASSOCIATEDITEM);
    TESComboBox::SetCurSelByData(ctl, TESForm::LookupByFormID(parentEffect.mgefParam));
}
void AssociatedItemMgefHandler::GetFromDialog(HWND dialog)
{
    HWND ctl = 0;
    // Associated Item 
    ctl = GetDlgItem(dialog,IDC_SUMN_ASSOCIATEDITEM);
    TESForm* form = (TESForm*)TESComboBox::GetCurSelData(ctl);
    parentEffect.mgefParam = form ? form->formID : 0;
}
#endif

/*************************** AssociatedItemEfitHandler ********************************/
/*************************** AssociatedItemEffect ********************************/
#ifdef OBLIVION
AssociatedItemEffect::AssociatedItemEffect(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
: ::AssociatedItemEffect(caster,magicItem,(::EffectItem*)effectItem) 
{
}
::ActiveEffect* AssociatedItemEffect::Clone() const
{
    ::ActiveEffect* neweff = Make(caster,magicItem,(OBME::EffectItem*)effectItem); 
    CopyTo(*neweff); 
    return neweff;
}
::ActiveEffect* AssociatedItemEffect::Make(MagicCaster* caster, MagicItem* magicItem, EffectItem* effectItem)
{
    return new AssociatedItemEffect(caster,magicItem,effectItem);
}
#endif
}   //  end namespace OBME