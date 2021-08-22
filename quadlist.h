#ifndef QUADLIST_H
#define QUADLIST_H

#include <iostream>

const uint64_t MAX_TAG = 1000000000;       //maximum value in quadlist for tailer
const uint64_t MIN_TAG = 0;      //minimum value in quadlist for headers

template<typename T>
class quadnode{    //definition of quadnode
    public:
        T data;
        quadnode *pred;
        quadnode *next;
        quadnode *above;
        quadnode *below;
        bool flag;
        int type;

        //constructor of quadnode
        quadnode(const T &d,quadnode *p = nullptr ,quadnode *n = nullptr,quadnode *a = nullptr ,quadnode *b = nullptr):data(d),pred(p),next(n),above(a),below(b),flag(false),type(0){}
        //copy constructor of quadnode
        quadnode(const quadnode &n):data(n.data),pred(nullptr),next(nullptr),above(nullptr),below(nullptr),flag(false),type(0){}
        //assignment operation of quadnode
        quadnode &operator=(const quadnode &n){
            data = n.data;
        }
};

template<typename T>
void insert(const T &e, quadnode<T> *pre, quadnode<T> *bottom);     //isnert node next to pre and above the bottom

template<typename T>
T remove(quadnode<T> *node);    //remove node

template<typename T>
bool isHeader(quadnode<T> *node);   //whether the node is header

template<typename T>
bool isTailer(quadnode<T> *node);   //whether the node is tailer      

template<typename T>
class quadlist{

    typedef quadnode<T> node;

    friend void insert<T>(const T &e, quadnode<T> *pre, quadnode<T> *bottom);  
    friend T remove<T>(quadnode<T> *node);   
    friend bool isHeader<T>(quadnode<T> *node);
    friend bool isTailer<T>(quadnode<T> *node);

    private:
        node* header;
        node* tailer;
        void init(const quadlist &l);                                                                     //initializer for copy constructor and assignment operator
        void deepCopyIterInFourDirection(node *&preNode, node *&self, node *&other, int dir); //deep copy iteration in four direction
        void deepCopyIterInOneLine(node *&preNode, node *&self, node *&other);                //deep copy iteration in one line
        void flagRestore(node *&other);                                                               //flagRestore
        void deleteIterInFourDirection(node *&node);                                                  //delete iteration in four direction
        void deleteIterInOneLine(node *&node);
        template <typename func>
        void traverseIterInFourDirection(node *&node, const func &f); //traverse iteration in four direction
        template <typename func>
        void traverseIterInOneLine(node *&node, const func &f); //traverse iteration in one line

    public:
        quadlist();                             //default constructor
        quadlist(const quadlist &l);            //copy constructor
        quadlist &operator=(const quadlist &l); //assignment operator
        ~quadlist();                            //destructor
        node *first() const{     //the first node
            return header;
        }
        node *last() const{      //the last node
            return tailer;
        }
		void insert_back(const T &data);		//insert data at the tail
		void clear();		//clear the whole quadlist
		bool empty() const;		//whether the quadlist is empty

        template <typename func>
        void traverse(const func &f);   //traverse the whole quadlist with operation f
};

//definition of defualt constructor
template<typename T>
quadlist<T>::quadlist(){
    header = new node(T());
    tailer = new node(T());
    header->type = 1;
    tailer->type = 2;
    (header->data).first = MIN_TAG;
    (tailer->data).first = MAX_TAG;
    header->next = tailer;
    tailer->pred = header;
}

//definition of initializer for copy constructor and assginment operation
//deep copy
template<typename T>
void quadlist<T>::init(const quadlist &l){
    header = new node(l.header->data);
    tailer = new node(l.tailer->data);
    header->type = 1;
    tailer->type = 2;
    header->next = tailer;
    tailer->pred = header;
    deepCopyIterInOneLine(header, header->next, l.header->next);
}

template<typename T>
void quadlist<T>::deepCopyIterInOneLine(node *&preNode, node *&self, node *&other){
    if(!isTailer<T>(other)){
        self = new node
        (other->data, preNode, preNode->next);
        self->pred = preNode;
        self->next->pred = self;
        deepCopyIterInOneLine(self, self->next, other->next);
    }
}

/*************************************************************************
 * Private iteration algorithm for deep copy in four direction
 * self pointer points to self's quadnode, other pointer points to the
 * object's quadnode which is copied
 * flag is used to show whether this quadnode is copied
 * Iteration spreads along four directions
 * dir: 1 refers to left, 2 refers to right, 3 refers to below, 4 refers
 * to above
 * flagRestore function restore flag recursively
 ************************************************************************/
template<typename T>
void quadlist<T>::deepCopyIterInFourDirection(node *&preNode, node *&self, node *&other, int dir){
    if((other != nullptr) && !other->flag){
        self = new quadnode(other->data);
        switch(dir){
            case 1:         //propagate from left
                self->pred = preNode;
                break;
            case 2:         //propagate from right
                self->next = preNode;
                break;
            case 3:         //propagate from below
                self->below = preNode;
                break;
            case 4:         //propagate from above
                self->above = preNode;
                break;
            default:
                throw std::runtime_error("the dir is not defined!");    //throw runtime error if dir doesn't fit any case
        }
        other->flag = true;
        deepCopyIter(*self, self->pred, other->pred, 2);
        deepCopyIter(*self, self->next, other->next, 1);
        deepCopyIter(*self, self->below, other->below, 4);
        deepCopyIter(*self, self->above, other->above, 3);
    }
}

template<typename T>
void quadlist<T>::flagRestore(node *&other){
    if((other != nullptr) && other->flag){
        other->flag = false;
        flagRestore(other->pred);
        flagRestore(other->next);
        flagRestore(other->below);
        flagRestore(other->above);
    }
}

//definition of copy constructor
template<typename T>
quadlist<T>::quadlist(const quadlist<T> &l):header(l.header),tailer(l.tailer){
    try{
        init(l);
    }catch(const std::exception &e){
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

//definition of assignment operation 
template<typename T>
quadlist<T> &quadlist<T>::operator=(const quadlist<T> &l){
    try{
        header = l.header;
        tailer = l.tailer;
        init(l);
    }catch(const std::exception &e){
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

//definition of destructor
template<typename T>
quadlist<T>::~quadlist(){
    deleteIterInOneLine(header);
}

template<typename T>
void quadlist<T>::deleteIterInOneLine(node *&node){
    if(!isTailer<T>(node)){
        node->next->pred = nullptr;
        deleteIterInOneLine(node->next);
    }
    delete node;
}

/*************************************************************************
 * Private iteration algorithm for delete in four direction
 * Pointer node points to the quadnode that is deleting
 * Iteration spreads along four directions. Before deleting a node,
 * check whether there is branch on each direction, if there is,
 * delete it firstly
 ************************************************************************/
template<typename T>
void quadlist<T>::deleteIterInFourDirection(node *&node){
    if(node != nullptr){
        if(node->next != nullptr){
            node->next->pred = nullptr;
            deleteIter(node->next);
        }
        if(node->pred != nullptr){
            node->pred->next = nullptr;
            deleteIter(node->pred);
        }
        if(node->below != nullptr){
            node->below->above = nullptr;
            deleteIter(node->below);
        }
        if(node->above != nullptr){
            node->above->below = nullptr;
            deleteIter(node->above);
        }
        delete node;
    }
}

//definition of insert
template<typename T>
void insert(const T &data, quadnode<T> *pre, quadnode<T> *bottom){
    try{
        if(bottom != nullptr && bottom->above != nullptr){
            throw std::runtime_error("can insert an node above the bottom node!");     //if there is a node above bottom, throw error
        }
        pre->next = new quadnode<T>(data, pre, pre->next, nullptr, bottom);
        if(pre->next->next != nullptr){
            pre->next->next->pred = pre->next;
        }
        if(bottom != nullptr){
            bottom->above = pre->next;
        }
    }catch(const std::exception &e){
        std::cout << e.what() << std::endl;
        exit(1);
    }
}

//insert data at tail
template<typename T>
void quadlist<T>::insert_back(const T &data) {
	node* pre = tailer->pred;
	insert<T>(data, pre, nullptr);
}

//clear the quadlist
template<typename T>
void quadlist<T>::clear() {
	node *tmp = header->next;
	while (tmp != tailer) {
		node* succ = tmp->next;
		tmp->pred->next = tmp->next;
		tmp->next->pred = tmp->pred;
		delete tmp;
		tmp = succ;
	}
}

//whether the quadlist is empty
template<typename T>
bool quadlist<T>::empty() const {
	return isTailer<T>(header->next);
}

//definition of remove
template<typename T>
T remove(quadnode<T> *node){
    if(node != nullptr){
        if(node->pred != nullptr){
            node->pred->next = node->next;
        }
        if(node->next != nullptr){
            node->next->pred = node->pred;
        }
        if(node->below != nullptr){
            node->below->above = node->above;
        }
        if(node->above != nullptr){
            node->above->below = node->below;
        }
        delete node;
    }
}

//definition of isHeader
template<typename T>
bool isHeader(quadnode<T> *node){
    return node->type == 1;
}   

//definition of isTailer
template<typename T>
bool isTailer(quadnode<T> *node){
    return node->type == 2;
}

//definition of traverse
template<typename T>
template<typename func>
void quadlist<T>::traverse(const func &f){
    traverseIterInOneLine(header->next, f);
}

template<typename T>
template<typename func>
void quadlist<T>::traverseIterInOneLine(node *&node, const func &f){
    if(node != nullptr){
        f(node->data);
        traverseIterInOneLine(node->next);
    }
}

/*************************************************************************
 * Private iteration algorithm for traverse in four direction
 * Pointer node points to the quadnode that is operating, f refers
 * to operation on the quadnode
 * Iteration spreads on four direction
 ************************************************************************/
template<typename T>
template<typename func>
void quadlist<T>::traverseIterInFourDirection(node *&node, const func &f){
    if((node != nullptr) && node->flag){
        f(node->data);
        node->flag = true;
        traverseIter(node->next, f);
        traverseIter(node->pred, f);
        traverseIter(node->below, f);
        traverseIter(node->above, f);
    }
}

#endif
