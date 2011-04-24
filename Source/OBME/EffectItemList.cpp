#include "OBME/EffectItemList.h"
#include "OBME/EffectItem.h"
#include "OBME/EffectItem.rc.h"
#include "OBME/EffectSetting.h"
#include "OBME/MagicItemEx.h"
#include "OBME/Magic.h"
#include "OBME/CSDialogUtilities.h"

#include "API/CSDialogs/TESDialog.h"
#include "API/CSDialogs/DialogConstants.h"
#include "API/CSDialogs/DialogExtraData.h"
#include "API/CSDialogs/TESFormSelection.h"

#include "Utilities/Memaddr.h"

namespace OBME {

// methods - parents & siblings
TESForm* EffectItemList::GetParentForm() const { return dynamic_cast<TESForm*>((::EffectItemList*)this); }
MagicItemEx* EffectItemList::GetMagicItemEx() const { return dynamic_cast<MagicItemEx*>((::EffectItemList*)this); }

// methods - add, remove, & iterate items
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
UInt8 EffectItemList::GetIndexOfEffect(const EffectItem* item) const
{
    int c = 0;
    for (const Node* node = &firstNode; node; node = node->next)
    {
        if (node->data == item) return c;   // found effect item
        c++;
    }
    return -1;
}
void EffectItemList::InsertBefore(EffectItem* item, EffectItem* position)
{
    if (!item) return;
    if (!position || position == firstNode.data) PushFront(item); // prepend to start of list
    else
    {
        // search list for position
        Node* prevNode = &firstNode;
        for (Node* node = firstNode.next; node; node = node->next)
        {
            if (node->data == position)
            {
                // insert into list
                node = new Node(item);
                node->next = prevNode->next;
                prevNode->next = node;
                break;
            }
            prevNode = node;
        }
        if (!prevNode->next) PushFront(item); // prepend to start of list
    }
    item->SetParentList(this);    // set item parent
    if (item->GetHostility() == Magic::kHostility_Hostile) hostileCount++;  // increment cached hostile effect count
}
void EffectItemList::InsertAfter(EffectItem* item, EffectItem* position)
{
    if (!item) return;
    if (!position || Empty()) PushBack(item); // append to end of list
    else
    {
        // search list for position
        bool found = false;
        for (Node* node = &firstNode; node; node = node->next)
        {
            if (node->data == position)
            {
                // insert into list
                found = true;
                Node* tmp = new Node(item);
                tmp->next = node->next;
                node->next = tmp;
                break;
            }
        }
        if (!found) PushBack(item); // append to end of list
    }
    item->SetParentList(this);    // set item parent
    if (item->GetHostility() == Magic::kHostility_Hostile) hostileCount++;  // increment cached hostile effect count
}
void EffectItemList::MoveUp(EffectItem* item)
{
    if (!item || item == firstNode.data) return;    // invalid item, or already at front of list
    Node* prevNode = &firstNode;
    for (Node* node = firstNode.next; node; node = node->next)
    {
        if (node->data == item)
        {
            // swap node data
            node->data = prevNode->data;
            prevNode->data = item;
            break;
        }
        prevNode = node;
    }
}
void EffectItemList::MoveDown(EffectItem* item)
{
    if (!item) return;  // invalid item
    for (Node* node = &firstNode; node; node = node->next)
    {
        if (node->data == item && node->next)
        {
            // swap node data
            node->data = node->next->data;
            node->next->data = item;
            break;
        }
    }
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
    Clear();  // clear list (destroy nodes)
    hostileCount = 0;   // reset cached hostile count
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

void EffectItemList::CopyFromVanilla(::EffectItemList* source, bool clearList)
{
    if (!source) return;
    ClearEffects();
    for (Node* node = &source->firstNode; node && node->data; node = node->next)
    {
        EffectItem* item = EffectItem::CreateFromVanilla(node->data,clearList);    // construct an OBME::EffectItem copy 
        AddEffect(item);    // append new item to this list
    }
    if (clearList) 
    {
        // clear now-empty source effect list
        source->Clear(); 
        source->hostileCount = 0;
    }
}

// methods - serialization
void EffectItemList::LinkEffects()
{
    for (Node* node = &firstNode; node && node->data; node = node->next) ((EffectItem*)node->data)->Link();
}
void EffectItemList::UnlinkEffects() 
{
    for (Node* node = &firstNode; node && node->data; node = node->next) ((EffectItem*)node->data)->Unlink();
}
void EffectItemList::ReplaceMgefCodeRef(UInt32 oldMgefCode, UInt32 newMgefCode)
{
    for (Node* node = &firstNode; node && node->data; node = node->next) ((EffectItem*)node->data)->ReplaceMgefCodeRef(oldMgefCode,newMgefCode);
}
void EffectItemList::RemoveFormReference(TESForm& form)
{
    // iterate through effects
    for (Node* node = &firstNode; node && node->data;) 
    {
        EffectItem* item = (EffectItem*)node->data;
        node = node->next;
        if (item->GetEffectSetting() == &form)
        {
            // specified form is this effect setting, remove effect from list
            RemoveEffect(item);
            delete item;
        }
        else 
        {
            // call corresponding effect item method
            item->RemoveFormReference(form);
        }
    }
}

// CS dialogs
#ifndef OBLIVION
// Column constants
static const enum ColumnIndicies
{
    kEfitCol_Index = 0,
    kEfitCol_EffectCode,
    kEfitCol_EffectName,
    kEfitCol_Magnitude,
    kEfitCol_Area,
    kEfitCol_Duration,
    kEfitCol_Range,
    kEfitCol_Cost,
    kEfitCol_School,
    kEfitCol_Overrides,
};
// DragDropItem struct for effect item drag & drop
struct DragDropItem
{
    // members
    EffectItem* item;
    POINT       point;
    HWND        target;
    HWND        msgTarget;
    // methods
    DragDropItem(EffectItem* nItem)
    {
        item = nItem;
        GetCursorPos(&point);
        target = WindowFromPoint(point);
        msgTarget = GetParent(target);
        if (!msgTarget || msgTarget == TESDialog::csHandle) msgTarget = target;
    }
    DragDropItem()
    {
        item = 0;
        point.x = point.y = 0;
        target = msgTarget = 0;
    }
};
static DragDropItem dragDropItem;
// methods - CS dialogs
bool EffectItemList::EffectListDlgMsgCallback(HWND dialog, int uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    switch (uMsg)
    {

    case WM_USERCOMMAND:
    case WM_COMMAND:
    {
        UInt32 commandCode = HIWORD(wParam);
        switch (LOWORD(wParam)) // switch on control id
        {            
        case IDM_EILS_DUPLICATE:
        {
            if (DialogExtraPopupMenu* popupExtra = TESDialog::GetDialogExtraPopupMenu(dialog,IDR_EILS_POPUPMENU))
            {
                if (popupExtra->dialogItemID == IDC_EILS_EFFECTLIST)
                {
                    NMLVKEYDOWN keydown;
                    keydown.hdr.code = LVN_KEYDOWN;
                    keydown.hdr.hwndFrom = GetDlgItem(dialog,IDC_EILS_EFFECTLIST);
                    keydown.hdr.idFrom = IDC_EILS_EFFECTLIST;
                    keydown.wVKey = 'D';
                    keydown.flags = 0;
                    SendMessage(dialog,WM_NOTIFY,(WPARAM)IDC_EILS_EFFECTLIST,(LPARAM)&keydown); // send 'D' keydown message
                }
            }            
            result = 0; return true;
        }
        case IDM_EILS_MOVEUP:
        case IDM_EILS_MOVEDOWN:
        {
            if (DialogExtraPopupMenu* popupExtra = TESDialog::GetDialogExtraPopupMenu(dialog,IDR_EILS_POPUPMENU))
            {
                HWND ctl = GetDlgItem(dialog,IDC_EILS_EFFECTLIST);
                EffectItem* item = (EffectItem*)TESListView::GetCurSelData(ctl);                
                if (popupExtra->dialogItemID == IDC_EILS_EFFECTLIST && item) 
                {
                    if (LOWORD(wParam) == IDM_EILS_MOVEUP) MoveUp(item);
                    else MoveDown(item);
                    ListView_SortItems(ctl,&EffectItemComparator,GetWindowLong(ctl,GWL_USERDATA));
                }
            }
            result = 0; return true;
        }
        }
        break;
    }

    case WM_NOTIFY:
    {
        NMHDR* nmhdr = (NMHDR*)lParam;  // extract notify message info struct
        if (nmhdr->idFrom != IDC_EILS_EFFECTLIST) return false;
        switch (nmhdr->code)  // switch on notification code
        {
        case LVN_BEGINDRAG:
        {
            NMLISTVIEW* lv = (NMLISTVIEW*)lParam;
            if (lv->iItem < 0) return false;    // invalid effect item
            dragDropItem = DragDropItem((EffectItem*)TESListView::GetItemData(lv->hdr.hwndFrom,lv->iItem));
            _VMESSAGE("Begin Drag for item %p",dragDropItem.item);
            SetCursor(LoadCursor(TESDialog::csInstance,MAKEINTRESOURCE(184)));
            SetCapture(dialog);
            result = 0; return true;
        }
        case NM_CUSTOMDRAW:
        {
            ListViewInsertionMark* mark = (ListViewInsertionMark*)GetWindowLong(ListView_GetHeader(nmhdr->hwndFrom),GWL_USERDATA);
            if (mark && mark->CustomDraw(lParam,result)) return true;   // pass custom draw message to insertion mark object
            break;
        }
        case LVN_KEYDOWN:
        {
            NMLVKEYDOWN* keydown = (NMLVKEYDOWN*)lParam;
            if (keydown && keydown->wVKey == 'D' && (GetMaxEffectCount() == 0 || GetMaxEffectCount() > Count()))
            {
                if (EffectItem* source = (EffectItem*)TESListView::GetCurSelData(keydown->hdr.hwndFrom))
                {
                    _VMESSAGE("Duplicating effect item %p ...",source);
                    EffectItem* item = new EffectItem(*source);
                    AddEffect(item);
                    TESListView::InsertItem(keydown->hdr.hwndFrom,item);  
                    ListView_SortItems(keydown->hdr.hwndFrom,&EffectItemComparator,GetWindowLong(keydown->hdr.hwndFrom,GWL_USERDATA));
                    TESListView::ForceSelectItem(keydown->hdr.hwndFrom,TESListView::LookupByData(keydown->hdr.hwndFrom,item)); // force select new entry                    
                    result = kUpdateAggregateValues;    // trigger update of aggregate values
                }
                else result = 0;
                return true;
            }
            break;
        }
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        if (!dragDropItem.item) return false; // dragdrop not in progress 
        DragDropItem tmpDragDrop(dragDropItem.item);
        if (dragDropItem.target && dragDropItem.target != tmpDragDrop.target) SendMessage(dragDropItem.msgTarget,WM_QUERYDROPITEM,-1,0);
        dragDropItem = tmpDragDrop;
        switch (SendMessage(dragDropItem.msgTarget,WM_QUERYDROPITEM,(WPARAM)dragDropItem.item,(LPARAM)&dragDropItem.point))
        {
        case -1:            
            SetCursor(LoadCursor(TESDialog::csInstance,MAKEINTRESOURCE(32512)));        
            break;
        case 0:
            SetCursor(LoadCursor(TESDialog::csInstance,MAKEINTRESOURCE(184)));
            break;
        default:
            SetCursor(LoadCursor(TESDialog::csInstance,MAKEINTRESOURCE(186)));
            break;
        }
        result = 0; return true;
    }

    case WM_LBUTTONUP:
    {
        if (!dragDropItem.item) return false; // dragdrop not in progress
        DragDropItem tmpDragDrop(dragDropItem.item);
        if (dragDropItem.target && dragDropItem.target != tmpDragDrop.target) SendMessage(dragDropItem.msgTarget,WM_QUERYDROPITEM,-1,0);
        dragDropItem = tmpDragDrop;
        SendMessage(dragDropItem.msgTarget,WM_DROPITEM,(WPARAM)dragDropItem.item,(LPARAM)&dragDropItem.point);
        SetCursor(LoadCursor(TESDialog::csInstance,MAKEINTRESOURCE(32512)));
        ReleaseCapture();
        dragDropItem.item = 0;
        result = 0; return true;
    }

    case WM_QUERYDROPITEM:
    case WM_DROPITEM:
    {
        POINT* pnt = (POINT*)lParam;
        HWND ctl = GetDlgItem(dialog,IDC_EILS_EFFECTLIST);
        EffectItem* dropItem = (EffectItem*)wParam;
        ListViewInsertionMark* mark = (ListViewInsertionMark*)GetWindowLong(ListView_GetHeader(ctl),GWL_USERDATA);
        if (mark) mark->ClearMark();    // clear current insertion mark        
        if (pnt && dropItem && WindowFromPoint(*pnt) == ctl)
        {              
            int index = -1;
            bool after = false;
            EffectItem* insertItem = 0;
            ScreenToClient(ctl,pnt);
            if (mark) mark->GetMarkIndexAt(*pnt,index,after);
            if (!ValidateEffectForDialog(*dropItem)) result = 0; // effect doesn't pass list filter
            else if (dropItem->GetParentList() != this && GetMaxEffectCount() > 0 && GetMaxEffectCount() <= Count()) result = 0; // too many effects
            else if (index < 0) 
            {
                if (dropItem->GetParentList() != this)
                {
                    insertItem = 0;
                    after = true;
                    result = 1; // drop point not over a valid item, allow appending if different list
                }
                else result = 0; // drop point not over valid point in same list, no drop
            }
            else if (dropItem == (EffectItem*)TESListView::GetItemData(ctl,index)) 
            {
                result = 0;  // drop point over current drop item - can't drop
            }
            else
            {
                insertItem = (EffectItem*)TESListView::GetItemData(ctl,index);
                if (mark && uMsg == WM_QUERYDROPITEM) mark->SetMark(index,after);
                result = -1; // drop over a valid item, allow insertion
            }
            if (result && uMsg == WM_DROPITEM)
            {   
                _VMESSAGE("Dropped item %p on (%p:%i)",dropItem,insertItem,after);
                if (dropItem->GetParentList() == this) 
                {
                    RemoveEffect(dropItem); // move item within list
                }
                else
                {
                    dropItem = new EffectItem(*dropItem); // insert duplicate item
                    TESListView::InsertItem(ctl,dropItem);  
                }
                if (after) InsertAfter(dropItem,insertItem);
                else InsertBefore(dropItem,insertItem);
                ListView_SortItems(ctl,&EffectItemComparator,GetWindowLong(ctl,GWL_USERDATA));
                TESListView::ForceSelectItem(ctl,TESListView::LookupByData(ctl,dropItem)); // force select entry
                result = kUpdateAggregateValues;    // trigger update of aggregate values
            }        
        }
        else result = 0;
        return true;
    }

    case TESDialog::WM_CANDROPSELECTION:
    case TESDialog::WM_DROPSELECTION:
    {        
        HWND ctl = (uMsg == TESDialog::WM_DROPSELECTION) ? WindowFromPoint(*(POINT*)lParam) : (HWND)lParam;
        if (!lParam || ctl != GetDlgItem(dialog,IDC_EILS_EFFECTLIST)) break;    // bad drop point
        result = false;
        if (!TESFormSelection::primarySelection || !TESFormSelection::primarySelection->selections) return true;  // bad selection buffer
        EffectSetting* mgef = dynamic_cast<EffectSetting*>(TESFormSelection::primarySelection->selections->form);
        if (!mgef) return true;   // selection is not an effect setting          
        EffectItem* item = new EffectItem(*mgef);
        result = ValidateEffectForDialog(*item) && (GetMaxEffectCount() == 0 || GetMaxEffectCount() > Count());
        if (result && uMsg == TESDialog::WM_DROPSELECTION)
        {
            _VMESSAGE("EffectItem added via drop: %s",mgef->GetDebugDescEx().c_str());
            AddEffect(item);
            TESListView::InsertItem(ctl,item); // insert new item
            ListView_SortItems(ctl,&EffectItemComparator,GetWindowLong(ctl,GWL_USERDATA));
            TESListView::ForceSelectItem(ctl,TESListView::LookupByData(ctl,item)); // force select new entry
            TESFormSelection::primarySelection->ClearSelection(true);   // clear selection buffer
            result = kUpdateAggregateValues;    // trigger update of aggregate values
        }
        else delete item;
        return true;
    }

    }

    // pass message to vanilla EffectItemList handler
    return ::EffectItemList::EffectListDlgMsgCallback(dialog,uMsg,wParam,lParam,result);
}
void EffectItemList::InitializeDialogEffectList(HWND listView)
{
    // setup columns
    TESListView::AllowRowSelection(listView);
    TESListView::ClearColumns(listView);
    RECT rect;
    GetClientRect(listView,&rect);
    float width = rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL) - 5;  // duplicates vanilla width calc

    // populate columns
    TESListView::InsertColumn(listView,kEfitCol_Index,      "#"            ,20,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_EffectCode, "Effect Code"  ,5,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_EffectName, "Effect Name"  ,130,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_Magnitude,  "Magnitude"    ,35,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_Area,       "Area"         ,35,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_Duration,   "Duration"     ,35,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_Range,      "Range"        ,100,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_Cost,       "Cost"         ,50,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_School,     "Magic School" ,75,LVCFMT_CENTER);
    TESListView::InsertColumn(listView,kEfitCol_Overrides,  "Has Overrides",20,LVCFMT_CENTER);
    
    // expand name column to fill available client area
    for (int c = 0; c < Header_GetItemCount(ListView_GetHeader(listView)); c++) width -= ListView_GetColumnWidth(listView,c);
    if (width > 0) ListView_SetColumnWidth(listView,kEfitCol_EffectName,ListView_GetColumnWidth(listView,kEfitCol_EffectName) + width);

    // allocate an insertion mark object & attach to window
    ListViewInsertionMark* mark = new ListViewInsertionMark(listView);
    SetWindowLong(ListView_GetHeader(listView),GWL_USERDATA,(LONG)mark);
}
void EffectItemList::CleanupDialogEffectList(HWND listView)
{
    // destroy insertion mark object
    ListViewInsertionMark* mark = (ListViewInsertionMark*)GetWindowLong(ListView_GetHeader(listView),GWL_USERDATA);
    if (mark) delete mark;
}
void EffectItemList::GetEffectDisplayInfo(void* displayInfo)
{
    if (!displayInfo) return;   // bad info struct
    NMLVDISPINFO& dInfo = *(NMLVDISPINFO*)displayInfo;
    if (~dInfo.item.mask & LVIF_TEXT) return;   // requested info is not text
    if (!dInfo.item.lParam) return;  // bad effect item
    EffectItem& item = *(EffectItem*)dInfo.item.lParam;
    switch (dInfo.item.iSubItem) // switch on column index
    {
    case kEfitCol_Index:
    {
        // TODO - add indicator for OBME chunks
        const EffectItemList* list = item.GetParentList();
        if (list) sprintf_s(dInfo.item.pszText,dInfo.item.cchTextMax,"%i",list->GetIndexOfEffect(&item));
        break;
    }
    case kEfitCol_EffectCode:
        sprintf_s(dInfo.item.pszText,dInfo.item.cchTextMax,"%08X",item.mgefCode);
        break;
    case kEfitCol_EffectName:
        strcpy_s(dInfo.item.pszText,dInfo.item.cchTextMax,item.GetEffectName().c_str());    // TODO - replace with GetDisplayName() or equivalent
        break;
    case kEfitCol_Magnitude:
        sprintf_s(dInfo.item.pszText,dInfo.item.cchTextMax,"%i",item.GetMagnitude());
        break;
    case kEfitCol_Area:
        sprintf_s(dInfo.item.pszText,dInfo.item.cchTextMax,"%i",item.GetArea());
        break;
    case kEfitCol_Duration:
        sprintf_s(dInfo.item.pszText,dInfo.item.cchTextMax,"%i",item.GetDuration());
        break;
    case kEfitCol_Range:
    {
        SInt32 projRange = item.GetProjectileRange();
        const char* range = Magic::GetRangeName(item.GetRange());
        if (projRange >= 0) sprintf_s(dInfo.item.pszText,dInfo.item.cchTextMax,"%s (%i ft)",range,projRange);
        else if (item.GetRange() == Magic::kRange_Target) sprintf_s(dInfo.item.pszText,dInfo.item.cchTextMax,"%s (No Limit)",range);
        else strcpy_s(dInfo.item.pszText,dInfo.item.cchTextMax,range);
        break;
    }
    case kEfitCol_Cost:
        sprintf_s(dInfo.item.pszText,dInfo.item.cchTextMax,"%0.2f",item.MagickaCost());
        break;
    case kEfitCol_School:
        strcpy_s(dInfo.item.pszText,dInfo.item.cchTextMax,Magic::GetSchoolName(item.GetSchool()));
        break; 
    case kEfitCol_Overrides:
    {
        EffectItem::EffectItemExtra* extra = (EffectItem::EffectItemExtra*)item.scriptInfo;
        strcpy_s(dInfo.item.pszText,dInfo.item.cchTextMax, (extra && (extra->mgefFlagMask || extra->efitFlagMask)) ? "Y" : "N");
        break;
    }
    }   
}
int EffectItemList::EffectItemComparator(const EffectItem& itemA, const EffectItem& itemB, int compareBy)
{
    int col = (compareBy > 0) ? compareBy - 1 : -compareBy - 1;
    int result = 0;
    switch (col)
    {
    case kEfitCol_Index:
    {
        const EffectItemList* list = itemA.GetParentList();
        result = StandardCompare(itemA.GetParentList(),itemB.GetParentList());
        if (list && result == 0) result = StandardCompare(list->GetIndexOfEffect(&itemA),list->GetIndexOfEffect(&itemB));
        break;
    }
    case kEfitCol_EffectCode:
        result = StandardCompare(itemA.mgefCode,itemB.mgefCode);
        break;
    case kEfitCol_EffectName:
        result = _stricmp(itemA.GetEffectName().c_str(),itemB.GetEffectName().c_str());  // TODO - replace with GetDisplayName() or equivalent
        break;
    case kEfitCol_Magnitude:
        result = StandardCompare(itemA.GetMagnitude(),itemB.GetMagnitude());
        break;
    case kEfitCol_Area:
        result = StandardCompare(itemA.GetArea(),itemB.GetArea());
        break;
    case kEfitCol_Duration:
        result = StandardCompare(itemA.GetDuration(),itemB.GetDuration());
        break;
    case kEfitCol_Range:
    {
        result = StandardCompare(itemA.GetRange(),itemB.GetRange());
        if (result == 0 && itemA.GetRange() == Magic::kRange_Target)
        {
            result = StandardCompare(itemA.GetProjectileRange(), itemB.GetProjectileRange());
            if (result != 0 && itemA.GetProjectileRange() < 0) result = 1;
            else if (result != 0 && itemB.GetProjectileRange() < 0) result = -1;
        }
        break;
    }
    case kEfitCol_Cost:
        result = StandardCompare(const_cast<EffectItem&>(itemA).MagickaCost(),const_cast<EffectItem&>(itemB).MagickaCost());
        break;
    case kEfitCol_School:
        result = _stricmp(Magic::GetSchoolName(itemA.GetSchool()),Magic::GetSchoolName(itemB.GetSchool())); 
        break;  
    case kEfitCol_Overrides:
    {        
        EffectItem::EffectItemExtra* extra = (EffectItem::EffectItemExtra*)itemA.scriptInfo;
        char ovrA = (extra && (extra->mgefFlagMask || extra->efitFlagMask)) ? 'Y' : 'N';
        extra = (EffectItem::EffectItemExtra*)itemB.scriptInfo;
        char ovrB = (extra && (extra->mgefFlagMask || extra->efitFlagMask)) ? 'Y' : 'N';
        result = StandardCompare(ovrA, ovrB);
        break;
    }
    }
    if (compareBy < 0) result = -result;    
    return result;
}
#endif

// memory patch addresses
memaddr RemoveEffect_Hook                   (0x00414BC0,0x00566F50);
memaddr AddEffect_Hook                      (0x00414B90,0x00566F20);
memaddr ClearEffects_Hook                   (0x00414C70,0x005670F0);
memaddr CopyEffectsFrom_Hook                (0x00414DC0,0x005673D0);
memaddr InitializeDialogEffectList_Hook     (0x0       ,0x00566BF0);
memaddr GetEffectDisplayInfo_Hook           (0x0       ,0x00567670);
memaddr EffectItemComparator_Hook           (0x0       ,0x005678B0);
memaddr MagicItem_LoadPopup_Patch           (0x0       ,0x0056D993); // creation of popup in MagicItem::SetInDialog
memaddr TESSigilStone_LoadPopup_Patch       (0x0       ,0x0051DE33); // creation of popup in TESSigilStone::SetInDialog
memaddr MagicItem_DlgProc_Patch             (0x0       ,0x0056D8F2); // call to ::EffectItemList::EffectListDlgMsgCallback
memaddr TESSigilStone_DlgProc_Patch         (0x0       ,0x0051DD75); // call to ::EffectItemList::EffectListDlgMsgCallback

// hooks & debugging
void EffectItemList::InitHooks()
{
    _MESSAGE("Initializing ...");

    // hook methods
    RemoveEffect_Hook.WriteRelJump(memaddr::GetPointerToMember(&RemoveEffect));
    AddEffect_Hook.WriteRelJump(memaddr::GetPointerToMember(&AddEffect));
    ClearEffects_Hook.WriteRelJump(memaddr::GetPointerToMember(&ClearEffects));
    CopyEffectsFrom_Hook.WriteRelJump(memaddr::GetPointerToMember(&CopyEffectsFrom));
    InitializeDialogEffectList_Hook.WriteRelJump(memaddr::GetPointerToMember(&InitializeDialogEffectList));
    GetEffectDisplayInfo_Hook.WriteRelJump(memaddr::GetPointerToMember(&GetEffectDisplayInfo));
    EffectItemComparator_Hook.WriteRelJump(memaddr::GetPointerToMember(&EffectItemComparator));

    // patch loading of popup menus
    MagicItem_LoadPopup_Patch.WriteRelCall(&OBME::LoadPopupMenu);
    TESSigilStone_LoadPopup_Patch.WriteRelCall(&OBME::LoadPopupMenu);

    // patch calls to EffectListDlgMsgCallback
    MagicItem_DlgProc_Patch.WriteRelCall(memaddr::GetPointerToMember(&EffectListDlgMsgCallback));
    TESSigilStone_DlgProc_Patch.WriteRelCall(memaddr::GetPointerToMember(&EffectListDlgMsgCallback));
}

}   // end namespace OBME