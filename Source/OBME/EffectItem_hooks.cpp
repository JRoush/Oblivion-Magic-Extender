#include "OBME/EffectItem.h"
#include "OBME/EffectSetting.h"

#include "Utilities/Memaddr.h"

// local declaration of module handle.
extern HMODULE hModule;

namespace OBME {

// memory patch addresses - allocation, construction, destruction
memaddr CreateItemForLoad_Patch                 (0x00415508,0x005674C8);    // for EffectItemList::LoadItem
memaddr CreateMarksmanDefSpell_Patch            (0x0041D5DB,0x0056FECE);    // creation of builtin marksman paralysis spell
memaddr CreatePlayerDefSpell_Patch              (0x0041D4EB,0x0056FDDE);    // creation of builtin default player spell
memaddr CopyEffectItemList_Patch                (0x00414E0C,0x00567413);    // for EffectItemList::CopyFrom
memaddr CreateNewItem_Patch                     (0x0       ,0x005681B4);    // creation of item for insert & new commands
memaddr CreateDialogTempCopy_Hook               (0x0       ,0x00566927);    // allocation & construction of temporary working copy for dialog
memaddr CreateDialogTempCopy_Retn               (0x0       ,0x0056694A);
memaddr Initialize_Hook                         (0x00414790,0x00566A10);    // replace indirect constructor
memaddr Destruct_Hook                           (0x00413BB0,0x00566690);    // replace destructor
// memory patch addresses - general methods
memaddr CopyFrom_Hook                           (0x00413FC0,0x005666E0);
memaddr SetEffectSetting_Hook                   (0x00413710,0x00566050);
// memory patch addresses - Spellmaking & Enchanting menus
    // ... yikes, there are a lot of these
// memory patch addresses - CS Dialogs
memaddr TESCS_CreateEfitDialog_PatchA           (0x0       ,0x00567F92);    // double click & edit
memaddr TESCS_CreateEfitDialog_PatchB           (0x0       ,0x0056821F);    // insert & new
memaddr BuildSchoolList_Hook                    (0x0       ,0x005654E0);
memaddr InitializeDialog_Hook                   (0x0       ,0x005664C0);
memaddr SetInDialog_Hook                        (0x0       ,0x00565DA0);
memaddr GetFromDialog_Hook                      (0x0       ,0x005657E0);
memaddr DialogMsgCallback_Hook                  (0x0       ,0x00566340);

// hooks
void _declspec(naked) CreateDialogTempCopy_Hndl(void)
{    
    _asm
    {
        push    ebp
        call    EffectItem::CreateCopy
        add     esp,4
        mov     esi,eax
        jmp     [CreateDialogTempCopy_Retn._addr]
    }
}
void EffectItem::InitHooks()
{
    _MESSAGE("Initializing ...");

    // patch allocation instances  
    // NOTE - this does not resize existing effect items on already existing objects; those objects must be rebuilt elsewhere
    CreateItemForLoad_Patch.WriteData8(sizeof(EffectItem));
    CreateMarksmanDefSpell_Patch.WriteData8(sizeof(EffectItem));
    CreatePlayerDefSpell_Patch.WriteData8(sizeof(EffectItem));
    CopyEffectItemList_Patch.WriteData8(sizeof(EffectItem));
    CreateNewItem_Patch.WriteData8(sizeof(EffectItem));
    CreateDialogTempCopy_Hook.WriteRelJump(&CreateDialogTempCopy_Hndl);  

    // hook construction & destruction methods
    Initialize_Hook.WriteRelJump(memaddr::GetPointerToMember(&Initialize));
    Destruct_Hook.WriteRelJump(memaddr::GetPointerToMember(&Destruct));

    // hook basic parameter methods
    CopyFrom_Hook.WriteRelJump(memaddr::GetPointerToMember(&CopyFrom));
    SetEffectSetting_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetEffectSetting));

    // patch creation of EFIT dialog in CS - replace CS module instance with local module instance
    TESCS_CreateEfitDialog_PatchA.WriteData32((UInt32)&hModule);
    TESCS_CreateEfitDialog_PatchB.WriteData32((UInt32)&hModule);

    // hook CS dialog methods
    #ifndef OBLIVION
    BuildSchoolList_Hook.WriteRelJump(memaddr::GetPointerToMember(&BuildSchoolList));
    InitializeDialog_Hook.WriteRelJump(memaddr::GetPointerToMember(&InitializeDialog));
    SetInDialog_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetInDialog));
    GetFromDialog_Hook.WriteRelJump(memaddr::GetPointerToMember(&GetFromDialog));
    DialogMsgCallback_Hook.WriteRelJump(memaddr::GetPointerToMember(&HandleDialogEvent));
    #endif

}

}   //  end namespace OBME