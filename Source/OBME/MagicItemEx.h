/* 
    OBME expansion of MagicItem.
    This class is the seat of all OBME-specific magic item features and extensions.
*/
#pragma once

// base classes
#include "OBME/MagicGroup.h"

// argument classes

namespace OBME {

// argument classes from OBME
class   MagicSchool;

class MagicItemEx : public MagicGroupList
{
public:

    // members
    //     /*++/++*/ MagicGroupList
    MEMBER /*++/++*/ UInt32     autoCalcFlags;
    MEMBER /*++/++*/ UInt32     magicItemExFlags;

    // virtual methods
    _LOCAL virtual /*000/000*/              ~MagicItemEx();
    _LOCAL virtual /*+++/+++*/              ParentForm();   // get TESForm* of parent
    _LOCAL virtual /*+++/+++*/              MagicItem();    // get MagicItem* of parent
    _LOCAL virtual /*+++/+++*/ const char*  GetIconPath();
    _LOCAL virtual /*+++/+++*/ SInt32       GetGoldValue();
    _LOCAL virtual /*+++/+++*/ UInt8        GetHostility();

    // methods
    _LOCAL MagicSchool*                     GetSchool();

};

}   // end namespace OBME