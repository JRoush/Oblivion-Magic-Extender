#include "OBME/EffectItem.h"
#include "OBME/EffectSetting.h"
#include "OBME/Magic.h"

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
memaddr CompareTo_Hook                          (0x00414190,0x00566150);
memaddr SetEffectSetting_Hook                   (0x00413710,0x00566050);
memaddr GetMagickaCost_Hook                     (0x00413810,0x00565730);
memaddr GetMagickaCostForCaster_Hook            (0x00413890,0x0);
// memory path addresses - overrides
memaddr GetEffectName_Hook                      (0x004139F0,0x00565D00);
memaddr SetEffectName_Hook                      (0x00413960,0x00565C70);    // only call not overridden elsewhere is CS Import Names function
memaddr GetSchool_Hook                          (0x00412F20,0x00564E80);
memaddr SetSchool_Hook                          (0x0       ,0x00564E60);    // all calls overridden, included only for completeness
memaddr GetFXEffectCode_Hook                    (0x00412CB0,0x00564DF0);
memaddr SetFXEffectCode_Hook                    (0x0       ,0x00564E00);    // all calls overridden, included only for completeness
memaddr GetIsHostile_Hook                       (0x00413470,0x00565280);    
memaddr SetIsHostile_Hook                       (0x0       ,0x00564EA0);    // all calls overridden, included only for completeness
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
memaddr CleanupDialog_Hook                      (0x0       ,0x00566979);    // just before destrction of temp working item in EffectItem dlg proc

// hooks
void _declspec(naked) SetEffectItemName_Hndl(void)
{
    _asm
    {
        push    [esp + 4]   // push string buffer of BSStringT
        call    EffectItem::SetEffectName   // set name via string buffer
        retn    8
    }
}
void _declspec(naked) GetFXEffectCode_Hndl(void)
{
    EffectItem* item;
    UInt32*     code;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        lea     eax, [ebp + 0x1C]    // address of eax stored on stack
        mov     code, eax
   }
    if (EffectSetting* mgef = item->GetFXEffect()) *code = mgef->mgefCode;
    else *code = EffectSetting::kMgefCode_None;
    _asm
    {
        mov     esp, ebp
        popad
        retn
    }
}
void _declspec(naked) GetIsHostile_Hndl(void)
{
    EffectItem* item;
    bool*       host;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        lea     eax, [ebp + 0x1C]    // address of eax stored on stack
        mov     host, eax
    }
    *host = (item->GetHostility() == Magic::kHostility_Hostile);
    _asm
    {
        mov     esp, ebp
        popad
        retn
    }
}
void _declspec(naked) SetIsHostile_Hndl(void)
{
    EffectItem* item;
    bool        host;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        mov     eax, [ebp + 0x24]    // address of first argument
        mov     host, al
    }
    item->SetHostility(host ? Magic::kHostility_Hostile : Magic::kHostility_Neutral);
    _asm
    {
        mov     esp, ebp
        popad
        retn    4
    }
}
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
void _declspec(naked) CleanupDialog_Hndl(void)
{
    EffectItem* item;
    HWND dialog;
    _asm
    {
        pushad
        mov     ebp, esp
        sub     esp, __LOCAL_SIZE
        mov     item, ecx
        mov     dialog, edi
    }
    item->CleanupDialog(dialog);
    item->Destruct();   // overwritten code: destruct temp item
    _asm
    {
        mov     esp, ebp
        popad
        retn
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
    CompareTo_Hook.WriteRelJump(memaddr::GetPointerToMember(&CompareTo));
    SetEffectSetting_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetEffectSetting));

    // hook override methods
    GetEffectName_Hook.WriteRelJump(memaddr::GetPointerToMember(&GetEffectName));
    SetEffectName_Hook.WriteRelJump(&SetEffectItemName_Hndl);
    GetSchool_Hook.WriteRelJump(memaddr::GetPointerToMember(&GetSchool));
    SetSchool_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetSchool));
    GetFXEffectCode_Hook.WriteRelJump(&GetFXEffectCode_Hndl);
    SetFXEffectCode_Hook.WriteRelJump(memaddr::GetPointerToMember(&SetFXEffect));
    GetIsHostile_Hook.WriteRelJump(&GetIsHostile_Hndl);
    SetIsHostile_Hook.WriteRelJump(&SetIsHostile_Hndl);

    // hook calculated property methods
    GetMagickaCost_Hook.WriteRelJump(memaddr::GetPointerToMember(&MagickaCost));
    GetMagickaCostForCaster_Hook.WriteRelJump(memaddr::GetPointerToMember(&MagickaCostForCaster));

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
    CleanupDialog_Hook.WriteRelCall(&CleanupDialog_Hndl);
    #endif

}

}   //  end namespace OBME