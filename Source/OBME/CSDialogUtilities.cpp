#include "OBME/CSDialogUtilities.h"

#include "API/CSDialogs/TESDialog.h"
#include "API/CSDialogs/DialogExtraData.h"
#include "API/ExtraData/ExtraDataList.h"

#include <CommCtrl.h>

// local declaration of module handle from obme.cpp
extern HMODULE hModule;

namespace OBME {

/************************* Popup Menu Replacer **********************************/
HMENU LoadPopupMenu(HWND dialog, INT menuTemplateID)
{
    HMENU menu = LoadMenu(hModule,MAKEINTRESOURCE(menuTemplateID));
    if (!menu) menu = LoadMenu(TESDialog::csInstance,MAKEINTRESOURCE(menuTemplateID));  // load from CS executable if not found in OBME
    if (!menu) return 0;
    HMENU submenu = GetSubMenu(menu,0);
    if (!submenu) return 0;

    DialogExtraPopupMenu* popupExtra = TESDialog::GetDialogExtraPopupMenu(dialog,menuTemplateID);
    if (!popupExtra)
    {        
        BaseExtraList* list = TESDialog::GetDialogExtraList(dialog);
        if (!list) return 0;
        // create new popup extra entry
        popupExtra = new DialogExtraPopupMenu;
        popupExtra->menu = menu;
        popupExtra->popupMenu = submenu;
        popupExtra->menuTemplateID = menuTemplateID;
        popupExtra->dialogItemID = 0;
        // push entry onto front of list
        popupExtra->extraNext = list->extraList;
        list->extraList = popupExtra;   
    }
    
    return submenu; // return popup menu handle
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