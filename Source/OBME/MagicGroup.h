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
    static const UInt32         kBaseDefaultFormID  = 0x400;    // base formid for default instances
    _LOCAL static void          CreateDefaults();
    _LOCAL static void          ClearDefaults();
    static MagicGroup*          g_SpellLimit;
    static MagicGroup*          g_SummonCreatureLimit;
    static MagicGroup*          g_BoundWeaponLimit;
    static MagicGroup*          g_ShieldVFX;
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
{
public:

    struct GroupEntry
    {
        MagicGroup*     group;
        SInt32          weight;
    };

    // members
    //     void**                       vtbl;
    MEMBER BSSimpleList<GroupEntry*>    groupList;
    
    // virtual method overrides:
    _LOCAL /*000/000*/ virtual void         InitializeComponent();
    _LOCAL /*004/004*/ virtual void         ClearComponentReferences();
    _LOCAL /*008/008*/ virtual void         CopyComponentFrom(const BaseFormComponent& source);
    _LOCAL /*00C/00C*/ virtual bool         CompareComponentTo(const BaseFormComponent& compareTo);
    #ifndef OBLIVION                           
    _LOCAL /*---/014*/ virtual void         RemoveComponentFormRef(TESForm& referencedForm);
    _LOCAL /*---/020*/ virtual bool         ComponentDlgMsgCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);                    
    _LOCAL /*---/024*/ virtual bool         IsComponentDlgValid(HWND dialog);
    _LOCAL /*---/028*/ virtual void         SetComponentInDlg(HWND dialog);
    _LOCAL /*---/02C*/ virtual void         GetComponentFromDlg(HWND dialog);
    _LOCAL /*---/030*/ virtual void         ComponentDlgCleanup(HWND dialog);
    #endif

    // methods
    _LOCAL void                SaveComponent();
    _LOCAL static void         LoadComponent(MagicGroupList& groupList, TESFile& file);

};

}   // end namespace OBME