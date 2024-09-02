#pragma once
#include <set>
#include <vector>

/*
    Wrapper around std::vector<Object*>.
    Programmer can use 'pushBack(new Object(blah, blah))' without having to worry about memory cleanup.
    Programmer should NOT use 'delete' with anything in this class.

    Features:
    - Guaranteed single-time destruction of each element
    - Programmer can use erase() without having to worry about 'delete vs erase'.
    - eraseMultiple() objects without having to worry about the order in which you erase them
    - All elements destroy themselves once the list itself destructs.
    - If doing pushBack(nullptr), nothing gets added.
    - size() is actually an unsigned int
*/

namespace nch { template <typename T> class ArrayList {
public:
    ArrayList(){}
    ~ArrayList() {
        clear();
    }

    std::vector<T*> vecCopy() { return arrlist; }
    unsigned int size() { return arrlist.size(); }

    T& at(int dex) {
        return (*arrlist.at(dex));
    }

    void pushBack(T* obj) {
        if(obj!=nullptr)
            arrlist.push_back(obj);
    }

    void pushBack(T objCopyable) {
        pushBack(new T(objCopyable));
    }

    void erase(int dex) {
        T* obj = &at(dex);
        if(obj!=nullptr) delete obj; 
        arrlist.erase(arrlist.begin()+dex);
    }
    void erase(int start, int end) {
        for(int i = end; i>=0 && i>=start; i--) {
            erase(i);
        }
    }
    void clear() { erase(0, size()-1); }

    void eraseMultiple(std::set<int> dexes)
    {
        for(std::set<int>::reverse_iterator rit = dexes.rbegin(); rit != dexes.rend(); rit++) {
            erase(*rit);
        }
    }

private:
    std::vector<T*> arrlist;

}; }