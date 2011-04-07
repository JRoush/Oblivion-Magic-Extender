#include "OBME/EffectItemList.h"
#include "OBME/EffectItem.h"
#include "OBME/EffectSetting.h"
#include "OBME/Magic.h"

#include "Utilities/Memaddr.h"

namespace OBME {

// methods - add & remove items
void EffectItemList::RemoveEffect(const EffectItem* item)
{
    EffectItem* itm = const_cast<EffectItem*>(item);
    if (itm && Find(itm) && itm->GetHostility() == Magic::kHostility_Hostile) hostileCount--;  // increment cached hostile effect count
    Remove(itm);   // remove item entry from list
    itm->SetParentList(0);    // clear item parent
}
void EffectItemList::AddEffect(EffectItem* item)
{
    item->SetParentList(this);    // set item parent
    PushBack(item); // append to effect list
    if (item->GetHostility() == Magic::kHostility_Hostile) hostileCount++;  // increment cached hostile effect count
}
void EffectItemList::ClearEffects()
{   
    for (Node* node = &firstNode; node; node = node->next)
    {
        if (node->data) delete (OBME::EffectItem*)node->data; // destroy + deallocate effect item        
    }
    Clear();  // clear list (destroy nodes)
    hostileCount = 0;   // reset cached hostile count
}

// methods - other effect lists
void EffectItemList::CopyEffectsFrom(const EffectItemList& copyFrom)
{
    for (Node* node = &firstNode; node; node = node->next)
    {
        if (EffectItem* item = (EffectItem*)node->data) 
        {
            item->Unlink(); // clear any form refs from effectitem
            delete item; // destroy + deallocate effect item     
        }
    }
    Clear();  // clear list (destroy nodes), and clear hostile count
    for (const Node* node = &copyFrom.firstNode; node; node = node->next)
    {
        if (EffectItem* item = (EffectItem*)node->data) 
        {   
            EffectItem* nItem = new EffectItem(*item->GetEffectSetting());  // create default item for specified effect setting
            nItem->SetParentList(this); 
            nItem->CopyFrom(*item); // copying with parent set will update refs in CS 
            AddEffect(nItem); // add new item to list
        }
    }
}

// memory patch addresses
memaddr RemoveEffect_Hook                   (0x00414BC0,0x00566F50);
memaddr AddEffect_Hook                      (0x00414B90,0x00566F20);
memaddr ClearEffects_Hook                   (0x00414C70,0x005670F0);
memaddr CopyEffectsFrom_Hook                (0x00414DC0,0x005673D0);

// hooks & debugging
void EffectItemList::InitHooks()
{
    _MESSAGE("Initializing ...");

    // hook methods
    RemoveEffect_Hook.WriteRelJump(memaddr::GetPointerToMember(&RemoveEffect));
    AddEffect_Hook.WriteRelJump(memaddr::GetPointerToMember(&AddEffect));
    ClearEffects_Hook.WriteRelJump(memaddr::GetPointerToMember(&ClearEffects));
    CopyEffectsFrom_Hook.WriteRelJump(memaddr::GetPointerToMember(&CopyEffectsFrom));

}

}   // end namespace OBME