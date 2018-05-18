#ifndef TASK_LIST_H_
#define TASK_LIST_H_

struct _list_st {
    struct _list_st *prev;
    struct _list_st *next;
};

typedef struct _list_st list_t;

#define _offset(type, mem) ((size_t)(&(((type*)(0))->mem)))
#define list_to_struct(ptr, type, mem) ((type*)(ptr - _offset(type, mem)))
#define _list_add(elm, p, n) do { \
    elm->prev = p;\
    elm->next = n;\
    p->next = n->prev = elm;\
} while(0)

#define _list_delete(p, n) do {\
    p->next = n;\
    n->prev = p;\
} while(0)

inline void list_init(list_t *ls) {
    ls->prev = ls->next = ls;
}

inline void list_add_before(list_t *ls, list_t *elm) {
    _list_add(elm, ls->prev, ls);
}

inline void list_add_after(list_t *ls, list_t *elm) {
    _list_add(elm, ls, ls->next);
}

inline void list_add(list_t *ls, list_t *elm) {
    return list_add_after(ls, elm);
}

inline void list_delete(list_t *elm) {
   _list_delete(elm->prev, elm->next); 
}

inline list_t* list_next(list_t *ls) {
    return ls->next;
}

inline list_t* list_prev(list_t *ls) {
    return ls->prev;
}

inline int list_empty(list_t *ls) {
    return ls->prev == ls->next;
}

#endif
