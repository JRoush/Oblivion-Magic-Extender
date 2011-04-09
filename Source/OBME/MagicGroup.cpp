#include "OBME/MagicGroup.h"
#include "OBME/MagicGroup.rc.h"
#include "Components/EventManager.h"

#include "API/TES/TESDataHandler.h"
#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "API/CSDialogs/TESFormSelection.h"
#include "Components/TESFileFormats.h"
#include "Components/FormRefCounter.h"
#include "Utilities/Memaddr.h"

// local declaration of module handle
// this handle is needed to extract resources embedded in this module (e.g. dialog templates)
extern HMODULE hModule;

namespace OBME {

//--------------------------------------------  MagicGroup  -----------------------------------------------------
// TESForm virtual method overrides
MagicGroup::~MagicGroup()
{
    _VMESSAGE("Destroying '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);
}
bool MagicGroup::LoadForm(TESFile& file)
{
    _VMESSAGE("Loading '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);

    file.InitializeFormFromRecord(*this); // initialize formID, formFlags, etc. from record header

    char buffer[0x200];
    for(UInt32 chunktype = file.GetChunkType(); chunktype; chunktype = file.GetNextChunk() ? file.GetChunkType() : 0)
    {
        // process chunk
        switch (Swap32(chunktype))
        {

        // editor id      
        case 'EDID':
            memset(buffer,0,sizeof(buffer));    // initialize buffer to zero, good practice
            file.GetChunkData(buffer,sizeof(buffer)-1); // load chunk into buffer
            _VMESSAGE("EDID chunk: '%s'",buffer);
            SetEditorID(buffer);
            break;   

        // name
        case 'FULL':            
            TESFullName::LoadComponent(*this,file); // use base class method
            _VMESSAGE("FULL chunk: '%s'",name.c_str());
            break;
        
        // extra data
        case 'DATA':
            // load all simple BaseFormComponents using LoadGenericComponents()
            TESForm::LoadGenericComponents(file,&extraData,sizeof(extraData));
            _VMESSAGE("DATA chunk: extraData %i",extraData);
            break;

        // unrecognized chunk type
        default:
            gLog.PushStyle();
            _WARNING("Unexpected chunk '%4.4s' {%08X} w/ size %08X", &chunktype, chunktype, file.currentChunk.chunkLength);
            gLog.PopStyle();
            break;

        }
        // continue to next chunk
    } 
    return true;
}
void MagicGroup::SaveFormChunks()
{
    _VMESSAGE("Saving '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);

    // initialize the global Form Record memory buffer, to which chunks are written by all 'Save' methods
    InitializeFormRecord(); 

    // Save component chunks
    TESFullName::SaveComponent(); // use base class method, saves in a FULL chunk

    // save all simple BaseFormComponents using SaveGenericComponents() in a DATA chunk
    SaveGenericComponents(&extraData,sizeof(extraData));

    // close the global form record buffer & write out to disk
    FinalizeFormRecord();
}
UInt8 MagicGroup::GetFormType()
{
    return extendedForm.FormType();
}
void MagicGroup::CopyFrom(TESForm& form)
{
    _VMESSAGE("Copying '%s'/%p:%p @ <%p> ONTO '%s'/%p:%p @ <%p> ",
        form.GetEditorID(),form.GetFormType(),form.formID,&form,GetEditorID(),GetFormType(),formID,this);

    MagicGroup* source = dynamic_cast<MagicGroup*>(&form);
    if (!source) return;    // source has wrong polymorphic type

    CopyAllComponentsFrom(form); // copy all BaseFormComponent properties
    extraData = source->extraData; // copy extraData, which is specific this form class

}
bool MagicGroup::CompareTo(TESForm& compareTo)
{
    _VMESSAGE("Comparing '%s'/%p:%p @ <%p> TO '%s'/%p:%p @ <%p> ",
        GetEditorID(),GetFormType(),formID,this,compareTo.GetEditorID(),compareTo.GetFormType(),compareTo.formID,&compareTo);

    MagicGroup* source = dynamic_cast<MagicGroup*>(&compareTo);
    if (!source) return true;    // source has wrong polymorphic type

    if (CompareAllComponentsTo(compareTo)) return true; // compare all BaseFormComponent properties
    if (extraData != source->extraData) return true;    // compare extraData, which is specific this form class

    return false; // forms are identical
}
#ifndef OBLIVION
bool MagicGroup::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_MGGP_GETTYPEINFO)
    {
        // user has clicked on the 'Get Form Type Info' button
        char buffer[0x100];
        sprintf_s(buffer,sizeof(buffer),"MagicGroup from plugin '%s' \nForm type 0x%02X \nRecord type '%4.4s'",
            extendedForm.pluginName, extendedForm.FormType(), extendedForm.ShortName());
         MessageBox(dialog,buffer,"Extended Form Type Information",0);
         return true;
    }
    // call TESFormIDListView::DialogMessageCallback to process messages general to all TESFormIDListViews
    return TESFormIDListView::DialogMessageCallback(dialog,uMsg,wParam,lParam,result);
}
void MagicGroup::SetInDialog(HWND dialog)
{
    HWND control = 0;

    if (dialogHandle == 0)
    {
        _DMESSAGE("Initializing Dialog");
        
        control = GetDlgItem(dialog,IDC_MGGP_EXTRADATA); // get handle of extraData combobox
        TESComboBox::PopulateWithActorValues(control,true,true); // populate combo with a list of actor values
        dialogHandle = dialog;  // set the global dialog handle
    }

    // call TESFormIDListView::SetInDialog to update the controls associated with all BaseFormComponents
    TESFormIDListView::SetInDialog(dialog);

    // update the extraData combo selection to match the current value of extraData
    control = GetDlgItem(dialog,IDC_MGGP_EXTRADATA);
    TESComboBox::SetCurSelByData(control,(void*)extraData);
}
void MagicGroup::GetFromDialog(HWND dialog)
{
    HWND control = 0;

    // call TESFormIDListView::GetFromDialog to update the properties associated with all BaseFormComponents
    TESFormIDListView::GetFromDialog(dialog);

    // set the value of extraData from the current combo selection
    control = GetDlgItem(dialog,IDC_MGGP_EXTRADATA);
    extraData = (UInt32)TESComboBox::GetCurSelData(control);
}
void MagicGroup::CleanupDialog(HWND dialog)
{
    dialogHandle = 0;   // clear the global dialog handle, since the window is now closed
}
#endif
// bitmask of virtual methods that need to be manually copied from vanilla vtbls (see notes in Constructor)
#ifdef OBLIVION
    const UInt32 TESForm_NoUseMethods[2] = { 0x86048400, 0x001E1FC7 };
#else
    const UInt32 TESForm_NoUseMethods[3] = { 0x09080000, 0xDF783F8F, 0x00000105 };
#endif
memaddr TESForm_vtbl                (0x00A3BE3C,0x0093DA0C);
// Constructor
MagicGroup::MagicGroup()
: TESFormIDListView(), TESFullName(), extraData(0)
{
    _VMESSAGE("Constructing '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);

    // set the form type assigned during extended form registration
    formType = extendedForm.FormType(); 
    
    if (static bool runonce = true)
    {
        // patch up TESForm vtbl, including it's BaseFormComponent section        
        memaddr thisvtbl = (UInt32)memaddr::GetObjectVtbl(this);
        _MESSAGE("Patching MagicGroup TESForm vtbl @ <%p>",thisvtbl);
        gLog.Indent();
        for (int i = 0; i < sizeof(TESForm_NoUseMethods)*0x8; i++)
        {
            if ((TESForm_NoUseMethods[i/0x20] >> (i%0x20)) & 1)
            {
                thisvtbl.SetVtblEntry(i*4,TESForm_vtbl.GetVtblEntry(i*4));
                _VMESSAGE("Patched Offset 0x%04X",i*4);
            }
        }
        gLog.Outdent();

        runonce  =  false;
    }
}
// COEF ExtendedForm component
ExtendedForm MagicGroup::extendedForm("OBME","MagicGroup","Magic Group",MagicGroup::CreateMagicGroup);  
TESForm* MagicGroup::CreateMagicGroup() { return new MagicGroup; } // method used by ExtendedForm to create new instances of this class
// default instances of MagicGroup
MagicGroup* MagicGroup::g_SpellLimit            = 0;
MagicGroup* MagicGroup::g_SummonCreatureLimit   = 0;
MagicGroup* MagicGroup::g_BoundWeaponLimit      = 0;
MagicGroup* MagicGroup::g_BoundHelmLimit        = 0;
MagicGroup* MagicGroup::g_ShieldVFX             = 0;
MagicGroup* MagicGroup::g_ActiveEffectDisplay   = 0;
void MagicGroup::CreateDefaults()
{   
    _MESSAGE("Creating default MagicGroups ...");
    //
    g_SpellLimit = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kFormID_SpellLimit));
    if (!g_SpellLimit)
    {
        g_SpellLimit = new MagicGroup;  
        g_SpellLimit->SetFormID(kFormID_SpellLimit);
        TESDataHandler::dataHandler->AddFormToHandler(g_SpellLimit);
        g_SpellLimit->name = "Single Instance Spells";
        g_SpellLimit->SetEditorID("mggpSpellLimit");
    }
    //
    g_SummonCreatureLimit = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kFormID_SummonCreatureLimit));
    if (!g_SummonCreatureLimit)
    {
        g_SummonCreatureLimit = new MagicGroup;  
        g_SummonCreatureLimit->SetFormID(kFormID_SummonCreatureLimit);
        TESDataHandler::dataHandler->AddFormToHandler(g_SummonCreatureLimit);
        g_SummonCreatureLimit->name = "Limited Summon Effects";
        g_SummonCreatureLimit->SetEditorID("mggpSummonCreatureLimit");
    }
    //
    g_BoundWeaponLimit = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kFormID_BoundWeaponLimit));
    if (!g_BoundWeaponLimit)
    {
        g_BoundWeaponLimit = new MagicGroup;  
        g_BoundWeaponLimit->SetFormID(kFormID_BoundWeaponLimit);
        TESDataHandler::dataHandler->AddFormToHandler(g_BoundWeaponLimit);
        g_BoundWeaponLimit->name = "Limited Bound Weapons";
        g_BoundWeaponLimit->SetEditorID("mggpBoundWeaponLimit");
    }
    //
    g_BoundHelmLimit = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kFormID_BoundHelmLimit));
    if (!g_BoundHelmLimit)
    {
        g_BoundHelmLimit = new MagicGroup;  
        g_BoundHelmLimit->SetFormID(kFormID_BoundHelmLimit);
        TESDataHandler::dataHandler->AddFormToHandler(g_BoundHelmLimit);
        g_BoundHelmLimit->name = "Limited Bound Helms";
        g_BoundHelmLimit->SetEditorID("mggpBoundHelmLimit");
    }
    //
    g_ShieldVFX = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kFormID_ShieldVFX));
    if (!g_ShieldVFX)
    {
        g_ShieldVFX = new MagicGroup;  
        g_ShieldVFX->SetFormID(kFormID_ShieldVFX);
        TESDataHandler::dataHandler->AddFormToHandler(g_ShieldVFX);
        g_ShieldVFX->name = "Shield VFX Response";
        g_ShieldVFX->SetEditorID("mggpShieldVFX");
    }
    //
    g_ActiveEffectDisplay = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kFormID_ActiveEffectDisplay));
    if (!g_ActiveEffectDisplay)
    {
        g_ActiveEffectDisplay = new MagicGroup;  
        g_ActiveEffectDisplay->SetFormID(kFormID_ActiveEffectDisplay);
        TESDataHandler::dataHandler->AddFormToHandler(g_ActiveEffectDisplay);
        g_ActiveEffectDisplay->name = "ActiveEffect Display";
        g_ActiveEffectDisplay->SetEditorID("mggpActiveEffectDisplay");
    }
}
void MagicGroup::ClearDefaults()
{
    g_SpellLimit = 0;
    g_SummonCreatureLimit = 0;
    g_BoundWeaponLimit = 0;
    g_BoundHelmLimit = 0;
    g_ShieldVFX = 0;
    g_ActiveEffectDisplay = 0;
}
// CS dialog management 
#ifndef OBLIVION
HWND  MagicGroup::dialogHandle = 0; // handle of open dialog window, if any
LRESULT MagicGroup_CSMenuHook(WPARAM wparam, LPARAM lparam)
{
    /*
        CSMainWindow_WMCommand event handler
        Peeks at WM_COMMAND messages sent to the main CS window
        Returns zero if message has been handled
    */
    if (LOWORD(wparam) == MagicGroup::kBaseMenuIdentifier + MagicGroup::extendedForm.FormType())
    {
        // WM_COMMAND messagesent by new menu item
        MagicGroup::OpenDialog();
        return 0;
    }
    return 1;
}
void MagicGroup::OpenDialog()
{
    /*
        Open CS dialog for editing forms of this type
    */
    if (dialogHandle == 0)  // check if dialog is already open
    {
         // define a 'FormEditParam' object, passed as an argument to the DialogProc
        TESDialog::FormEditParam param;
        param.formType = extendedForm.FormType();
        param.form = 0;
        // open a modeless dialog for editing forms of this type
        // TESFormIDListView::DlgProc is used as the DialogProc
        HWND handle = CreateDialogParam(    hModule, // handle of module in which dialog template is embedded
                                            MAKEINTRESOURCE(IDD_MGGP), // dialog template identifier
                                            TESDialog::csHandle, // handle of parent window
                                            &TESFormIDListView::DlgProc, // pointer to DialogProc
                                            (LPARAM)&param); // application specific parameter
        _DMESSAGE("Opened Dialog w/ handle %p",handle);
    }
}
#endif
// global initialization function
void MagicGroup::Initialize()
{
    // Perform one-time initialization required for this form class
    _MESSAGE("Initializing MagicGroup ...");

    // register form type with the ExtendedForm COEF component
    extendedForm.CreateDefaultForms = CreateDefaults;
    extendedForm.ClearForms = ClearDefaults;
    extendedForm.Register("MGGP");
    
    #ifndef OBLIVION

    // insert new item into CS main menu
    HMENU menu = GetMenu(TESDialog::csHandle); // get main CS menu handle
    menu = GetSubMenu(menu,5);              // get 'Gameplay' submenu handle
    MENUITEMINFO iteminfo;      
    iteminfo.cbSize = sizeof(iteminfo);        
    iteminfo.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING;
    iteminfo.fType = MFT_STRING;
    char menulabel[] = "Magic Groups ...";
    iteminfo.dwTypeData = menulabel;
    iteminfo.cch = sizeof(menulabel);
    iteminfo.wID = kBaseMenuIdentifier + MagicGroup::extendedForm.FormType();
    InsertMenuItem(menu,0x9C80,false,&iteminfo); // Insert new entry after 'Magic Effects...' item

    // register event handler with the EventManager COEF component
    EventManager::RegisterEventCallback(EventManager::CSMainWindow_WMCommand,&MagicGroup_CSMenuHook);

    #endif
}

//--------------------------------------------  MagicGroupList  -----------------------------------------------------
// virtual method overrides:
void MagicGroupList::InitializeComponent()
{
    ClearMagicGroups(); // clear group list
}
void MagicGroupList::ClearComponentReferences()
{
    ClearMagicGroups(); // clear group list
}
void MagicGroupList::CopyComponentFrom(const BaseFormComponent& source)
{
    _DMESSAGE("");
    const MagicGroupList* src = dynamic_cast<const MagicGroupList*>(&source);
    TESForm* parentForm = dynamic_cast<TESForm*>(this);
    if (!src) return;   // invalid polymorphic type
    
    // clear refs to groups currently in list, add refs to groups in source list
    if (parentForm)
    {
        for (GroupEntry* entry = groupList; entry; entry = entry->next)
        {
            if (entry->group) FormRefCounter::RemoveReference(parentForm,entry->group);
        }
        for (GroupEntry* entry = src->groupList; entry; entry = entry->next)
        {
            if (entry->group) FormRefCounter::AddReference(parentForm,entry->group);
        }
    }
    // clear existing list
    ClearMagicGroups(); 
    // copy entries from source list
    for (GroupEntry* entry = src->groupList; entry; entry = entry->next)
    {
        GroupEntry* newEntry = new GroupEntry;
        newEntry->group = entry->group;
        newEntry->weight = entry->weight;
        newEntry->next = groupList;
        groupList = newEntry;
    }
}
bool MagicGroupList::CompareComponentTo(const BaseFormComponent& compareTo) const
{
    _DMESSAGE("");
    MagicGroupList* src = const_cast<MagicGroupList*>(dynamic_cast<const MagicGroupList*>(&compareTo));
    if (!src) return true;   // invalid polymorphic type

    if (MagicGroupCount() != src->MagicGroupCount()) return true;   // mismatch in group counts
    for (GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        if (GroupEntry* sEntry = src->GetMagicGroup(entry->group))
        {
            if (entry->weight != sEntry->weight) return true;   // group weight differs
        }
        else return true;  // group not found in other list
    }

    return false;   // lists are identical
}
#ifndef OBLIVION  
void MagicGroupList::BuildComponentFormRefList(BSSimpleList<TESForm*>* formRefs)
{
    // TODO - support properly
    // ATM, though, this is never invoked because it would take an explicit call from the method of the parent form
}
void MagicGroupList::RemoveComponentFormRef(TESForm& referencedForm)
{
    MagicGroup* group = dynamic_cast<MagicGroup*>(&referencedForm);
    if (group) RemoveMagicGroup(group);
}
bool MagicGroupList::ComponentFormRefRevisionsMatch(BSSimpleList<TESForm*>* checkinList)
{
    // TODO - support properly
    // ATM, though, this is never invoked because it would take an explicit call from the method of the parent form
    return true;
}
void MagicGroupList::GetRevisionUnmatchedComponentFormRefs(BSSimpleList<TESForm*>* checkinList, BSStringT& output)
{
    // TODO - support properly
    // ATM, though, this is never invoked because it would take an explicit call from the method of the parent form
}   
void MagicGroupList::GetDispInfo(NMLVDISPINFO* info)
{
    if ((info->item.mask & LVIF_TEXT) == 0) return;  // only get display strings
    GroupEntry* entry = (GroupEntry*)info->item.lParam;
    if (!entry || !entry->group) 
    {
        // Invalid group entry
        sprintf_s(info->item.pszText,info->item.cchTextMax,"[ERROR]");
        return;
    }
    switch (info->item.iSubItem)
    {
    case 0: // Weight
        sprintf_s(info->item.pszText,info->item.cchTextMax,"%i",entry->weight);
        break;
    case 1: // Group EditorID
        sprintf_s(info->item.pszText,info->item.cchTextMax,"%s",entry->group->GetEditorID());
        break;
    case 2: // Group FormID
        sprintf_s(info->item.pszText,info->item.cchTextMax,"%08X",entry->group->formID);
        break;
    }
}
int MagicGroupList::CompareListEntries(GroupEntry* entryA, GroupEntry* entryB, int sortparam)
{
    if (!entryA || !entryB || !entryA->group || !entryB->group) return 0;
    int result = 0;
    switch (sortparam < 0 ? - sortparam - 1 : sortparam - 1)
    {
    case 0: // weight
        if (entryA->weight > entryB->weight) result = 1;
        else if (entryA->weight < entryB->weight) result = -1;
        else result = 0;
        break;
    case 1: // group editorID
        result = strcmp(entryA->group->GetEditorID(), entryB->group->GetEditorID());
        break;
    case 2: // group formID
        if (entryA->group->formID > entryB->group->formID) result = 1;
        else if (entryA->group->formID < entryB->group->formID) result = -1;
        else result = 0;
        break;
    }
    if (sortparam < 0) return -result;
    else return result;
}
void MagicGroupList::InitializeComponentDlg(HWND dialog)
{
    HWND ctl;
    // Group list
    ctl = GetDlgItem(dialog,IDC_MGLS_GROUPLIST);
    TESListView::AllowRowSelection(ctl); // allow gridlines & row selection
    ListView_SetExtendedListViewStyleEx(ctl,LVS_EX_INFOTIP,LVS_EX_INFOTIP);   // allow tooltips
    TESListView::ClearColumns(ctl); // populate with columns
    TESListView::InsertColumn(ctl,0,"Weight",50);
    TESListView::InsertColumn(ctl,1,"Group",200);
    TESListView::InsertColumn(ctl,2,"FormID",60);
    SetWindowLong(ctl,GWL_USERDATA,1);  // initialize sort param
    // Group Add List
    ctl = GetDlgItem(dialog,IDC_MGLS_ADDGROUPSEL);
    TESComboBox::PopulateWithForms(ctl,MagicGroup::extendedForm.FormType(),true,false);
    if (ComboBox_GetCount(ctl) > 0) TESComboBox::SetCurSel(ctl,0);  // select first item
}
bool MagicGroupList::ComponentDlgMsgCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{     
    HWND ctl;
    switch (uMsg)
    {
    case WM_COMMAND: 
    {
        if (HIWORD(wParam) != BN_CLICKED) break;    // handle only click commands
        switch (LOWORD(wParam))  // switch on control id
        {
        case IDC_MGLS_ADDGROUP:  // Add group
        {
            _VMESSAGE("Add Group clicked");
            if (void* curData = TESComboBox::GetCurSelData(GetDlgItem(dialog,IDC_MGLS_ADDGROUPSEL))) 
            {      
                ctl = GetDlgItem(dialog,IDC_MGLS_GROUPLIST);
                GroupEntry* entry = GetMagicGroup((MagicGroup*)curData);
                if (!entry)
                {
                    AddMagicGroup((MagicGroup*)curData,0);  // Add the group, with weight zero, to the list
                    entry = GetMagicGroup((MagicGroup*)curData);                    
                    TESListView::InsertItem(ctl,entry,0,0); // insert new item
                    ListView_SortItems(ctl,CompareListEntries,GetWindowLong(ctl,GWL_USERDATA)); // resort items
                }                
                TESListView::ForceSelectItem(ctl,TESListView::LookupByData(ctl,entry)); // force select new entry
            }
            result = 0; return true;   // retval = false signals command handled  
        }
        case IDC_MGLS_REMOVEGROUP:  // Remove group
        {
            _VMESSAGE("Remove Group clicked");  
            ctl = GetDlgItem(dialog,IDC_MGLS_GROUPLIST);
            for (int i = ListView_GetNextItem(ctl,-1,LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(ctl,i-1,LVNI_SELECTED))
            {
                GroupEntry* entry = (GroupEntry*)TESListView::GetItemData(ctl,i);
                if (entry) RemoveMagicGroup(entry->group);
                ListView_DeleteItem(ctl,i);
            }
            result = 0; return true;   // retval = false signals command handled  
        }
        }
        break;
    }
    case WM_NOTIFY:
    {
        NMHDR* nmhdr = (NMHDR*)lParam;  // extract notify message info struct
        if (nmhdr->idFrom != IDC_MGLS_GROUPLIST) break; // only handle notifications from group list
        switch (nmhdr->code)
        {
        case LVN_GETDISPINFO:   // get display info
            GetDispInfo((NMLVDISPINFO*)nmhdr);
            return true;    // no retval
        case LVN_COLUMNCLICK:   // click column header (sort)
        {
            int sortparam = ((NMLISTVIEW*)nmhdr)->iSubItem + 1;
            if (sortparam == GetWindowLong(nmhdr->hwndFrom,GWL_USERDATA)) sortparam = -sortparam;
            SetWindowLong(nmhdr->hwndFrom,GWL_USERDATA,sortparam);
            ListView_SortItems(nmhdr->hwndFrom,CompareListEntries,sortparam); // resort items
            return true;    // no retval
        }
        case LVN_ENDLABELEDIT: // enter item weight
        {
            NMLVDISPINFO* info = (NMLVDISPINFO*)nmhdr;
            if (info->item.iItem < 0 || info->item.pszText == 0) break;
            if (GroupEntry* entry = (GroupEntry*)info->item.lParam)
            {
                entry->weight = atoi(info->item.pszText);   // set weight from entered text

            }
            break;
        }
        }
        break;
    } 
    case TESDialog::WM_CANDROPSELECTION:
    {
        if ((UInt8)wParam == MagicGroup::extendedForm.FormType() && (HWND)lParam == GetDlgItem(dialog,IDC_MGLS_GROUPLIST))
        {
            result = true;  return true;    // allow drag & drop of magic groups onto group listview control
        }
        break;
    }
    case TESDialog::WM_DROPSELECTION:
    {        
        ctl = GetDlgItem(dialog,IDC_MGLS_GROUPLIST);
        if (!lParam || WindowFromPoint(*(POINT*)lParam) != ctl) break;    // bad drop point
        if (!TESFormSelection::primarySelection || !TESFormSelection::primarySelection->selections) break;  // bad selection buffer
        MagicGroup* group = dynamic_cast<MagicGroup*>(TESFormSelection::primarySelection->selections->form);
        if (!group) break;   // selection is not a MagicGroup
        _VMESSAGE("Add Group via drop"); 
        GroupEntry* entry = GetMagicGroup(group);
        if (!entry)
        {
            AddMagicGroup(group,0);  // Add the group, with weight zero, to the list
            entry = GetMagicGroup(group);                    
            TESListView::InsertItem(ctl,entry,0,0); // insert new item
            ListView_SortItems(ctl,CompareListEntries,GetWindowLong(ctl,GWL_USERDATA)); // resort items
        }                
        TESListView::ForceSelectItem(ctl,TESListView::LookupByData(ctl,entry)); // force select new entry
        TESFormSelection::primarySelection->ClearSelection(true);   // clear selection buffer
        return true;
    }
    }
    return false;
}                   
bool MagicGroupList::IsComponentDlgValid(HWND dialog)
{
    return true;
}
void MagicGroupList::SetComponentInDlg(HWND dialog)
{
    HWND ctl;
    // Group list
    ctl = GetDlgItem(dialog,IDC_MGLS_GROUPLIST);
    TESListView::ClearItems(ctl);
    for (GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        TESListView::InsertItem(ctl,entry,0,0);
    }
    ListView_SortItems(ctl,CompareListEntries,GetWindowLong(ctl,GWL_USERDATA)); // resort items
}
void MagicGroupList::GetComponentFromDlg(HWND dialog)
{
}
void MagicGroupList::ComponentDlgCleanup(HWND dialog)
{
    // no cleanup
}
#endif
// methods - serialization
struct MagicGroupListMGLSChunk
{// size 08
    UInt32 /*00*/ groupFormID; 
    SInt32 /*04*/ weight; 
};
void MagicGroupList::SaveComponent()
{
    MagicGroupListMGLSChunk mgls;
    for (GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        mgls.groupFormID = entry->group->formID;
        mgls.weight = entry->weight;
        TESForm::PutFormRecordChunkData(Swap32('MGLS'),&mgls,sizeof(mgls));
    }
}
bool MagicGroupList::LoadComponent(MagicGroupList& groupList, TESFile& file)
{
    if (file.GetChunkType() != Swap32('MGLS')) return false;    // wrong chunk type
    MagicGroupListMGLSChunk mgls;
    file.GetChunkData(&mgls,sizeof(mgls));  // load MGLS chunk
    TESFileFormats::ResolveModValue(mgls.groupFormID,file,TESFileFormats::kResType_FormID);
    groupList.AddMagicGroup((MagicGroup*)mgls.groupFormID,mgls.weight);
    return true;
}
void MagicGroupList::LinkComponent()
{
    // assume group ptrs are actually formIDs
    TESForm* parentForm = dynamic_cast<TESForm*>(this);
    GroupEntry** lastPtr = &groupList;
    for (GroupEntry* entry = groupList; entry;)
    {
        entry->group = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID((UInt32)entry->group));
        if (entry->group)
        {
            // update cross refs in CS
            if (parentForm) FormRefCounter::AddReference(parentForm,entry->group);
            // resolved successfully, move to next entry
            lastPtr = &entry->next;
            entry = entry->next;
        }
        else
        {
            BSStringT desc; if (parentForm) parentForm->GetDebugDescription(desc);
            _ERROR("%s has a MagicGroup of unrecognized type",desc.c_str());
            // invalid group formid, remove from list
            *lastPtr = entry->next;
            delete entry;
            entry = *lastPtr;
        }
    }
}
void MagicGroupList::UnlinkComponent()
{
    // revert group ptrs to formIDs
    TESForm* parentForm = dynamic_cast<TESForm*>(this);
    for (GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        if (parentForm) FormRefCounter::RemoveReference(parentForm,entry->group);
        if (entry->group) entry->group = (MagicGroup*)entry->group->formID;
    }
}
// methods - list management
void MagicGroupList::AddMagicGroup(MagicGroup* group, SInt32 weight)
{
    if (!group) return;
    for (GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        if (entry->group == group) {entry->weight = weight; return;} // group already in list
    }
    // group not found, add to head of list
    GroupEntry* entry = new GroupEntry;
    entry->group = group;
    entry->weight = weight;
    entry->next = groupList;
    groupList = entry;    
}
void MagicGroupList::RemoveMagicGroup(MagicGroup* group)
{
    GroupEntry** lastPtr = &groupList;
    for (GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        if (entry->group == group) // group found in list
        {
            *lastPtr = entry->next; // link pointer from previous node to next node
            delete entry;   // destroy node
            return; // done
        }
        lastPtr = &entry->next;
    }
    // group not found
}
void MagicGroupList::ClearMagicGroups()
{
    // delete all group entries
    for (GroupEntry* entry; entry = groupList;)
    {
        groupList = entry->next;
        delete entry;
    } 
}
bool MagicGroupList::SetMagicGroupWeight(MagicGroup* group, SInt32 weight)
{
    for (GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        if (entry->group == group) {entry->weight = weight; return true;} // group found in list
    }
    return false; // group not found
}
MagicGroupList::GroupEntry* MagicGroupList::GetMagicGroup(MagicGroup* group)
{
    for (GroupEntry* entry = groupList; entry; entry = entry->next)
    {
        if (entry->group == group) return entry; // group found in list
    }
    return 0; // group not found
}
UInt32 MagicGroupList::MagicGroupCount() const
{
    UInt32 count = 0;
    for (GroupEntry* entry = groupList; entry; entry = entry->next) count++;
    return count;
}
// constructor, destructor
MagicGroupList::MagicGroupList() : groupList(0) {}
MagicGroupList::~MagicGroupList()
{
    ClearMagicGroups(); // clear group list
}

} // end namespace OBME