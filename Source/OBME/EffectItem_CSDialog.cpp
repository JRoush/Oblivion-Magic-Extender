#include "OBME/EffectItem.h"
#include "OBME/EffectItem.rc.h"
#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"

#include "API/CSDialogs/TESDialog.h"
#include "API/CSDialogs/DialogConstants.h"

static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; // send in lieu of WM_COMMAND to when defering control setup from SetInDialog to DialogCallback

namespace OBME {
#ifndef OBLIVION

// methods - dialog interface for CS
void EffectItem::BuildSchoolList(HWND comboList)
{
    TESComboBox::Clear(comboList);
    
    // TODO - populate with MagicSchool forms, using school codes rather than form*
    for (int c = 0; c < Magic::kSchool__MAX; c++) TESComboBox::AddItem(comboList,Magic::GetSchoolName(c),(void*)c);
}
void EffectItem::InitializeDialog(HWND dialog)
{
    // cache pre-editing cost
    cost = -1;  // clear cached cost
    origBaseMagicka = MagickaCost();

    // mgef list
    HWND ctl = GetDlgItem(dialog,IDC_EFIT_EFFECT);
    BuildEffectList(ctl,false);
    TESComboBox::SetCurSelByData(ctl,GetEffectSetting());

    // school list
    BuildSchoolList(GetDlgItem(dialog,IDC_EFIT_SCHOOL));

    // vfx effect list
    BuildEffectList(GetDlgItem(dialog,IDC_EFIT_VFX),true);

    // set area,duration,magnitude    
    SetDlgItemInt(dialog,IDC_EFIT_AREA,GetArea(),true);
    SetDlgItemInt(dialog,IDC_EFIT_DURATION,GetDuration(),true);
    SetDlgItemInt(dialog,IDC_EFIT_MAGNITUDE,GetMagnitude(),true);

    // set effect-dependent controls
    SetInDialog(dialog);    
}
void EffectItem::SetInDialog(HWND dialog)
{
    // magnitude & duration enable states    
    EnableWindow(GetDlgItem(dialog,IDC_EFIT_DURATION),!GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_NoDuration));
    EnableWindow(GetDlgItem(dialog,IDC_EFIT_MAGNITUDE),!GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_NoMagnitude));

    // rebuild range list 
    HWND ctl = GetDlgItem(dialog,IDC_EFIT_RANGE);
    BuildRangeList(ctl);
    TESComboBox::SetCurSelByData(ctl,(void*)GetRange());

    // dispatch range update message - updates area, proj. range, costs
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_RANGE,CBN_SELCHANGE),(LPARAM)ctl);  

    // TODO - switch/init handler subdialog
}
void EffectItem::GetFromDialog(HWND dialog)
{
    // effect setting is set on change, no not retrieved here    
    // range is set on change, no not retrieved here 

    // projectile range
    _MESSAGE("Old proj range = %i",projectileRange);
    HWND ctl = GetDlgItem(dialog,IDC_EFIT_PROJRANGEENABLE);
    if (IsWindowEnabled(ctl) && Button_GetCheck(ctl) == BST_CHECKED) SetProjectileRange(GetDlgItemInt(dialog,IDC_EFIT_PROJRANGE,0,true));
    else SetProjectileRange(-1);
    _MESSAGE("New proj range = %i",projectileRange);

    // area, duration, magnitude
    SetArea(IsWindowEnabled(GetDlgItem(dialog,IDC_EFIT_AREA)) ? GetDlgItemInt(dialog,IDC_EFIT_AREA,0,false) : 0);
    SetDuration(IsWindowEnabled(GetDlgItem(dialog,IDC_EFIT_DURATION)) ? GetDlgItemInt(dialog,IDC_EFIT_DURATION,0,false) : 0);
    SetMagnitude(IsWindowEnabled(GetDlgItem(dialog,IDC_EFIT_MAGNITUDE)) ? GetDlgItemInt(dialog,IDC_EFIT_MAGNITUDE,0,false) : 0);
}
bool EffectItem::HandleDialogEvent(HWND dialog, int uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (uMsg == WM_USERCOMMAND || uMsg == WM_COMMAND)
    {
        UInt32 commandCode = HIWORD(wParam);
        switch (LOWORD(wParam)) // switch on control id
        {
            case IDC_EFIT_RANGE:
            {
                if (commandCode != CBN_SELCHANGE) break;
                SetRange((UInt8)TESComboBox::GetCurSelData((HWND)lParam)); // update range
                // update projectile range checkbox
                HWND ctl = GetDlgItem(dialog,IDC_EFIT_PROJRANGEENABLE);
                _MESSAGE("Checking proj range = %i (%i)",GetProjectileRange(),projectileRange);
                Button_SetCheck(ctl, (GetProjectileRange() >= 0) ? BST_CHECKED : BST_UNCHECKED);
                EnableWindow(ctl,GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_HasProjRange) && GetRange() == Magic::kRange_Target);
                // update enable state of area edit
                bool hasArea = !GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_NoArea) && GetRange() != Magic::kRange_Self; 
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_AREA),hasArea);
            }
            case IDC_EFIT_PROJRANGEENABLE:
            {                
                // update projectile range edit control
                HWND enableCheck = GetDlgItem(dialog,IDC_EFIT_PROJRANGEENABLE);
                HWND edit = GetDlgItem(dialog,IDC_EFIT_PROJRANGE);
                if (IsWindowEnabled(enableCheck) && Button_GetCheck(enableCheck) == BST_CHECKED)
                {
                    // enable projectile range, set positive-definitve range value
                    int range = GetProjectileRange();
                    if (range < 0) range = 0;
                    EnableWindow(edit,true);
                    SetDlgItemInt(dialog,IDC_EFIT_PROJRANGE,range,true);
                }
                else
                {
                    // disable projectile range, clear control text
                    EnableWindow(edit,false);
                    SetWindowText(edit,"");
                }
            }
            case IDC_EFIT_PROJRANGE:
            case IDC_EFIT_AREA:
            case IDC_EFIT_DURATION:
            case IDC_EFIT_MAGNITUDE:
                // update cost controls
                result = 0; return true; // result = 0 to mark command message handled
            case IDC_EFIT_EFFECT:
            {
                // effect setting changed
                EffectSetting* mgef = (EffectSetting*)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_EFIT_EFFECT));
                SetEffectSetting(*mgef); // change effect setting
                SetInDialog(dialog);  // set effect-dependent controls
                result = 0; return true; // result = 0 to mark command message handled
            }
        }
    }
    if (effectHandler && effectHandler->DialogMessageCallback(dialog,uMsg,wParam,lParam,result)) return true;   // pass to handler
    return false;
}
#endif
}   //  end namespace OBME