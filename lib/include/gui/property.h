#ifndef __LIBCACTUSOS__GUI__PROPERTY_H
#define __LIBCACTUSOS__GUI__PROPERTY_H

#include <types.h>

namespace LIBCactusOS
{
    class Control;

    // Property that is used to declare a gui specific variable
    // When this property changes the gui needs to be repainted
    template <typename T>
    class GUIProperty
    {
    friend class Control;
    protected:
        // Value of this property
        T value;

        // Which control is the owner of this property
        Control* parent = 0;
    public:
        // Create new property with default value and pointer to parent control
        GUIProperty(Control* p, T def) { this->parent = p; this->value = def; }

        // Deconstructor
        ~GUIProperty() { }

        
        // Assignment operator
        GUIProperty& operator=(T newVal)
        {
            this->value = newVal;
            if(this->parent) this->parent->ForcePaint();
            return *this;
        }

        // Increase operator
        GUIProperty& operator+=(T newVal)
        {
            this->value += newVal;
            if(this->parent) this->parent->ForcePaint();
            return *this;
        }

        // Decrease operator
        GUIProperty& operator-=(T newVal)
        {
            this->value -= newVal;
            if(this->parent) this->parent->ForcePaint();
            return *this;
        }

        // Get operator, used for value feedback
        operator T() const
        {
            return this->value;
        }
    };        
}

#endif