/* 
    Magic Groups - generalized stacking behaviors and basic interactions for EffectSettings and MagicItems
*/
#pragma once

// base classes
#include "API/TESForms/TESForm.h" // TESFormIDListView
#include "API/TESForms/BaseFormComponent.h" // additonal form components
#include "Components/ExtendedForm.h"

namespace OBME {

class MagicGroup :  public TESFormIDListView, public TESFullName
{
public:

    // members
    //     /*00/00*/ TESForm        18/24
    //     /*18/24*/ TESFullName    0C/0C
    MEMBER /*24/30*/ UInt32         extraData;  // one new member, in addition to the base clases

    // TESFormIDListView virtual method overrides
    _LOCAL /*010/034*/ virtual              ~MagicGroup();
    _LOCAL /*01C/040*/ virtual bool         LoadForm(TESFile& file);
    _LOCAL /*024/048*/ virtual void         SaveFormChunks();
    _LOCAL /*070/074*/ virtual UInt8        GetFormType();
    _LOCAL /*0B4/0B8*/ virtual void         CopyFrom(TESForm& form);
    _LOCAL /*0B8/0BC*/ virtual bool         CompareTo(TESForm& compareTo);
    #ifndef OBLIVION
    _LOCAL /*---/10C*/ virtual bool         DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result); 
    _LOCAL /*---/114*/ virtual void         SetInDialog(HWND dialog);
    _LOCAL /*---/118*/ virtual void         GetFromDialog(HWND dialog);
    _LOCAL /*---/11C*/ virtual void         CleanupDialog(HWND dialog);
    #endif

    // constructor
    _LOCAL MagicGroup();
    
    // COEF ExtendedForm component
    static ExtendedForm         extendedForm; 
    _LOCAL static TESForm*      CreateMagicGroup(); // creates a blank group

    // default instances of MagicGroup
    _LOCAL static void          CreateDefaults();
    _LOCAL static void          ClearDefaults();
    static const UInt32 kFormID_SpellLimit          = 0x400;    static MagicGroup* g_SpellLimit;
    static const UInt32 kFormID_SummonCreatureLimit = 0x401;    static MagicGroup* g_SummonCreatureLimit;
    static const UInt32 kFormID_BoundWeaponLimit    = 0x402;    static MagicGroup* g_BoundWeaponLimit;
    static const UInt32 kFormID_ShieldVFX           = 0x403;    static MagicGroup* g_ShieldVFX;
        // TODO - rest of default Magic Groups

    // CS dialog management
    #ifndef OBLIVION
    static const UInt32         kBaseMenuIdentifier = 0xCC00;   // menu identifier = base + assigned form type code
    static HWND                 dialogHandle; // handle of dialog if currently open
    _LOCAL static void          OpenDialog(); // opens dialog if it is not currently open
    #endif

    // global initialization function, called once when submodule is first loaded
    _LOCAL static void          Initialize();

private:
};

class MagicGroupList : public BaseFormComponent
{// size 0C/0C
public:

    struct GroupEntry
    {
        MagicGroup*     group;
        SInt32          weight;
        GroupEntry*     next;
        USEFORMHEAP // use form heap for class new/delete operations
    };

    // members
    //     /*00/00*/ void**         vtbl;
    MEMBER /*04/04*/ GroupEntry*    groupList;
    
    // virtual method overrides:
    _LOCAL /*000/000*/ virtual void         InitializeComponent();
    _LOCAL /*004/004*/ virtual void         ClearComponentReferences();
    _LOCAL /*008/008*/ virtual void         CopyComponentFrom(const BaseFormComponent& source);
    _LOCAL /*00C/00C*/ virtual bool         CompareComponentTo(const BaseFormComponent& compareTo) const;
    #ifndef OBLIVION                           
    _LOCAL /*---/010*/ virtual void         BuildComponentFormRefList(BSSimpleList<TESForm*>* formRefs);
    _LOCAL /*---/014*/ virtual void         RemoveComponentFormRef(TESForm& referencedForm);
    _LOCAL /*---/018*/ virtual bool         ComponentFormRefRevisionsMatch(BSSimpleList<TESForm*>* checkinList);
    _LOCAL /*---/01C*/ virtual void         GetRevisionUnmatchedComponentFormRefs(BSSimpleList<TESForm*>* checkinList, BSStringT& output);
    _LOCAL /*---/020*/ virtual bool         ComponentDlgMsgCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);                    
    _LOCAL /*---/024*/ virtual bool         IsComponentDlgValid(HWND dialog);
    _LOCAL /*---/028*/ virtual void         SetComponentInDlg(HWND dialog);
    _LOCAL /*---/02C*/ virtual void         GetComponentFromDlg(HWND dialog);
    _LOCAL /*---/030*/ virtual void         ComponentDlgCleanup(HWND dialog);
    #endif

    // methods - dialog management
    #ifndef OBLIVION                                               
    _LOCAL void                 InitializeComponentDlg(HWND dialog);
    _LOCAL static void          GetDispInfo(NMLVDISPINFO* info);
    _LOCAL static int CALLBACK  CompareListEntries(GroupEntry* entryA, GroupEntry* entryB, int sortparam);
    #endif

    // methods - serialization
    _LOCAL void                 SaveComponent();
    _LOCAL static bool          LoadComponent(MagicGroupList& groupList, TESFile& file);
    _LOCAL void                 LinkComponent();
    _LOCAL void                 UnlinkComponent();

    // methods - list management
    _LOCAL void                 AddMagicGroup(MagicGroup* group, SInt32 weight = 0);
    _LOCAL void                 RemoveMagicGroup(MagicGroup* group);
    _LOCAL void                 ClearMagicGroups();
    _LOCAL bool                 SetMagicGroupWeight(MagicGroup* group, SInt32 weight = 0);
    _LOCAL GroupEntry*          GetMagicGroup(MagicGroup* group);  
    _LOCAL UInt32               MagicGroupCount() const;

    // constructor, destructor
    _LOCAL MagicGroupList();
    _LOCAL ~MagicGroupList();
};

}   // end namespace OBME