/*
    suPHP - (c)2002-2013 Sebastian Marsching <sebastian@marsching.com>

    This file is part of suPHP.

    suPHP is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    suPHP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with suPHP; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef SUPHP_SMARTPTR_H

namespace suPHP {
    class Environment;
};

#define SUPHP_SMARTPTR_H

#include <map>
#include <cstdlib>
#include "PointerException.hpp"

namespace suPHP {
    /**
     * Class template encapsulating pointers.
     */
    template<class T>
    class SmartPtr {
    private:
        static std::map<T*, int> *counter;
        T* ptr;
        
        void increment(T* key_ptr);
        void decrement(T* key_ptr);
        
    public:
        /**
         * Constructor using NULL pointer
         */
        SmartPtr();
        
        /**
         * Constructor taking T* as argument
         */
        SmartPtr(T* obj_ptr);

        /**
         * Copy constructor
         */
        SmartPtr(const SmartPtr<T>& ref);
        
        /**
         * Destructor
         */
        ~SmartPtr();
        
        /**
         * Copy operator
         */
        const SmartPtr& operator=(const SmartPtr<T>& ref);

        /**
         * Dereference operator.
         * Returns reference to object hold by smart pointer
         */
        T& operator*() const throw(PointerException);
        
        /**
         * Member access operator
         */
        T* operator->() const throw(PointerException);
        
        /**
         * Returns underlying pointer
         */
        T* get() const;
        
        /**
         * Returns underlying pointer and releases it
         * from management by the SmartPtr.
         * Throws an exception if underlying pointer is
         * hold by more than one SmartPtr.
         */
        T* release() throw (PointerException);
        
        /**
         * Resets SmartPtr to point to another object
         */
        void reset(T* obj_ptr);
        
        /**
         * Compares to pointers.
         * Returns true, if both point to the same object,
         * false otherwise
         */
        bool operator==(const SmartPtr<T>& ref);
        
    };
    
    template<class T>
    std::map<T*, int> *suPHP::SmartPtr<T>::counter;
    
    template<class T>
    suPHP::SmartPtr<T>::SmartPtr() {
        if (SmartPtr<T>::counter == NULL) {
            SmartPtr<T>::counter = new std::map<T*, int>;
        }
        this->ptr = NULL;
    }
    
    template<class T>
    suPHP::SmartPtr<T>::SmartPtr(T* obj_ptr) {
        if (SmartPtr<T>::counter == NULL) {
            SmartPtr<T>::counter = new std::map<T*, int>;
        }
        this->ptr = obj_ptr;
        this->increment(obj_ptr);
    }
    
    template<class T>
    suPHP::SmartPtr<T>::SmartPtr(const SmartPtr<T>& ref) {
        if (SmartPtr<T>::counter == NULL) {
            SmartPtr<T>::counter = new std::map<T*, int>;
        }
        this->ptr = ref.ptr;
        if (ref.ptr != NULL)
            this->increment(ref.ptr);
    }
    
    template<class T>
    suPHP::SmartPtr<T>::~SmartPtr() {
        if (this->ptr != NULL)
            this->decrement(this->ptr);
        if (SmartPtr<T>::counter != NULL && SmartPtr<T>::counter->size() == 0) {
            delete SmartPtr<T>::counter;
            SmartPtr<T>::counter = NULL;
        }
    }
    
    template<class T>
    const SmartPtr<T>& suPHP::SmartPtr<T>::operator=(
        const SmartPtr<T>& ref) {
        this.reset(ref.ptr);
        return *this;
    }
    
    template<class T>
    T& suPHP::SmartPtr<T>::operator*() const throw (PointerException) {
        if (this->ptr == NULL)
            throw PointerException("Cannot dereference NULL pointer", 
                                   __FILE__, __LINE__);
        return *(this->ptr);
    }
    
    template<class T>
    T* suPHP::SmartPtr<T>::operator->() const throw (PointerException) {
        if (this->ptr == NULL)
            throw PointerException("Cannot access member of NULL pointer", 
                                   __FILE__, __LINE__);
        return this->ptr;
    }
    
    template<class T>
    T* suPHP::SmartPtr<T>::get() const {
        return this->ptr;
    }
    
    template<class T>
    T* suPHP::SmartPtr<T>::release() throw (PointerException) {
        T* obj_ptr = this->ptr;
        if (obj_ptr == NULL)
            return NULL;

        int& c = SmartPtr<T>::counter->find(obj_ptr)->second;
        
        if (c > 1) {
            throw PointerException(
                "Cannot release object hold by more than one SmartPointer.", 
                __FILE__, __LINE__);
        } else {
            SmartPtr<T>::counter->erase(obj_ptr);
        }
        this->ptr = NULL;
        return obj_ptr;
    }
    
    template<class T>
    void suPHP::SmartPtr<T>::reset(T* obj_ptr) {
        if (obj_ptr != this->ptr) {
            this->decrement(this->ptr);
            this->ptr = obj_ptr;
            this->increment(obj_ptr);
        }
    }
    
    template<class T>
    void suPHP::SmartPtr<T>::increment(T* key_ptr) {
        if (key_ptr == NULL)
            return;

        if (SmartPtr<T>::counter->find(key_ptr) 
            != SmartPtr<T>::counter->end()) {
            (SmartPtr<T>::counter->find(key_ptr)->second)++;
        } else {
            std::pair<T*, int> p;
            p.first = key_ptr;
            p.second = 1;
            SmartPtr<T>::counter->insert(p);
        }
    }

    template<class T>
    void suPHP::SmartPtr<T>::decrement(T* key_ptr) {
        if (key_ptr == NULL)
            return;

        int& c = SmartPtr<T>::counter->find(key_ptr)->second;
        c--;
        if (c < 1) {
            delete key_ptr;
            SmartPtr<T>::counter->erase(key_ptr);
        }
    }
    
    template<class T>
    bool suPHP::SmartPtr<T>::operator==(const SmartPtr<T>& ref) {
        if (this->get() == ref.get())
            return true;
        else
            return false;
    }

};

#endif // SUPHP_SMARTPTR_H
