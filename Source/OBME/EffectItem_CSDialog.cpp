#include "OBME/EffectItem.h"
#include "OBME/EffectItem.rc.h"
#include "OBME/EffectSetting.h"
#include "OBME/EffectHandlers/EffectHandler.h"
#include "OBME/Magic.h"
#include "OBME/CSDialogUtilities.h"

#include "API/Actors/ActorValues.h"
#include "API/CSDialogs/TESDialog.h"
#include "API/CSDialogs/DialogConstants.h"
#include "API/CSDialogs/DialogExtraData.h"
#include "API/ExtraData/BSExtraData.h"
#include "API/ExtraData/ExtraDataList.h"

// local declaration of module handle from obme.cpp
extern HMODULE hModule;

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
    _VMESSAGE("Initialize <%p> in dialog");

    // cache pre-editing cost
    cost = -1;  // clear cached cost
    origBaseMagicka = MagickaCost();

    // mgef list
    HWND ctl = GetDlgItem(dialog,IDC_EFIT_EFFECT);
    BuildEffectList(ctl,false);
    TESComboBox::SetCurSelByData(ctl,GetEffectSetting());

    // projectile range
    SInt32 projRange = scriptInfo ? ((EffectItemExtra*)scriptInfo)->projectileRange : -1;
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_PROJRANGEENABLE), (projRange < 0) ? BST_UNCHECKED : BST_CHECKED );
    SetDlgItemInt(dialog,IDC_EFIT_PROJRANGE, (projRange < 0) ? 0 : projRange ,true); 

    // set area, duration, magnitude   
    SetDlgItemInt(dialog,IDC_EFIT_AREA,GetArea(),false);
    SetDlgItemInt(dialog,IDC_EFIT_DURATION,GetDuration(),false);
    SetDlgItemInt(dialog,IDC_EFIT_MAGNITUDE,GetMagnitude(),false);

    // name override
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_NAME), IsEfitFieldOverridden(kEfitFlagShift_Name) ? BST_CHECKED : BST_UNCHECKED );
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_WHOLEDESC), GetEfitFlag(kEfitFlagShift_Name) ? BST_CHECKED : BST_UNCHECKED );
    SetWindowText(GetDlgItem(dialog,IDC_EFIT_NAME),GetEffectName());
    SetWindowText(GetDlgItem(dialog,IDC_EFIT_WHOLEDESC),GetDisplayText()); 
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_OVR_NAME,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_EFIT_OVR_NAME));
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_OVR_WHOLEDESC,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_EFIT_OVR_WHOLEDESC));

    // school override
    BuildSchoolList(GetDlgItem(dialog,IDC_EFIT_SCHOOL));
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_EFIT_SCHOOL),(void*)GetSchool());
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_SCHOOL), IsEfitFieldOverridden(kEfitFlagShift_School) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_OVR_SCHOOL,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_EFIT_OVR_SCHOOL));

    // fx effect override
    BuildEffectList(GetDlgItem(dialog,IDC_EFIT_VFX),true);
    TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_EFIT_VFX), GetFXEffect());
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_FXEFFECT), IsEfitFieldOverridden(kEfitFlagShift_FXMgef) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_OVR_FXEFFECT,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_EFIT_OVR_FXEFFECT));

    // hostility override
    ctl = GetDlgItem(dialog,IDC_EFIT_HOSTILITY);
    TESComboBox::Clear(ctl);
    TESComboBox::AddItem(ctl,"Neutral",(void*)Magic::kHostility_Neutral);
    TESComboBox::AddItem(ctl,"Hostile",(void*)Magic::kHostility_Hostile);
    TESComboBox::AddItem(ctl,"Beneficial",(void*)Magic::kHostility_Beneficial);
    TESComboBox::SetCurSelByData(ctl, (void*)GetHostility());
    bool overridden = IsMgefFlagOverridden(EffectSetting::kMgefFlagShift_Hostile) && IsMgefFlagOverridden(EffectSetting::kMgefFlagShift_Beneficial);
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_HOSTILITY), overridden ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_OVR_HOSTILITY,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_EFIT_OVR_HOSTILITY));

    // resistance override
    ctl = GetDlgItem(dialog,IDC_EFIT_RESISTAV);
    TESComboBox::PopulateWithActorValues(ctl,true,false);
    TESComboBox::AddItem(ctl,kNoneEntry,(void*)ActorValues::kActorVal__UBOUND); // game uses 0xFFFFFFFF as an invalid resistance code
    TESComboBox::SetCurSelByData(ctl, (void*)GetResistAV());
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_RESISTAV), IsEfitFieldOverridden(kEfitFlagShift_ResistAV) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_OVR_RESISTAV,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_EFIT_OVR_RESISTAV));

    // cost override
    bool costOverride = IsEfitFieldOverridden(kEfitFlagShift_Cost);
    bool costNoAutoCalc = GetEfitFlag(kEfitFlagShift_Cost);
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_BASECOST), (costOverride && !costNoAutoCalc) ? BST_CHECKED : BST_UNCHECKED );
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_WHOLECOST), (costOverride && costNoAutoCalc) ? BST_CHECKED : BST_UNCHECKED );
    TESDialog::SetDlgItemTextFloat(dialog,IDC_EFIT_COST, costNoAutoCalc ? MagickaCost() : GetBaseCost());
    TESDialog::SetTextFloatLimits(GetDlgItem(dialog,IDC_EFIT_COST), 0, 10000, false);
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_OVR_BASECOST,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_EFIT_OVR_BASECOST));

    // icon override
    DialogExtraIcon* iconExtra = new DialogExtraIcon;   // build a temporary TESIcon object 
    iconExtra->texturePath = GetEffectIcon();
    TESDialog::GetDialogExtraList(dialog)->AddExtra(iconExtra); // add temp icon to dialog ExtraDataList
    iconExtra->SetComponentInDlg(dialog);
    Button_SetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_ICON), IsEfitFieldOverridden(kEfitFlagShift_Icon) ? BST_CHECKED : BST_UNCHECKED );
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_OVR_ICON,BN_CLICKED),(LPARAM)GetDlgItem(dialog,IDC_EFIT_OVR_ICON));

    // set effect-dependent controls
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_EFFECT,CBN_SELCHANGE),(LPARAM)GetDlgItem(dialog,IDC_EFIT_EFFECT));   
}
void EffectItem::SetInDialog(HWND dialog)
{
    // magnitude & duration enable states    
    EnableWindow(GetDlgItem(dialog,IDC_EFIT_DURATION),!GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_NoDuration));
    EnableWindow(GetDlgItem(dialog,IDC_EFIT_MAGNITUDE),!GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_NoMagnitude));

    // rebuild range list 
    HWND rangeCtl = GetDlgItem(dialog,IDC_EFIT_RANGE);
    BuildRangeList(rangeCtl);
    TESComboBox::SetCurSelByData(rangeCtl,(void*)GetRange());

    // dispatch range update message - updates area, proj. range, costs
    SendMessage(dialog,WM_USERCOMMAND,MAKEWPARAM(IDC_EFIT_RANGE,CBN_SELCHANGE),(LPARAM)rangeCtl);

    // handler
    if (GetHandler()) 
    {
        GetHandler()->InitializeDialog(dialog);
        GetHandler()->SetInDialog(dialog);
    }
}
void EffectItem::GetFromDialog(HWND dialog)
{
    _VMESSAGE("Get <%p> from dialog");
    // effect setting is set on change, no not retrieved here    
    // range is set on change, no not retrieved here 

    // proj. range, area, duration, magnitude
    SetProjectileRange(IsWindowEnabled(GetDlgItem(dialog,IDC_EFIT_PROJRANGE)) ? GetDlgItemInt(dialog,IDC_EFIT_PROJRANGE,0,false) : -1);
    SetArea(IsWindowEnabled(GetDlgItem(dialog,IDC_EFIT_AREA)) ? GetDlgItemInt(dialog,IDC_EFIT_AREA,0,false) : 0);
    SetDuration(IsWindowEnabled(GetDlgItem(dialog,IDC_EFIT_DURATION)) ? GetDlgItemInt(dialog,IDC_EFIT_DURATION,0,false) : 0);
    SetMagnitude(IsWindowEnabled(GetDlgItem(dialog,IDC_EFIT_MAGNITUDE)) ? GetDlgItemInt(dialog,IDC_EFIT_MAGNITUDE,0,false) : 0);

    // handler
    if (GetHandler()) GetHandler()->GetFromDialog(dialog);

    // name override
    if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_NAME)))
    {
        char buffer[0x100];
        if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_WHOLEDESC))) 
        {
            GetWindowText(GetDlgItem(dialog,IDC_EFIT_WHOLEDESC),buffer,sizeof(buffer));
            SetDisplayText(buffer);
        }
        else
        {
            GetWindowText(GetDlgItem(dialog,IDC_EFIT_NAME),buffer,sizeof(buffer));
            SetEffectName(buffer);
        }
    }
    else SetEfitFieldOverride(kEfitFlagShift_Name,false);   // clear override

    // school override
    if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_SCHOOL))) SetSchool((UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_EFIT_SCHOOL)));
    else SetEfitFieldOverride(kEfitFlagShift_School,false);

    // fx effect override
    if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_FXEFFECT))) 
    {
        EffectSetting* mgef = (EffectSetting*)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_EFIT_VFX));
        SetFXEffect(mgef ? mgef->mgefCode : EffectSetting::kMgefCode_None);
    }
    else SetEfitFieldOverride(kEfitFlagShift_FXMgef,false);

    // hostility override
    if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_HOSTILITY))) SetHostility((UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_EFIT_HOSTILITY)));
    else 
    {
        SetMgefFlagOverride(EffectSetting::kMgefFlagShift_Hostile,false);
        SetMgefFlagOverride(EffectSetting::kMgefFlagShift_Beneficial,false);
    }

    // resistance override
    if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_RESISTAV))) SetResistAV((UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_EFIT_RESISTAV)));
    else SetEfitFieldOverride(kEfitFlagShift_ResistAV,false);

    // cost override
    if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_BASECOST))) SetBaseCost(TESDialog::GetDlgItemTextFloat(dialog,IDC_EFIT_COST));
    else if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_WHOLECOST))) SetMagickaCost(TESDialog::GetDlgItemTextFloat(dialog,IDC_EFIT_COST));
    else SetEfitFieldOverride(kEfitFlagShift_Cost,false);

    // icon override
    DialogExtraIcon* extraIcon = (DialogExtraIcon*)TESDialog::GetDialogExtraData(dialog,DialogExtraIcon::kDialogExtra_Icon);
    if (Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_ICON)) && extraIcon)
    {
        extraIcon->GetComponentFromDlg(dialog); // get icon path
        SetEffectIcon(extraIcon->texturePath);
    }
    else SetEfitFieldOverride(kEfitFlagShift_Icon,false);
    
}
void EffectItem::CleanupDialog(HWND dialog)
{
    _VMESSAGE("Cleanup dialog for <%p>");

    // handler
    if (GetHandler()) GetHandler()->CleanupDialog(dialog);

    // icon extra obj
    if (DialogExtraIcon* extraIcon = (DialogExtraIcon*)TESDialog::GetDialogExtraData(dialog,DialogExtraIcon::kDialogExtra_Icon))
    {
        extraIcon->ComponentDlgCleanup(dialog);
    }
}
bool EffectItem::HandleDialogEvent(HWND dialog, int uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (uMsg == WM_USERCOMMAND || uMsg == WM_COMMAND)
    {
        UInt32 commandCode = HIWORD(wParam);
        switch (LOWORD(wParam)) // switch on control id
        {            
            // Range - changes propegate downward
            case IDC_EFIT_RANGE:
            {
                if (commandCode != CBN_SELCHANGE) break;
                SetRange((UInt8)TESComboBox::GetCurSelData((HWND)lParam)); // update range
                // update enable state of proj range checkbox
                bool canHaveProjRange = GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_HasProjRange) && GetRange() == Magic::kRange_Target;
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_PROJRANGEENABLE),canHaveProjRange);
                // update enable state of area edit
                bool hasArea = !GetEffectSetting()->GetFlag(EffectSetting::kMgefFlagShift_NoArea) && GetRange() != Magic::kRange_Self; 
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_AREA),hasArea);
            }
            case IDC_EFIT_PROJRANGEENABLE:
            {                
                HWND enableCheck = GetDlgItem(dialog,IDC_EFIT_PROJRANGEENABLE);
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_PROJRANGE),IsWindowEnabled(enableCheck) && Button_GetCheck(enableCheck) == BST_CHECKED);
            }
            case IDC_EFIT_PROJRANGE:
            case IDC_EFIT_AREA:
            case IDC_EFIT_DURATION:
            case IDC_EFIT_MAGNITUDE:
                // update cost controls
                result = 0; return true; // result = 0 to mark command message handled

            // EffectSetting - invokes SetInDialog
            case IDC_EFIT_EFFECT:
            {
                if (commandCode != CBN_SELCHANGE) break;
                // change effect setting
                DialogExtraSubwindow* extraSubwindow = (DialogExtraSubwindow*)TESDialog::GetDialogExtraData(dialog,BSExtraData::kDialogExtra_SubWindow);
                EffectSetting* mgef = (EffectSetting*)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_EFIT_EFFECT));
                if (GetHandler() && extraSubwindow) GetHandler()->CleanupDialog(dialog); // cleanup current handler dialog
                SetEffectSetting(*mgef);
                // add subwindow extra data on first pass
                if (!extraSubwindow)
                {
                    extraSubwindow = new DialogExtraSubwindow;
                    TESDialog::GetDialogExtraList(dialog)->AddExtra(extraSubwindow);
                }
                // rebuild handler subdialog if necessary
                INT newTemplate = GetHandler() ? GetHandler()->DialogTemplateID() : 0;
                if (newTemplate != extraSubwindow->dialogTemplateID)
                {
                    extraSubwindow->dialogTemplateID = newTemplate;
                    if (extraSubwindow->subwindow)
                    {
                        // destroy current subwindow, clear pointer and set new dialog template id
                        delete extraSubwindow->subwindow;
                        extraSubwindow->subwindow = 0;
                    }
                    if (newTemplate)
                    {
                        // build new subwindow
                        extraSubwindow->subwindow = new TESDialog::Subwindow;
                        extraSubwindow->subwindow->hDialog = dialog;
                        extraSubwindow->subwindow->hInstance = hModule;
                        extraSubwindow->subwindow->hContainer = GetDlgItem(dialog,IDC_EFIT_HANDLERCONTAINER);
                        RECT rect;
                        GetWindowRect(extraSubwindow->subwindow->hContainer,&rect);
                        extraSubwindow->subwindow->position.x = rect.left;
                        extraSubwindow->subwindow->position.y = rect.top;
                        ScreenToClient(dialog,&extraSubwindow->subwindow->position); 
                        // build subwindow control list (copy controls from dialog template into parent dialog)
                        TESDialog::BuildSubwindow(newTemplate,extraSubwindow->subwindow);
                    }
                }
                // set effect-dependent controls (including initialize + set handler in subdialog)
                SetInDialog(dialog);  
                result = 0; return true; // result = 0 to mark command message handled
            }

            // Overrides
            case IDC_EFIT_OVR_NAME:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_NAME));  
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_NAME),over);
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_WHOLEDESC),over);
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_OVR_WHOLEDESC),over);                
                result = 0; return true; // result = 0 to mark command message handled
            }
            case IDC_EFIT_OVR_WHOLEDESC:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_WHOLEDESC));
                ShowWindow(GetDlgItem(dialog,IDC_EFIT_WHOLEDESC),over);
                ShowWindow(GetDlgItem(dialog,IDC_EFIT_NAME),!over);
                result = 0; return true; // result = 0 to mark command message handled
            }
            case IDC_EFIT_OVR_SCHOOL:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_SCHOOL));
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_SCHOOL),over);
                result = 0; return true; // result = 0 to mark command message handled
            }
            case IDC_EFIT_OVR_FXEFFECT:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_FXEFFECT));
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_VFX),over);
                result = 0; return true; // result = 0 to mark command message handled
            }
            case IDC_EFIT_OVR_HOSTILITY:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_HOSTILITY));
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_HOSTILITY),over);
                result = 0; return true; // result = 0 to mark command message handled
            }            
            case IDC_EFIT_OVR_RESISTAV:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_RESISTAV));
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_RESISTAV),over);
                result = 0; return true; // result = 0 to mark command message handled
            }                     
            case IDC_EFIT_OVR_BASECOST: 
            case IDC_EFIT_OVR_WHOLECOST:
            { 
                HWND base = GetDlgItem(dialog,IDC_EFIT_OVR_BASECOST);
                HWND whole = GetDlgItem(dialog,IDC_EFIT_OVR_WHOLECOST);
                if ((HWND)lParam == whole && Button_GetCheck(whole)) Button_SetCheck(base,BST_UNCHECKED);
                else if ((HWND)lParam == base && Button_GetCheck(base)) Button_SetCheck(whole,BST_UNCHECKED);
                EnableWindow(GetDlgItem(dialog,IDC_EFIT_COST), Button_GetCheck(base) || Button_GetCheck(whole));
                result = 0; return true; // result = 0 to mark command message handled
            }           
            case IDC_EFIT_OVR_ICON:
            {
                bool over = Button_GetCheck(GetDlgItem(dialog,IDC_EFIT_OVR_ICON));
                EnableWindow(GetDlgItem(dialog,IDC_TXTR_PATH),over);
                ShowWindow(GetDlgItem(dialog,IDC_TXTR_IMAGE),over);
                result = 0; return true; // result = 0 to mark command message handled
            }  
        }
    }  
    
    // pass to handler
    if (GetHandler() && GetHandler()->DialogMessageCallback(dialog,uMsg,wParam,lParam,result)) return true; 

    // pass to temp TESIcon object
    if(DialogExtraIcon* extraIcon = (DialogExtraIcon*)TESDialog::GetDialogExtraData(dialog,DialogExtraIcon::kDialogExtra_Icon))
    {
        if (extraIcon->ComponentDlgMsgCallback(dialog,uMsg,wParam,lParam,result)) return true;  
    }

    // unhandled message
    return false;
}
#endif
}   //  end namespace OBME