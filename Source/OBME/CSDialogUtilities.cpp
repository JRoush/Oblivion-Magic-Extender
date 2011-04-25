#include "OBME/CSDialogUtilities.h"

#include "API/CSDialogs/TESDialog.h"
#include "API/CSDialogs/DialogExtraData.h"
#include "API/ExtraData/ExtraDataList.h"
#include "API/TES/MemoryHeap.h"

#include "Utilities/Memaddr.h"

#include <CommCtrl.h>

// local declaration of module handle from obme.cpp
extern HMODULE hModule;

namespace OBME {

/************************* Tab Collection **********************************/
TabCollection::TabCollection(HWND dialog, HWND tabControl) : parentDialog(dialog), parentControl(tabControl) {}
TabCollection::~TabCollection() { ClearTabs(); }
TabCollection* TabCollection::GetTabCollection(HWND dialog, HWND tabControl)
{
    TabCollection* tc = (TabCollection*)GetWindowLong(tabControl,GWL_USERDATA);
    if (!tc)
    {
        tc = new TabCollection(dialog,tabControl);
        SetWindowLong(tabControl,GWL_USERDATA,(LONG)tc);
    }
    return tc;
}
void TabCollection::DestroyTabCollection(HWND tabControl)
{
    TabCollection* tc = (TabCollection*)GetWindowLong(tabControl,GWL_USERDATA);
    if (tc) delete tc;
}
void TabCollection::ClearTabs()
{
    TCITEM item;
    item.mask = TCIF_PARAM;
    item.lParam = 0;
    while (TabCtrl_GetItem(parentControl,0,&item))
    {
        if (item.lParam) delete (TESDialog::Subwindow*)item.lParam; // destroy subwindow
        TabCtrl_DeleteItem(parentControl,0);
    }
}
int TabCollection::InsertTab(HINSTANCE instance, INT dlgTemplate, const char* tabName, int index)
{
    
    // initialize subwindow
    TESDialog::Subwindow* subwindow = new TESDialog::Subwindow;
    subwindow->hDialog = parentDialog;
    subwindow->hContainer = parentControl;
    subwindow->hInstance = instance;
    // add tab to control    
    index = TESTabControl::InsertItem(parentControl,index,tabName,subwindow);
    if (index >= 0)
    {
        // set default subwindow position
        RECT rect;
        GetWindowRect(parentControl,&rect);
        TabCtrl_AdjustRect(parentControl,false,&rect);
        subwindow->position.x = rect.left;
        subwindow->position.y = rect.top;
        ScreenToClient(parentDialog,&subwindow->position); 
        // build subwindow
        TESDialog::BuildSubwindow(dlgTemplate,subwindow);        
        for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
        {
            ShowWindow(node->data, false);  // hide controls
        }
    }
    else delete subwindow;  // delete subwindow on failure
    return index;
}
int TabCollection::SetActiveTab(int index)
{
    int curSel = TESTabControl::GetCurSel(parentControl);
    if (TESDialog::Subwindow* subwindow = (TESDialog::Subwindow*)TESTabControl::GetCurSelData(parentControl))
    {
        for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
        {
            ShowWindow(node->data, false);  // hide controls
        } 
    }
    TabCtrl_SetCurSel(parentControl,index);
    if (TESDialog::Subwindow* subwindow = (TESDialog::Subwindow*)TESTabControl::GetCurSelData(parentControl))
    {
        for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
        {
            ShowWindow(node->data, true);  // show controls
        } 
    }
    return curSel;
}
bool TabCollection::HandleTabControlMessage(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (uMsg != WM_NOTIFY) return false; // not a notify message
    NMHDR* nmhdr = (NMHDR*)lParam; 
    if (nmhdr->hwndFrom != parentControl) return false; // not from parent tab control
    switch(nmhdr->code)
    {
    case TCN_SELCHANGING:
    case TCN_SELCHANGE:
    {
        if (TESDialog::Subwindow* subwindow = (TESDialog::Subwindow*)TESTabControl::GetCurSelData(nmhdr->hwndFrom))
        {
            for (BSSimpleList<HWND>::Node* node = &subwindow->controls.firstNode; node && node->data; node = node->next)
            {
                ShowWindow(node->data, nmhdr->code == TCN_SELCHANGE);  // show/hide controls
            }
        }
        result = 0; return true;
    }
    default:
        return false;
    }
}
/************************* Insertion mark object **********************************/
ListViewInsertionMark::ListViewInsertionMark(HWND hListView) : listView(hListView), markIndex(-1), markAfter(false) {}
void ListViewInsertionMark::SetMark(int index, bool after)
{
    if (markIndex != index && markAfter != after) ClearMark(); // clear current mark if set
    markIndex = index;
    markAfter = after;
    if (markIndex >= 0)
    {
        RECT itemRect;
        ListView_GetItemRect(listView,markIndex,&itemRect,LVIR_BOUNDS);
        RedrawWindow(listView,&itemRect,0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN); // redraw item
    }
}
void ListViewInsertionMark::ClearMark()
{
    if (markIndex < 0) return;  // no mark set
    markIndex = -1; // clear current mark
    RECT itemRect;
    ListView_GetItemRect(listView,markIndex,&itemRect,LVIR_BOUNDS);
    RedrawWindow(listView,&itemRect,0, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);   // redraw item
}
bool ListViewInsertionMark::CustomDraw(LPARAM lParam, LRESULT& result) const
{
    NMLVCUSTOMDRAW* cd = (NMLVCUSTOMDRAW*)lParam;
    if (cd->nmcd.hdr.hwndFrom != listView) return false;    // draw message is not from this listview window
    if (markIndex < 0) return false;    // no mark set

    switch (cd->nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            result = CDRF_NOTIFYITEMDRAW;   // request item-specific draw messages
            return true;
        case CDDS_ITEMPREPAINT: 
            result = CDRF_NOTIFYPOSTPAINT;  // request item-specific post-draw messages
            return true;
        case CDDS_ITEMPOSTPAINT:
        {    
            result = CDRF_DODEFAULT; // perform default post-draw actions
            if (markIndex != cd->nmcd.dwItemSpec) return true;  // drawing non-mark item 
            // prepare graphics objects
            HBRUSH brush = CreateSolidBrush(RGB(255,0,0));
            HGDIOBJ prevBrush = SelectObject(cd->nmcd.hdc,brush);
            HPEN pen = CreatePen(PS_SOLID,0,RGB(255,0,0));
            HGDIOBJ prevPen = SelectObject(cd->nmcd.hdc,pen);                
            // get mark coordinates
            RECT rect;
            ListView_GetItemRect(cd->nmcd.hdr.hwndFrom,cd->nmcd.dwItemSpec,&rect,LVIR_BOUNDS);
            POINT mouse;
            GetCursorPos(&mouse);
            ScreenToClient(cd->nmcd.hdr.hwndFrom,&mouse);
            // draw mark
            POINT triangle[3] = {{0}};
            triangle[0].x = -5;
            triangle[1].x = triangle[2].y = 5;
            if (markAfter)
            {
                for (int c = 0; c < 3; c++) { triangle[c].x += mouse.x; triangle[c].y = -triangle[c].y + rect.bottom - 3; }
                MoveToEx(cd->nmcd.hdc,rect.left,rect.bottom-3,0);
                LineTo(cd->nmcd.hdc,rect.right,rect.bottom-3);
            }
            else
            {                        
                for (int c = 0; c < 3; c++) { triangle[c].x += mouse.x; triangle[c].y += rect.top; }
                MoveToEx(cd->nmcd.hdc,rect.left,rect.top,0);
                LineTo(cd->nmcd.hdc,rect.right,rect.top);
            }
            Polygon(cd->nmcd.hdc,triangle,3);                
            // cleanup graphics objects
            SelectObject(cd->nmcd.hdc,prevPen);
            DeleteObject(pen);
            SelectObject(cd->nmcd.hdc,prevBrush);
            DeleteObject(brush);
            // done
            return true;
        }
    }

    // draw message not handled
    return false;
}
void ListViewInsertionMark::GetMarkIndexAt(const POINT& point, int& index, bool& after) const
{
    
    LVHITTESTINFO hitInfo;
    hitInfo.pt.x = point.x;
    hitInfo.pt.y = point.y;
    index = ListView_HitTestEx(listView,&hitInfo);  // perform hit test to find nearest item
    if (index >= 0)
    {
        RECT itemRect;
        ListView_GetItemRect(listView,index,&itemRect,LVIR_BOUNDS);
        after = (point.y * 2 > itemRect.bottom + itemRect.top);    // compare drop point to vertical middle of item
    }
}

}   //  end namspace OBME