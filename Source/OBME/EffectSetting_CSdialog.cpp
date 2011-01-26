#include "OBME/EffectSetting.h"
#include "OBME/EffectSetting.rc.h"
#include "OBME/MagicGroup.rc.h"
#include "OBME/Magic.h" // hostility constants
#include "OBME/EffectHandlers/EffectHandler.h"

#include "API/TES/TESDataHandler.h"
#include "API/Actors/ActorValues.h" // AV codes
#include "API/CSDialogs/TESDialog.h"
#include "API/CSDialogs/DialogExtraData.h"
#include "API/CSDialogs/DialogConstants.h"
#include "API/ExtraData/ExtraDataList.h"
#include "API/Settings/Settings.h"  // projectile names

#pragma warning (disable:4800) // forced typecast of int to bool

// local declaration of module handle.
extern HMODULE hModule;

namespace OBME {
#ifndef OBLIVION

// Dialog management
static const UInt32 kTabCount = 5;
static const UInt32 kTabTemplateID[kTabCount] = {IDD_MGEF_GENERAL,IDD_MGEF_DYNAMICS,IDD_MGLS,IDD_MGEF_FX,IDD_MGEF_HANDLER};
static const char* kTabLabels[kTabCount] = {"General","Dynamics","Groups","FX","Handler"};
static const char* kNoneEntry = " NONE ";
static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; // send in lieu of WM_COMMAND to avoid problems with TESFormIDListVIew::DlgProc 

// global method for returning small enumerations as booleans
inline bool BoolEx(UInt8 value) {return *(bool*)&value;}

// Helper functions
bool ExtraDataList_RemoveData(BaseExtraList* list, BSExtraData* data, bool destroy = true)
{
    // helper functions - find & remove specified data from extra list
    // unlike the methods defined by the API, this will only remove the specified item, not all items of the given type
    if (!list || !data) return false; // bad arguments
    bool found = false;
    int typeCount = 0;
    BSExtraData** prev = &list->extraList;
    for (BSExtraData* extra = list->extraList; extra; extra = extra->extraNext)
    {        
        if (extra->extraType == data->extraType) typeCount++;    // found extra with same type
        if (extra == data) 
        {            
            *prev = extra->extraNext;   // item found, remove from LL
            typeCount--; //decrement type count
            found = true;
        }
        else prev = &(**prev).extraNext;       
    }
    if (typeCount == 0) list->extraTypes[data->extraType/8] &= ~(1<<(data->extraType%8)); // clear extra list flags
    if (destroy) delete data;  // destroy extra data object
    return found;
}
TESDialog::Subwindow* InsertTabSubwindow(HWND dialog, HWND tabs, INT index)
{
    _DMESSAGE("Inserting tab %i '%s' w/ template ID %08X in tabctrl %08X of dialog %08X",index,kTabLabels[index],kTabTemplateID[index],tabs,dialog);
    // insert tab into tabs control
    TESTabControl::InsertItem(tabs,index,kTabLabels[index],(void*)index);
    // create a subwindow object for the tab
    TESDialog::Subwindow* subwindow = new TESDialog::Subwindow;
    // set subwindow position relative to parent dialog
    subwindow->hDialog = dialog;
    subwindow->hInstance = (HINSTANCE)hModule;
    subwindow->hContainer = tabs;
    RECT rect;
    GetWindowRect(tabs,&rect);
    TabCtrl_AdjustRect(tabs,false,&rect);
    subwindow->position.x = rect.left;
    subwindow->position.y = rect.top;
    ScreenToClient(dialog,&subwindow->position); 
    // build subwindow control list (copy controls from tab dialog template into parent dialog)
    TESDialog::BuildSubwindow(kTabTemplateID[index],subwindow);
    // add subwindow extra data to parent
    TESDialog::AddDialogExtraSubwindow(dialog,kTabTemplateID[index],subwindow);
    // hide tab controls
    for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
    {
        ShowWindow(node->data, false);
    }
    return subwindow;
}
void ShowTabSubwindow(HWND dialog, INT index, bool show)
{
    _DMESSAGE("Showing (%i) Tab %i '%s'",show,index,kTabLabels[index]);
    // fetch subwindow for tab    
    TESDialog::Subwindow* subwindow = 0;
    if (subwindow = TESDialog::GetSubwindow(dialog,kTabTemplateID[index]))
    {
        _DMESSAGE("Showing subwindow <%p>",subwindow);
        // show controls
        if (subwindow->hSubwindow) 
        {
            // subwindow is instantiated as an actual subwindow
            ShowWindow(subwindow->hSubwindow, show);
        }  
        for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
        {
            // iteratre through controls & show
            ShowWindow(node->data, show);
        }
    }
    // handler subwindow
    DialogExtraSubwindow* extraSubwindow = (DialogExtraSubwindow*)GetWindowLong(GetDlgItem(dialog,IDC_MGEF_HANDLER),GWL_USERDATA);
    if (kTabTemplateID[index] == IDD_MGEF_HANDLER && extraSubwindow && extraSubwindow->subwindow)
    {
        _DMESSAGE("Showing handler subwindow <%p>",extraSubwindow->subwindow);
        // this is the effect handler tab, show/hide handler subwindow as well
        if (extraSubwindow->subwindow->hSubwindow) 
        {
            // subwindow is instantiated as an actual subwindow
            ShowWindow(extraSubwindow->subwindow->hSubwindow, show);
        }  
        for (BSSimpleList<HWND>::Node* node = &extraSubwindow->subwindow->controls.firstNode; node && node->data; node = node->next)
        {
            // iteratre through controls & show
            ShowWindow(node->data, show);
        }
    }
}
// Dialog functions
void EffectSetting::InitializeDialog(HWND dialog)
{
    _DMESSAGE("Initializing MGEF dialog ...");
    HWND ctl;

    // build tabs
    ctl = GetDlgItem(dialog,IDC_MGEF_TABS);
    for (UInt32 i = 0; i < kTabCount; i++) InsertTabSubwindow(dialog,ctl,i);
    // select first tab
    TabCtrl_SetCurSel(ctl,0);
    ShowTabSubwindow(dialog,0,true);

    // GENERAL
    {
        // mgef code
        ctl = GetDlgItem(dialog,IDC_MGEF_MGEFCODE);
        Edit_LimitText(ctl,8);  // no more than 8 characters in mgefcode textbox
        // school
        ctl = GetDlgItem(dialog,IDC_MGEF_SCHOOL);
        TESComboBox::Clear(ctl);
        TESComboBox::AddItem(ctl,kNoneEntry,(void*)Magic::kSchool__MAX);
        for (int i = 0; i < Magic::kSchool__MAX; i++)
        {
            TESComboBox::AddItem(ctl,Magic::GetSchoolName(i),(void*)i);
        }
        // effect item description callback
        ctl = GetDlgItem(dialog,IDC_MGEF_DESCRIPTORCALLBACK);
        TESComboBox::PopulateWithScripts(ctl,true,false,false);
        TESComboBox::AddItem(ctl,"< Magnitude is % >",(void*)EffectSetting::kMgefFlagShift_MagnitudeIsPercent);
        TESComboBox::AddItem(ctl,"< Magnitude is Level >",(void*)EffectSetting::kMgefFlagShift_MagnitudeIsLevel);
        TESComboBox::AddItem(ctl,"< Magnitude is Feet >",(void*)EffectSetting::kMgefFlagShift_MagnitudeIsFeet);
        // counter list columns
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
        TESListView::ClearColumns(ctl);
        TESListView::ClearItems(ctl);
        SetupFormListColumns(ctl);
        SetWindowLong(ctl,GWL_USERDATA,1); // sorted by editorid
        // counter effect add list
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTCHOICES);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_EffectSetting,true,false);
    }

    // DYNAMICS
        // Cost Callback
        ctl = GetDlgItem(dialog,IDC_MGEF_COSTCALLBACK);
        TESComboBox::PopulateWithScripts(ctl,true,false,false);
        // Resistance AV
        ctl = GetDlgItem(dialog,IDC_MGEF_RESISTVALUE);
        TESComboBox::PopulateWithActorValues(ctl,true,false);
        TESComboBox::AddItem(ctl,kNoneEntry,(void*)ActorValues::kActorVal__UBOUND); // game uses 0xFFFFFFFF as an invalid resistance code
        // Hostility
        ctl = GetDlgItem(dialog,IDC_MGEF_HOSTILITY);
        TESComboBox::Clear(ctl);
        TESComboBox::AddItem(ctl,"Neutral",(void*)Magic::kHostility_Neutral);
        TESComboBox::AddItem(ctl,"Hostile",(void*)Magic::kHostility_Hostile);
        TESComboBox::AddItem(ctl,"Beneficial",(void*)Magic::kHostility_Beneficial);
        // Projectile Type
        ctl = GetDlgItem(dialog,IDC_MGEF_PROJECTILETYPE);
        TESComboBox::Clear(ctl);
        for (int i = 0; i < Magic::kProjType__MAX; i++)
        {
            TESComboBox::AddItem(ctl,Magic::projectileTypeNames[i]->value.s,(void*)i);
        }

    // GROUPS
        MagicGroupList::InitializeComponentDlg(dialog);

    // FX
        // Light, Hit + Enchant shaders
        ctl = GetDlgItem(dialog,IDC_MGEF_LIGHT);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Light,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_EFFECTSHADER);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_EffectShader,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_ENCHANTSHADER);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_EffectShader,true,true);
        // Sounds
        ctl = GetDlgItem(dialog,IDC_MGEF_CASTINGSOUND);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Sound,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_BOLTSOUND);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Sound,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_HITSOUND);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Sound,true,true);
        ctl = GetDlgItem(dialog,IDC_MGEF_AREASOUND);
        TESComboBox::PopulateWithForms(ctl,TESForm::kFormType_Sound,true,true);    

    // HANDLER
        ctl = GetDlgItem(dialog,IDC_MGEF_HANDLER);
        TESComboBox::Clear(ctl);
        UInt32 ehCode = 0;
        const char* ehName = 0;
        while (EffectHandler::GetNextHandler(ehCode,ehName))
        {
            TESComboBox::AddItem(ctl,ehName,(void*)ehCode);
        }
        SetWindowLong(ctl,GWL_USERDATA,0); // store DialogExtraSubwindow* for handler in UserData
}
bool EffectSetting::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    //_DMESSAGE("MSG {%p} WP {%p} LP {%p}",uMsg,wParam,lParam);
    HWND ctl;
    char buffer[0x50];
    void* curData;

    // parse message
    switch (uMsg)
    {
    case WM_USERCOMMAND:
    case WM_COMMAND:
    {
        UInt32 commandCode = HIWORD(wParam);
        switch (LOWORD(wParam))  // switch on control id
        {
        case IDC_MGEF_DYNAMICMGEFCODE:  // Dynamic mgefCode checkbox
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Dynamic code check clicked");  
                ctl = GetDlgItem(dialog,IDC_MGEF_MGEFCODE);
                if (IsDlgButtonChecked(dialog,IDC_MGEF_DYNAMICMGEFCODE))
                {
                    // dynamic mgefCode                
                    sprintf_s(buffer,sizeof(buffer),"%08X", IsMgefCodeDynamic(mgefCode) ? mgefCode : GetUnusedDynamicCode());
                    EnableWindow(ctl,false);
                }
                else
                {
                    // static mgefCode                
                    sprintf_s(buffer,sizeof(buffer),"%08X", IsMgefCodeDynamic(mgefCode) ? GetUnusedStaticCode() : mgefCode);
                    EnableWindow(ctl,true);
                }
                SetWindowText(ctl,buffer);
                result = 0; return true;   // retval = false signals command handled    
            }
            break;
        case IDC_MGEF_ADDCOUNTER:     // Add Counter Effect button
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Add counter effect");  
                curData = TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTCHOICES));
                if (curData) 
                {              
                    ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
                    TESListView::InsertItem(ctl,curData); // insert new item
                    ListView_SortItems(ctl,TESFormIDListView::FormListComparator,GetWindowLong(ctl,GWL_USERDATA)); // (re)sort items
                    TESListView::ForceSelectItem(ctl,TESListView::LookupByData(ctl,curData)); // force select new entry
                }
                result = 0; return true;    // retval = false signals command handled
            }
            break;
        case IDC_MGEF_REMOVECOUNTER:    // Remove Counter effect button
            if (commandCode == BN_CLICKED)
            {
                _VMESSAGE("Remove counter effect");  
                ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
                for (int i = ListView_GetNextItem(ctl,-1,LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(ctl,i-1,LVNI_SELECTED))
                {
                    ListView_DeleteItem(ctl,i);
                }
                result = 0; return true;    // retval = false signals command handled   
            }
            break;
        case IDC_MGEF_HANDLER:  // Handler selection combo box
        {
            if (commandCode == CBN_SELCHANGE)
            {
                _VMESSAGE("Handler changed");
                ctl = GetDlgItem(dialog,IDC_MGEF_HANDLER);
                // create a new handler if necessary
                bool requiresInit = false;
                UInt32 ehCode = (UInt32)TESComboBox::GetCurSelData(ctl);
                if (ehCode != GetHandler().HandlerCode()) 
                {
                    GetHandler().CleanupDialog(dialog); // cleanup dialog for current handler
                    SetHandler(MgefHandler::Create(ehCode,*this));  // set new handler
                    requiresInit = true;    // flag handler for initialization
                }
                // change dialog templates if necessary
                DialogExtraSubwindow* extraSubwindow = (DialogExtraSubwindow*)GetWindowLong(ctl,GWL_USERDATA);
                if (extraSubwindow && GetHandler().DialogTemplateID() != extraSubwindow->dialogTemplateID)
                {
                    ExtraDataList_RemoveData(TESDialog::GetDialogExtraList(dialog),extraSubwindow,true);
                    extraSubwindow = 0;
                }
                // create a new subwindow if necessary
                if (GetHandler().DialogTemplateID() && !extraSubwindow)
                {        
                    TESDialog::Subwindow* subwindow = new TESDialog::Subwindow;               
                    // set subwindow position relative to parent dialog
                    subwindow->hDialog = dialog;
                    subwindow->hInstance = (HINSTANCE)hModule;
                    subwindow->hContainer = GetDlgItem(dialog,IDC_MGEF_HANDLERCONTAINER);
                    RECT rect;
                    GetWindowRect(subwindow->hContainer,&rect);
                    subwindow->position.x = rect.left;
                    subwindow->position.y = rect.top;
                    ScreenToClient(dialog,&subwindow->position); 
                    // build subwindow control list (copy controls from tab dialog template into parent dialog)
                    TESDialog::BuildSubwindow(GetHandler().DialogTemplateID(),subwindow);
                    // add subwindow extra data to parent
                    extraSubwindow = TESDialog::AddDialogExtraSubwindow(dialog,GetHandler().DialogTemplateID(),subwindow);
                    // hide controls if handler tab is nto selected
                    int cursel = TabCtrl_GetCurSel(GetDlgItem(dialog,IDC_MGEF_TABS));
                    if (cursel < 0 || kTabTemplateID[cursel] != IDD_MGEF_HANDLER)
                    {                    
                        for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
                        {
                            ShowWindow(node->data, false);
                        }
                    }
                    // flag handler for initialization
                    requiresInit = true;
                }
                // store current dialog template in handler selection window UserData
                SetWindowLong(ctl,GWL_USERDATA,(UInt32)extraSubwindow);
                // initialize
                if (requiresInit) GetHandler().InitializeDialog(dialog);
                // set handler in dialog
                GetHandler().SetInDialog(dialog);
                result = 0; return true;  // retval = false signals command handled
            }
            break;
        }
        }
        break;
    }
    case WM_NOTIFY:  
    {
        NMHDR* nmhdr = (NMHDR*)lParam;  // extract notify message info struct
        switch (nmhdr->idFrom)  // switch on control id
        {
        case IDC_MGEF_TABS: // main tab control
            if (nmhdr->code == TCN_SELCHANGING)
            {
                // current tab is about to be unselected
                _VMESSAGE("Tab hidden");
                ShowTabSubwindow(dialog,TabCtrl_GetCurSel(nmhdr->hwndFrom),false);
                result = 0; return true;   // retval = false to allow change
            }
            else if (nmhdr->code == TCN_SELCHANGE)
            {
                // current tab was just selected
                _VMESSAGE("Tab Shown");
                ShowTabSubwindow(dialog,TabCtrl_GetCurSel(nmhdr->hwndFrom),true);  
                return true;  // no retval
            }
            break;
        }
        break;
    }
    }
    // invoke handler msg callback
    if (GetHandler().DialogMessageCallback(dialog,uMsg,wParam,lParam,result)) return true;
    // invoke group list msg callback
    if (MagicGroupList::ComponentDlgMsgCallback(dialog,uMsg,wParam,lParam,result)) return true;
    // invoke base class msg callback
    return ::EffectSetting::DialogMessageCallback(dialog,uMsg,wParam,lParam,result);
}
void EffectSetting::SetInDialog(HWND dialog)
{
    _VMESSAGE("Setting %s",GetDebugDescEx().c_str());
    if (!dialogHandle)
    {
        // first time initialization
        InitializeDialog(dialog);
        dialogHandle = dialog;
    }

    HWND ctl;
    char buffer[0x50];
    
    // GENERAL
    {
        // effect code box
        bool codeEditable = !IsMgefCodeVanilla(mgefCode) && // can't change vanilla effect codes 
                            (formFlags & TESForm::kFormFlags_FromActiveFile) > 0 && // code only editable if effect is in active file
                            (fileList.Empty() || // no override files (newly created effect) ...
                                (fileList.Count() == 1 && // .. or only a single override file, the active file
                                fileList.firstNode.data == TESDataHandler::dataHandler->fileManager.activeFile)
                             );
        ctl = GetDlgItem(dialog,IDC_MGEF_MGEFCODE);            
        sprintf_s(buffer,sizeof(buffer),"%08X", mgefCode);
        SetWindowText(ctl,buffer);
        EnableWindow(ctl,!IsMgefCodeDynamic(mgefCode) && codeEditable);
        // dynamic effect code checkbox
        CheckDlgButton(dialog,IDC_MGEF_DYNAMICMGEFCODE,IsMgefCodeDynamic(mgefCode));
        EnableWindow(GetDlgItem(dialog,IDC_MGEF_DYNAMICMGEFCODE),codeEditable);    
        // school
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_SCHOOL),(void*)school);
        // effect item description callback
        void* nameFormat = 0;  // TODO - get mgef member
        if (GetFlag(kMgefFlagShift_MagnitudeIsLevel)) nameFormat = (void*)kMgefFlagShift_MagnitudeIsLevel;
        if (GetFlag(kMgefFlagShift_MagnitudeIsFeet)) nameFormat = (void*)kMgefFlagShift_MagnitudeIsFeet;
        if (GetFlag(kMgefFlagShift_MagnitudeIsPercent)) nameFormat = (void*)kMgefFlagShift_MagnitudeIsPercent;
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_DESCRIPTORCALLBACK),nameFormat);
        // counter effects
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);
        TESListView::ClearItems(ctl);
        for (int i = 0; i < numCounters; i++)
        {
            void* countereff = EffectSetting::LookupByCode(counterArray[i]);
            if (countereff) TESListView::InsertItem(ctl,countereff);
            else _WARNING("Unrecognized counter effect '%4.4s' {%08X}",&counterArray[i],counterArray[i]);
        }
        ListView_SortItems(ctl,TESFormIDListView::FormListComparator,GetWindowLong(ctl,GWL_USERDATA)); // (re)sort items
        // counter effect add list
        TESComboBox::SetCurSel(GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTCHOICES),0);
    }

    // DYNAMICS
    {
        // Cost
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_BASECOST,baseCost);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_DISPELFACTOR,0/*TODO*/);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_COSTCALLBACK),0/*TODO*/);
        // Resistance
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_RESISTVALUE),(void*)GetResistAV());
        // Hostility
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_HOSTILITY),(void*)GetHostility());
        // Mag, Dur, Area, Range
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_MAGNITUDEEXP,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_AREAEXP,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_DURATIONEXP,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_BARTERFACTOR,barterFactor);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_ENCHANTMENTFACTOR,enchantFactor);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_TARGETFACTOR,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_TOUCHFACTOR,0/*TODO*/);
        // Projectile
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEEXP,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEMULT,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEBASE,0/*TODO*/);
        TESDialog::SetDlgItemTextFloat(dialog,IDC_MGEF_PROJECTILESPEED,projSpeed);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_PROJECTILETYPE),(void*)GetProjectileType());
    }

    // GROUPS
    MagicGroupList::SetComponentInDlg(dialog);

    // FX
    {
        // Light, Hit + Enchant shaders
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_LIGHT),light);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_EFFECTSHADER),effectShader);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_ENCHANTSHADER),enchantShader);
        // Sounds
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_CASTINGSOUND),castingSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_BOLTSOUND),boltSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_HITSOUND),hitSound);
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_AREASOUND),areaSound);   
    }

    // HANDLER
    {
        TESComboBox::SetCurSelByData(GetDlgItem(dialog,IDC_MGEF_HANDLER),(void*)GetHandler().HandlerCode()); 
        SendNotifyMessage(dialog, WM_USERCOMMAND,(CBN_SELCHANGE << 0x10) | IDC_MGEF_HANDLER,  // trigger CBN_SELCHANGED event
                                    (LPARAM)GetDlgItem(dialog,IDC_MGEF_HANDLER));
        GetHandler().SetInDialog(dialog);
    }

    // FLAGS
    for (int i = 0; i < 0x40; i++) CheckDlgButton(dialog,IDC_MGEF_FLAGEXBASE + i,GetFlag(i));

    // BASES
    ::TESFormIDListView::SetInDialog(dialog);
}
void EffectSetting::GetFromDialog(HWND dialog)
{
    _VMESSAGE("Getting %s",GetDebugDescEx().c_str());

    // call base method - retrieves most vanilla effectsetting fields, and calls GetFromDialog for components
    // will fail to retrieve AssocItem and Flags, as the control IDs for these fields have been changed
    UInt32 resistance = resistAV;
    ::EffectSetting::GetFromDialog(dialog);

    HWND ctl;
    char buffer[0x50];

    // GENERAL
    {
        // effect code
        GetWindowText(GetDlgItem(dialog,IDC_MGEF_MGEFCODE),buffer,sizeof(buffer));
        mgefCode = strtoul(buffer,0,16);  // TODO - validation?
        // effect item description callback
        UInt32 nameFormat = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_DESCRIPTORCALLBACK));
        SetFlag(kMgefFlagShift_MagnitudeIsPercent, nameFormat == kMgefFlagShift_MagnitudeIsPercent);
        SetFlag(kMgefFlagShift_MagnitudeIsLevel, nameFormat == kMgefFlagShift_MagnitudeIsLevel);
        SetFlag(kMgefFlagShift_MagnitudeIsFeet, nameFormat == kMgefFlagShift_MagnitudeIsFeet);
        // ... TODO - mgef member = (nameFormat > 0xFF) ? (DataT*)nameFormat : 0;
        // counter effects
        ctl = GetDlgItem(dialog,IDC_MGEF_COUNTEREFFECTS);        
        if (counterArray) {MemoryHeap::FormHeapFree(counterArray); counterArray = 0;} // clear current counter array
        if (numCounters = ListView_GetItemCount(ctl))
        {      
            // build array of effect codes
            counterArray = (UInt32*)MemoryHeap::FormHeapAlloc(numCounters * 4);
            gLog.Indent();
            for (int i = 0; i < numCounters; i++)
            {
                EffectSetting* mgef = (EffectSetting*)TESListView::GetItemData(ctl,i);
                counterArray[i] = mgef->mgefCode;
            }
            gLog.Outdent();
        }
    }

    // DYNAMICS
    {
        // Cost
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_DISPELFACTOR);
        /*TODO = */ TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_COSTCALLBACK));
        // Hostility
        SetHostility((UInt8)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_HOSTILITY)));
        // Mag, Dur, Area, Range
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_MAGNITUDEEXP);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_AREAEXP);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_DURATIONEXP);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_TARGETFACTOR);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_TOUCHFACTOR);
        // Projectile
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEEXP);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEMULT);
        /*TODO = */ TESDialog::GetDlgItemTextFloat(dialog,IDC_MGEF_PROJRANGEBASE);      
    }

    // GROUPS
    MagicGroupList::GetComponentFromDlg(dialog);

    // HANDLER
    {
        UInt32 ehCode = (UInt32)TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGEF_HANDLER));
        if (ehCode != GetHandler().HandlerCode()) SetHandler(MgefHandler::Create(ehCode,*this));          
        GetHandler().GetFromDialog(dialog);
    }

    // FLAGS
    for (int i = 0; i < 0x40; i++) {if (GetDlgItem(dialog,IDC_MGEF_FLAGEXBASE + i)) { SetFlag(i,IsDlgButtonChecked(dialog,IDC_MGEF_FLAGEXBASE + i)); }}

    _VMESSAGE("Gotten %s from dialog, comparison = %02i",GetDebugDescEx().c_str(),CompareTo(*TESDialog::GetFormEditParam(dialog)));
}
void EffectSetting::CleanupDialog(HWND dialog)
{
    _DMESSAGE("Cleanup MGEF Dialog");
    ::EffectSetting::CleanupDialog(dialog);
    MagicGroupList::ComponentDlgCleanup(dialog);
}
void EffectSetting::SetupFormListColumns(HWND listView)
{
    _DMESSAGE("Initializing columns for LV %p",listView);
    // call base function - clears columns & adds EditorID, FormID, & Name column
    ::EffectSetting::SetupFormListColumns(listView);
    // add EffectCode column
    TESListView::InsertColumn(listView,3,"Effect Code",80);
    // resize columns to squeeze 4th col in
    ListView_SetColumnWidth(listView,0,ListView_GetColumnWidth(listView,0)-30);
    ListView_SetColumnWidth(listView,2,ListView_GetColumnWidth(listView,2)-50);
}
void EffectSetting::GetFormListDispInfo(void* displayInfo)
{
    NMLVDISPINFO* info = (NMLVDISPINFO*)displayInfo;
    if (!info) return;
    EffectSetting* mgef = (EffectSetting*)info->item.lParam;
    if (!mgef) return;
    switch (info->item.iSubItem)
    {
    case 0: // EditorID + State flags
    {
        const char* deleted = (formFlags & TESForm::kFormFlags_Deleted) ? " D" : "";
        const char* active = (formFlags & TESForm::kFormFlags_FromActiveFile) ? (mgef->RequiresObmeMgefChunks() ? " ^" : " *") : "";
        sprintf_s(info->item.pszText,info->item.cchTextMax,"%s%s%s",mgef->GetEditorID(),active,deleted);
        break;
    }
    case 1: // FormID
        sprintf_s(info->item.pszText,info->item.cchTextMax,"%08X",mgef->formID);
        break;
    case 2: // Name
        sprintf_s(info->item.pszText,info->item.cchTextMax,"%s",mgef->name.Size() ? mgef->name.c_str() : "");
        break;
    case 3: // mgefCode
        sprintf_s(info->item.pszText,info->item.cchTextMax,"%08X",mgef->mgefCode);
        return;
    }
}
int EffectSetting::CompareInFormList(const TESForm& compareTo, int compareBy)
{
    EffectSetting* mgef = (EffectSetting*)&compareTo;
    switch (compareBy)
    {
    case 4:  
    case -4:  
        // mgefCode column - compare mgef Codes
        if (mgefCode == mgef->mgefCode) return 0;
        if (mgefCode < mgef->mgefCode) return -1;
        if (mgefCode > mgef->mgefCode) return 1;
    default:         
        // call base method - compare editorID, formID, & name
        return ::EffectSetting::CompareInFormList(compareTo,compareBy);
    }
}

#endif
}  // end namespace OBME