#include "OBME/MagicItemForm.h"
#include "OBME/EffectItem.h"
#include "OBME/EffectSetting.h"

#include "API/TES/TESDataHandler.h"
#include "Utilities/Memaddr.h"

namespace OBME {

// hooks
void ReplaceVanillaEffectItems(::EffectItemList* list)  // TODO - move this function to OBME::EffectItemList once it is defined
{    
    if (!list) return;
    for (::EffectItemList::Node* effnode = &list->firstNode; effnode && effnode->data; effnode = effnode->next)
    {
        EffectItem* item = EffectItem::CreateCopyFromVanilla(effnode->data,true);    // construct an OBME::EffectItem copy 
        effnode->data =  item;     
        item->SetParentList(list);
    }
}
void SpellItem::InitHooks()
{
    _MESSAGE("Initializing ...");

    // replace any existing SpellItems with (unlinked) OBME::SpellItems     
    if (TESDataHandler::dataHandler)
    {
        for(BSSimpleList<::SpellItem*>::Node* node = &TESDataHandler::dataHandler->spellItems.firstNode; node && node->data; node = node->next)
        {
            // TODO - completely reconstruct SpellItem as an OBME::SpellItem
            BSStringT desc;
            node->data->GetDebugDescription(desc);
            _VMESSAGE("Rebuilding %s",desc.c_str());
            ReplaceVanillaEffectItems(node->data);
        }

        // TODO - move this code into similar sections for EnchantmentItem,AlchemyItem,IngredientItem,TESSigilStone
        /*for(BSSimpleList<EnchantmentItem*>::Node* node = &dataHandler->enchantmentItems.firstNode; node; node = node->next)
        {
            if (!node->data) continue;
            UpdateEffectItems(node->data);
        }
        if (dataHandler->objects)
        {
            for(TESObject* object = dataHandler->objects->first; object; object = object->next)
            {
                UpdateEffectItems(object);
            }
        }*/
    }
}

}   //  end namespace OBME