#ifndef __LIBCACTUSOS__GUI__PROPERTIES_H
#define __LIBCACTUSOS__GUI__PROPERTIES_H

#include <types.h>

namespace LIBCactusOS
{
    // Declare a property for a control or widget
    #define GUI_PROPERTY_DEC(name, type, defaultVal, needRefresh) \
                        protected:                              \
                            type p##name = defaultVal;          \
                        public:                                 \
                            void Set##name(type newVal) {       \
                                p##name = newVal;               \
                                if(needRefresh)                 \
                                    this->needsRefresh = true;  \
                            }                                   \
                            type Get##name() {                  \
                                return p##name;                 \
                            }                                   \
                                        
}

#endif