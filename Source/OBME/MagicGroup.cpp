#include "OBME/MagicGroup.h"
#include "OBME/MagicGroup.rc.h"
#include "Components/EventManager.h"

#include "API/TES/TESDataHandler.h"
#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "Utilities/Memaddr.h"

// local declaration of module handle
// this handle is needed to extract resources embedded in this module (e.g. dialog templates)
extern HMODULE hModule;

namespace OBME {

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

    CopyGenericComponentsFrom(form); // copy all BaseFormComponent properties
    extraData = source->extraData; // copy extraData, which is specific this form class

}
bool MagicGroup::CompareTo(TESForm& compareTo)
{
    _VMESSAGE("Comparing '%s'/%p:%p @ <%p> TO '%s'/%p:%p @ <%p> ",
        GetEditorID(),GetFormType(),formID,this,compareTo.GetEditorID(),compareTo.GetFormType(),compareTo.formID,&compareTo);

    MagicGroup* source = dynamic_cast<MagicGroup*>(&compareTo);
    if (!source) return true;    // source has wrong polymorphic type

    if (CompareGenericComponentsTo(compareTo)) return true; // compare all BaseFormComponent properties
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
    const UInt32 BaseFormComponent_NoUseMethods[1] = {0x00};
#else
    const UInt32 TESForm_NoUseMethods[3] = { 0x090800D0, 0xDF783F8F, 0x00000105 };
    const UInt32 BaseFormComponent_NoUseMethods[1] = {0xD0};
#endif
memaddr TESForm_vtbl                (0x00A3BE3C,0x0093DA0C);
memaddr TESFullName_vtbl            (0x00A322A0,0x00938118);

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

        // patch up component vtbls
        memaddr nameVtbl = (UInt32)memaddr::GetObjectVtbl(dynamic_cast<TESFullName*>(this));
        _MESSAGE("Patching MagicGroup TESFullName vtbl @ <%p> ...",nameVtbl);        
        gLog.Indent();
        for (int i = 0; i < sizeof(BaseFormComponent_NoUseMethods)*0x8; i++)
        {
            if ((BaseFormComponent_NoUseMethods[i/0x20] >> (i%0x20)) & 1)
            {
                nameVtbl.SetVtblEntry(i*4,TESFullName_vtbl.GetVtblEntry(i*4));
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
MagicGroup* MagicGroup::g_ShieldVFX             = 0;
void MagicGroup::CreateDefaults()
{   
    _MESSAGE("Creating default MagicGroups ...");
    int formIDoffset = 0;
    //
    g_SpellLimit = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kBaseDefaultFormID + formIDoffset));
    if (!g_SpellLimit)
    {
        g_SpellLimit = new MagicGroup;  
        g_SpellLimit->SetFormID(kBaseDefaultFormID + formIDoffset);
        TESDataHandler::dataHandler->AddFormToHandler(g_SpellLimit);
        g_SpellLimit->name = "Single Instance Spells";
        g_SpellLimit->SetEditorID("mggpSpellLimit");
    }
    formIDoffset++;
    //
    g_SummonCreatureLimit = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kBaseDefaultFormID + formIDoffset));
    if (!g_SummonCreatureLimit)
    {
        g_SummonCreatureLimit = new MagicGroup;  
        g_SummonCreatureLimit->SetFormID(kBaseDefaultFormID + formIDoffset);
        TESDataHandler::dataHandler->AddFormToHandler(g_SummonCreatureLimit);
        g_SummonCreatureLimit->name = "Limited Summon Effects";
        g_SummonCreatureLimit->SetEditorID("mggpSummonCreatureLimit");
    }
    formIDoffset++;
    //
    g_BoundWeaponLimit = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kBaseDefaultFormID + formIDoffset));
    if (!g_BoundWeaponLimit)
    {
        g_BoundWeaponLimit = new MagicGroup;  
        g_BoundWeaponLimit->SetFormID(kBaseDefaultFormID + formIDoffset);
        TESDataHandler::dataHandler->AddFormToHandler(g_BoundWeaponLimit);
        g_BoundWeaponLimit->name = "Limited Bound Weapons";
        g_BoundWeaponLimit->SetEditorID("mggpBoundWeaponLimit");
    }
    formIDoffset++;
    //
    g_ShieldVFX = dynamic_cast<MagicGroup*>(TESForm::LookupByFormID(kBaseDefaultFormID + formIDoffset));
    if (!g_ShieldVFX)
    {
        g_ShieldVFX = new MagicGroup;  
        g_ShieldVFX->SetFormID(kBaseDefaultFormID + formIDoffset);
        TESDataHandler::dataHandler->AddFormToHandler(g_ShieldVFX);
        g_ShieldVFX->name = "Shield VFX Response";
        g_ShieldVFX->SetEditorID("mggpShieldVFX");
    }
    formIDoffset++;
}
void MagicGroup::ClearDefaults()
{
    g_SpellLimit = 0;
    g_SummonCreatureLimit = 0;
    g_BoundWeaponLimit = 0;
    g_ShieldVFX = 0;
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

} // end namespace OBME