/* 
    OBME utilities for manaing CS dialogs
*/
#pragma once
#ifndef OBLIVION

// base classes
#include "API/ExtraData/BSExtraData.h"
#include "API/TESForms/BaseFormComponent.h" // TESIcon
#include "API/TES/MemoryHeap.h"

namespace OBME {
    
// custom COMMAND message - synonym for WM_COMMAND; avoids faulty message trapping in TESFormIDListView::DlgProc 
static const UInt32 WM_USERCOMMAND =  WM_APP + 0x55; 

// custom "NONE" entry for combo lists - reproduces string from CS
static const char* kNoneEntry = " NONE ";

// templated comparison function, for listview comparsions
template <class DataT> int StandardCompare(DataT a, DataT b) { if (a<b) return -1; else if (a>b) return 1; else return 0; }

// popup menu replacer - loads popup menu from OBME instead of CS executable if possible
HMENU LoadPopupMenu(HWND dialog, INT menuTemplateID);

// extra TESIcon class for icon override manipulation
class DialogExtraIcon : public BSExtraData, public TESIcon
{
public:

    static const UInt8 kDialogExtra_Icon = 0xF0;     

     // BSExtraData virtual method overrides:
    _LOCAL /*00/00*/ virtual        ~DialogExtraIcon() {}  // default destructor will invoke BSExtraData, TESIcon destructors
    // no additional virtual methods

    // constructor
    _LOCAL DialogExtraIcon() : BSExtraData(), TESIcon() {extraType = kDialogExtra_Icon;}

    // use FormHeap for class new/delete
    USEFORMHEAP
};

// Insertion mark object for drag+drop on listviews in report mode
class ListViewInsertionMark
{
public:
    // methods
    ListViewInsertionMark(HWND hListView);
    void    SetMark(int index, bool after); // moves insertion mark, forcing redraw if necessary
    void    ClearMark(); // clears insertion mark, forcing redraw if necessary
    bool    CustomDraw(LPARAM lParam, LRESULT& result) const;  // process custom draw message for listview, returning true if handled
    void    GetMarkIndexAt(const POINT& point, int& index, bool& after) const;  // determines index+after values for specified point (in client coords)
    int     GetMarkIndex() const { return markIndex; }
    bool    GetMarkAfter() const { return markAfter; }
    // use form heap
    USEFORMHEAP
private:
    const HWND  listView;
    int         markIndex;  // position of current mark
    bool        markAfter;  // mark is after item (vs before)
};

}   //  end namspace OBME
#endif