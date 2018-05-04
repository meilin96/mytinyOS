#include "list.h"
#include "interrupt.h"

void list_init(List* list ){
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}

void list_insert_before(ListElem* before, ListElem* elem){
    //通过关中断的方式实现原子操作，后面会实现锁来代替, to be continued
    enum intr_status old_status = intr_disable();

    before->prev->next = elem;
    elem->prev = before->prev;
    elem->next = before;
    before->prev = elem;

    intr_set_status(old_status);
}

void list_push_front(List* list, ListElem* elem){
    list_insert_before(list->head.next, elem);
    //elem->prev = &(list->tail);
    //elem->next = NULL;
    //list->tail = *elem;
}

void list_push_back(List* list, ListElem* elem){
    list_insert_before(&(list->tail), elem);
}

void list_append(List *list, ListElem *elem) {
    list_insert_before(&(list->tail), elem);
}

void list_remove(ListElem* elem){
    enum intr_status old_status = intr_disable();
    
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    intr_set_status(old_status);
}

ListElem* list_pop_front(List* list){
    ListElem* felem = list->head.next;
    list_remove(felem);
    return felem;
}
//查找list中是否存在elem
bool elem_find(List* list, ListElem* elem){
    ListElem* e = list->head.next;
    while(e != &(list->tail)){
        if(e == elem)
            return true;
        e = e->next;
    }
    return false;
}

ListElem* list_traversal(List* list, function func, int arg){
    if(list_empty(list))
        return NULL;

    ListElem* elem = list->head.next;
    while(elem != &(list->tail)){
        if(func(elem, arg) == true){
            return elem;
        }
        elem = elem->next;
    }
    return NULL;
}

uint32_t list_len(List* list){
    uint32_t cnt = 0;
    ListElem* elem = list->head.next;

    while(elem != &(list->tail)){
        cnt++;
        elem = elem->next;
    }
    return cnt;
}

bool list_empty(List* list){
    return list->head.next == &list->tail?true:false;  
}
