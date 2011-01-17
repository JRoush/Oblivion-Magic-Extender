#include "OBME/EffectHandlers/ValueModifierEffectHandler.h"
#include "OBME/EffectHandlers/EffectHandler.rc.h"
#include "OBME/EffectSetting.h"

namespace OBME {

/*************************** ValueModifierESHandler ********************************/
// serialization
bool ValueModifierESHandler::LoadHandlerChunk(TESFile& file)
{
    return true; // default handler stores no data in chunk
}
void ValueModifierESHandler::SaveHandlerChunk()
{
    // default handler saves no data in chunk, and hence saves no chunk at all
}
void ValueModifierESHandler::LinkHandler()
{
    // default handler has no fields to link
    // NOTE: the ValueModifierEffect handler will need to link the assoc AV on the parent effect item
    // however, the SummonEffect handler does *not* need to link the assoc form
}
// copy/compare
void ValueModifierESHandler::CopyFrom(const EffectSettingHandler& copyFrom)
{
    // default handler has no fields to copy
}
bool ValueModifierESHandler::CompareTo(const EffectSettingHandler& compareTo)
{
    if (HandlerCode() != compareTo.HandlerCode()) return true;
    
    // default handler has no other fields to compare
    return false;
}
// child Dialog for CS editing
#ifndef OBLIVION
INT ValueModifierESHandler::DialogTemplateID()
{
    // default handler does not require a child dialog in MGEF editing
    return IDD_VALUEMODIFIER;
}
bool ValueModifierESHandler::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    _DMESSAGE("");
    // default handler process no messages
    return false;
}
void ValueModifierESHandler::SetInDialog(HWND dialog)
{
    _DMESSAGE("");

    // flags
    CheckDlgButton(dialog,IDC_FLAG_DETRIMENTAL,parentEffect.GetFlag(EffectSetting::kMgefFlagShift_Detrimental));
    CheckDlgButton(dialog,IDC_FLAG_RECOVERS,parentEffect.GetFlag(EffectSetting::kMgefFlagShift_Recovers));
    CheckDlgButton(dialog,IDC_FLAG_USEATTRIBUTE,parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseAttribute));
    CheckDlgButton(dialog,IDC_FLAG_USESKILL,parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseSkill));
    CheckDlgButton(dialog,IDC_FLAG_USEACTORVALUE,parentEffect.GetFlag(EffectSetting::kMgefFlagShift_UseActorValue));
    CheckDlgButton(dialog,IDC_FLAG_MAGNITUDEISPERCENT,parentEffect.GetFlag(EffectSetting::kMgefFlagShift_MagnitudeIsPercent));
    CheckDlgButton(dialog,IDC_FLAG_MAGNITUDEISLEVEL,parentEffect.GetFlag(EffectSetting::kMgefFlagShift_MagnitudeIsLevel));
    CheckDlgButton(dialog,IDC_FLAG_MAGNITUDEISFEET,parentEffect.GetFlag(EffectSetting::kMgefFlagShift_MagnitudeIsFeet));

    return;
}
void ValueModifierESHandler::GetFromDialog(HWND dialog)
{
    _DMESSAGE("");

    // flags
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_Detrimental,IsDlgButtonChecked(dialog,IDC_FLAG_DETRIMENTAL));
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_Recovers,IsDlgButtonChecked(dialog,IDC_FLAG_RECOVERS));
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseAttribute,IsDlgButtonChecked(dialog,IDC_FLAG_USEATTRIBUTE));
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseSkill,IsDlgButtonChecked(dialog,IDC_FLAG_USESKILL));
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_UseActorValue,IsDlgButtonChecked(dialog,IDC_FLAG_USEACTORVALUE));
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_MagnitudeIsPercent,IsDlgButtonChecked(dialog,IDC_FLAG_MAGNITUDEISPERCENT));
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_MagnitudeIsLevel,IsDlgButtonChecked(dialog,IDC_FLAG_MAGNITUDEISLEVEL));
    parentEffect.SetFlag(EffectSetting::kMgefFlagShift_MagnitudeIsFeet,IsDlgButtonChecked(dialog,IDC_FLAG_MAGNITUDEISFEET));

    return;
}
#endif

/*************************** ValueModifierEIHandler ********************************/
/*************************** ValueModifierAEHandler ********************************/

}   //  end namespace OBME