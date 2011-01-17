/*
    GenericHandler is a set 'placeholder' EffectHandlers to sub in when the actual handler is not yet defined, or whose
    definition would be trivial. A generic handler will produce correct handler code & name, but all other behavior is
    the same as the default handler.
*/
#pragma once

// base classes
#include "OBME/EffectHandlers/EffectHandler.h"

namespace OBME {

class GenericEffectSettingHandler : public EffectSettingHandler
{
public:
    // constructor
    GenericEffectSettingHandler(EffectSetting& effect, UInt32 handlerCode) : EffectSettingHandler(effect), _handlerCode(handlerCode) {}

    // creation 
    template <UInt32 handlerCode>
    static GenericEffectSettingHandler* Make(EffectSetting& effect) {return new GenericEffectSettingHandler(effect,handlerCode);}

    // handler code and name
    virtual UInt32              HandlerCode() const {return _handlerCode;}

private:
    UInt32                      _handlerCode;
};

class GenericEffectItemHandler : public EffectItemHandler
{
public:
    // constructor
    GenericEffectItemHandler(EffectItem& item, UInt32 handlerCode) : EffectItemHandler(item), _handlerCode(handlerCode) {}

    // creation 
    template <UInt32 handlerCode, const char* handlerName>
    static GenericEffectItemHandler*     Make(EffectItem& item) {return new GenericEffectItemHandler(item,handlerCode,handlerName);}

     // handler code and name
    virtual UInt32              HandlerCode() const {return _handlerCode;}
    virtual const char*         HandlerName() const {return _handlerName;} 

private:
    UInt32                      _handlerCode;
    const char*                 _handlerName;
};

#ifdef OBLIVION
// TODO - generic active effect handler ?
#endif

}   //  end namespace OBME