#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <iostream>
#include <list>
#include <memory>
#include <ctime>
#include <cstdlib>
#include "quadlist.h"

template<typename key, typename value>
class skiplist{

    typedef std::list<quadlist<std::pair<key, value>>> List;
    typedef typename List::iterator Iter;
    typedef quadnode<std::pair<key, value>> node;

    public:
        skiplist();                             //default constructor
        skiplist(const skiplist&);            //copy constructor
        skiplist &operator=(const skiplist&); //assignment operator
        ~skiplist() {}                          //destructor
        unsigned size() const{ //the number of items
            return Size;
        }
        int level() const{      //the height of skiplist
            return ptr->size();
        }
        void put(const key&, const value&);       //add item into skiplist
        value get(const key&) const;       //get value by key
        uint64_t remove(const key&);           //remove item in skiplist
        node *find(const key &) const;      //find item in skiplist
        void clear();       //clear all the items in skiplist
        const quadlist<std::pair<key,value>> &pairList() const{     //return the list that contains all pairs
            return ptr->back();
        }

    private:
        std::shared_ptr<List> ptr;     //resourse manager
        unsigned Size;

        void addLayer();        //add a level in skiplist
        void removeTower(node*);        //remove a tower in skiplist
        void removeTopLayer();         //remove top layer of skiplist
};

/**********************************************************************
 * randomGenerator
 * generate random number in {0, 1} by half probability
 * *******************************************************************/
template<typename T>
bool randomGenerator(){
    clock_t   now   =   clock(); 
    while(clock() - now < 1){} 
    srand(clock());
    return rand() % 2;
}

//definition of default constructor
template<typename key, typename value>
skiplist<key,value>::skiplist():ptr(std::make_shared<List>()),Size(0){
    ptr->push_front(quadlist<std::pair<key, value>>());
}

//definition of copy constructor
template<typename key, typename value>
skiplist<key,value>::skiplist(const skiplist &l):ptr(std::make_shared<List>(l)),Size(l.size()){}

//definition of assignment operator
template<typename key, typename value>
skiplist<key,value> &skiplist<key,value>::operator=(const skiplist &l){
    Size = l.size();
    ptr.reset(&l);
}

//definition of addLevel
template<typename key, typename value>
void skiplist<key,value>::addLayer(){
    ptr->push_front(quadlist<std::pair<key, value>>());
    Iter upLayer = ptr->begin();
    Iter bottomLayer = ++(ptr->begin());
    (upLayer->first())->below = bottomLayer->first();
    (bottomLayer->first())->above = upLayer->first();
    (upLayer->last())->below = bottomLayer->last();
    (bottomLayer->last())->above = upLayer->last();
}

/*************************************************************************
 * Put operation for skiplist
 * Add item(k, v) into skiplist(allow repeating)
 * The items in bottom layer is sorted
 * When add a new item in one layer, there is half probability
 * that the tower can grow
 * When one tower's height is beyond the highest level, then we
 * need to add a new layer in skiplist
 ************************************************************************/
template<typename key, typename value>
void skiplist<key,value>::put(const key &k, const value &v){
    try{
        node *nodePtr = (ptr->begin())->first();      //start from the header of top layer

        //search the position the item should inserted in
        for (Iter iter = ptr->begin(); iter != ptr->end(); iter++){
            if(iter != ptr->begin()){
                nodePtr = nodePtr->below;
            }
            while(k >= (nodePtr->data).first){
                nodePtr = nodePtr->next;
            }
            nodePtr = nodePtr->pred;
        }

        insert<std::pair<key,value>>(std::pair<key, value>(k, v), nodePtr, nullptr);
        node *bottom = nodePtr->next;

        //tower grows by half probability
        while(randomGenerator<int>()){
            while(nodePtr->above == nullptr){
                if(isHeader<std::pair<key,value>>(nodePtr)){       //nodePtr points to header
                    addLayer();
                    break;
                }else{
                    nodePtr = nodePtr->pred;
                }
            }
            nodePtr = nodePtr->above;
            insert<std::pair<key,value>>(std::pair<key, value>(k, v), nodePtr, bottom);
            bottom = nodePtr->next;
        }

        Size++;
    }catch(const std::exception &e){
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

/*************************************************************************
 * Get operation for skiplist
 * Searching start from the top layer, and deep down to the
 * bottom. If the bottom layer doesn't contain this item, 
 * throw runtime error
 ************************************************************************/
template<typename key, typename value>
value skiplist<key,value>::get(const key &k) const{
    try{
        return (find(k)->data).second;
    }catch(const std::exception &e){
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

template<typename key, typename value>
typename skiplist<key,value>::node *skiplist<key,value>::find(const key &k) const{
    node *nodePtr = (ptr->begin())->first();        //start from the header of top layer

    for(Iter iter = ptr->begin(); iter != ptr->end(); iter++){
        if(iter != ptr->begin()){
            nodePtr = nodePtr->below;
        }
        while(k > (nodePtr->data).first || isHeader(nodePtr)){
            nodePtr = nodePtr->next;
        }
        if(k == (nodePtr->data).first){
            return nodePtr;
        }
        nodePtr = nodePtr->pred;
    }

    return nullptr;
}

/*************************************************************************
 * Remove operation for skiplist
 * Searching for position of the item we need to remove at first,
 * then remove the whole tower of the item
 ************************************************************************/
template<typename key, typename value>
uint64_t skiplist<key,value>::remove(const key &k){
    try{
        node *position = find(k);
		uint64_t size = (position->data.second).size();

        if(position != nullptr){
            removeTower(find(k));          //remove the whole tower of an item
            Size--;
			return size;
        }else{                             //can't find this item in skiplist
            throw std::runtime_error("the item is not found!");
        }   
    }catch(const std::exception &e){
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

template<typename key, typename value>
void skiplist<key,value>::removeTower(node *nodePtr){
    while(nodePtr != nullptr){
        nodePtr->pred->next = nodePtr->next;
        nodePtr->next->pred = nodePtr->pred;

        if(isHeader<std::pair<key,value>>(nodePtr->pred) && isTailer<std::pair<key,value>>(nodePtr->next)){     //the layer is empty, remove it
            if(nodePtr->below != nullptr){
                nodePtr = nodePtr->below;
                delete nodePtr->above;
                nodePtr->above = nullptr;
                removeTopLayer();
            }else{
                delete nodePtr;
                nodePtr = nullptr;
            }
        }else{
            if(nodePtr->below != nullptr){
                nodePtr = nodePtr->below;
                delete nodePtr->above;
                nodePtr->above = nullptr;
            }else{
                delete nodePtr;
                nodePtr = nullptr;
            }
        }
    }
}

template<typename key, typename value>
void skiplist<key,value>::removeTopLayer(){
    ptr->pop_front();
    
    if(!ptr->empty()){
        ((ptr->begin())->first())->above = nullptr;
        ((ptr->begin())->last())->above = nullptr;
    }
}

/*************************************************************************
 * Clear operation for skiplist
 * For each items in the bottom layer, remove it from skiplist
 ************************************************************************/
template<typename key, typename value>
void skiplist<key,value>::clear(){
    node *tmp = ((ptr->back()).first())->next;      //points to the first item of bottom layer
    while(!isTailer(tmp)){
		node* after = tmp->next;
        remove((tmp->data).first);
		tmp = after;
    }
}

#endif