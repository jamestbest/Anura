//
// Created by jamescoward on 07/06/2024.
//

#ifndef ARRAY_H
#define ARRAY_H

#include <errno.h>
#include <stdlib.h>

#include "Commons.h"

#define MIN_ARRAY_SIZE 10

typedef struct ArrayFlags {
    uint32_t sorted: 1;
} ArrayFlags;

typedef struct Array {
    void* arr;
    size_t capacity;
    size_t pos;
    uint32_t element_size;
    ArrayFlags flags;
} Array;

static const Array ARRAY_ERR;

Array arr_construct(uint element_size, uint min_element_count);
Array arr_create(uint element_size);
bool arr_is_at_capacity(const Array* array);
void arr_resize(Array* array);
void arr_add(Array* array, const void* element);
void* arr_add_i(Array* arr);
void arr_add_dyn(Array* array, const void* element, size_t element_size);
void arr_set(Array* array, size_t index, const void* element);
void arr_set_dyn(Array* array, size_t index, const void* element, size_t element_size);
bool arr_remove(Array* array, uint index);
void* arr_pop(Array* array);
void* arr_peek(const Array* array);
void* arr_ptr(const Array* array, uint index);
void arr_destroy(Array* array);
void arr_sort(Array* arr, int (*cmp_func)(const void* a, const void* b));
uint arr_search(Array* array, const void* search_elem, int (*cmp_func)(const void* a, const void* b));
void* arr_search_e(Array* array, const void* s_e, int (*cmp)(const void* a, const void* b));

#define ARRAY_TYPE(type, typename)                                             \
    typedef struct {                                                           \
        type* arr;                                                             \
        size_t capacity;                                                       \
        size_t pos;                                                            \
        uint32_t element_size;                                                 \
        ArrayFlags flags;                                                      \
    } typename##Array;

#define ARRAY_PROTO(type, typename)                                            \
    ARRAY_TYPE(type, typename)                                                 \
                                                                               \
    extern const typename##Array typename##ARRAY_EMPTY;                        \
                                                                               \
    void typename##_arr_add(typename##Array* arr, const type element);         \
    type typename##_arr_get(const typename##Array* arr, const uint index);     \
    type* typename##_arr_ptr(const typename##Array* arr, const uint index);    \
    type typename##_arr_pop(typename##Array* arr);                             \
    type typename##_arr_peek(const typename##Array* arr);                      \
    typename##Array typename##_arr_construct(uint elem_c);                     \
    typename##Array typename##_arr_create();                                   \
    bool typename##_arr_is_at_capacity(const typename##Array* array);          \
    void typename##_arr_resize(typename##Array* array);                        \
    void typename##_arr_set(typename##Array* array,                            \
        const size_t index, const type* element);                              \
    bool typename##_arr_remove(typename##Array* array, const uint index);      \
    void typename##_arr_destroy(typename##Array* array);                       \

#define ARRAY_PROTO_CMP(type, typename, generic_cmp, cmp_elem)                 \
    ARRAY_PROTO(type, typename)                                                \
    type* typename##_arr_search_ie(                                            \
        typename##Array* array,                                                \
        const typeof((type){}.cmp_elem) s_e                                    \
    );                                                                         \
    uint typename##_arr_search_i(                                              \
        typename##Array* array,                                                \
        const typeof((type){}.cmp_elem) search_element                         \
    );                                                                         \
    int typename##_arr_search_cmp_i(                                           \
        const void* search_element,                                            \
        const void* array_element                                              \
    );                                                                         \
    void typename##_arr_sort_i(                                                \
        typename##Array* array                                                 \
    );                                                                         \
    int typename##_arr_cmp_i(                                                  \
        const void* a, const void* b                                           \
    );                                                                         \

#define ARRAY_ADD(type, typename)                                              \
    const typename##Array typename##ARRAY_EMPTY= {                             \
        .arr= NULL,                                                            \
        .capacity= 0,                                                          \
        .pos= 0,                                                               \
        .element_size= 0,                                                      \
        .flags= {0}                                                            \
    };                                                                         \
                                                                               \
    void typename##_arr_add(typename##Array* arr, const type element) {        \
         arr_add((Array*)arr, &element);                                       \
    }                                                                          \
                                                                               \
    type* typename##_arr_add_i(typename##Array* arr) {                         \
         return arr_add_i((Array*)arr);                                        \
    }                                                                          \
                                                                               \
    type typename##_arr_get(const typename##Array* arr, const uint index) {    \
         return *(type*)arr_ptr((Array*)arr, index);                           \
    }                                                                          \
                                                                               \
    type* typename##_arr_ptr(const typename##Array* arr, const uint index) {   \
        return (type*)arr_ptr((Array*)arr, index);                             \
    }                                                                          \
                                                                               \
    type typename##_arr_pop(typename##Array* arr) {                            \
        return *(type*)arr_pop((Array*)arr);                                   \
    }                                                                          \
                                                                               \
    type typename##_arr_peek(const typename##Array* arr) {                     \
        return *(type*)arr_peek((Array*)arr);                                  \
    }                                                                          \
                                                                               \
    typename##Array typename##_arr_construct(uint elem_c) {                    \
        type* memory= malloc(sizeof (type) * elem_c);                          \
                                                                               \
        if (!memory) exit(ENOMEM);                                             \
                                                                               \
        const typename##Array arr= (typename##Array){                          \
            memory,                                                            \
            elem_c,                                                            \
            0,                                                                 \
            sizeof (type),                                                     \
            {0}                                                                \
        };                                                                     \
                                                                               \
        return arr;                                                            \
    }                                                                          \
                                                                               \
    typename##Array typename##_arr_create() {                                  \
        return typename##_arr_construct(MIN_ARRAY_SIZE);                       \
    }                                                                          \
                                                                               \
    void typename##_arr_resize(typename##Array* array) {                       \
           return arr_resize((Array*) array);                                  \
    }                                                                          \
                                                                               \
    void typename##_arr_set(                                                   \
        typename##Array* array,                                                \
        const size_t index,                                                    \
        const type* element                                                    \
    ) {                                                                        \
        return arr_set((Array*)array, index, (void*)element);                  \
    }                                                                          \
                                                                               \
    bool typename##_arr_remove(typename##Array* array, const uint index) {     \
        return arr_remove((Array*)array, index);                               \
    }                                                                          \
                                                                               \
    void typename##_arr_destroy(typename##Array* array) {                      \
        return arr_destroy((Array*)array);                                     \
    }                                                                          \
                                                                               \
    void typename##_arr_sort(                                                  \
        typename##Array* array, int (*cmp)(const void* a, const void* b)       \
    ) {                                                                        \
        return arr_sort((Array*)array, cmp);                                   \
    }                                                                          \
                                                                               \
    uint typename##_arr_search(                                                \
        typename##Array* array,                                                \
        const void* search_element,                                            \
        int (*cmp)(const void* a, const void* b)                               \
    ) {                                                                        \
        return arr_search((Array*)array, search_element, cmp);                 \
    }                                                                          \

#define ARRAY_CMP(type, typename, generic_cmp, cmp_elem)                       \
    int typename##_arr_cmp_i(                                                  \
            const void* a, const void* b                                       \
    ){                                                                         \
        return generic_cmp(                                                    \
            ((type*)a)->cmp_elem,                                              \
            ((type*)b)->cmp_elem                                               \
        );                                                                     \
    }                                                                          \
                                                                               \
    void typename##_arr_sort_i(                                                \
        typename##Array* array                                                 \
    ) {                                                                        \
        return arr_sort((Array*)array, typename##_arr_cmp_i);                  \
    }                                                                          \
                                                                               \
    int typename##_arr_search_cmp_i(                                           \
        const void* search_element,                                            \
        const void* array_element                                              \
    ) {                                                                        \
        const typeof((type){}.cmp_elem) array_attribute=(((type*)array_element)->cmp_elem);\
        return generic_cmp(*(typeof((type){}.cmp_elem)*)search_element, array_attribute);  \
    }                                                                          \
                                                                               \
    uint typename##_arr_search_i(                                              \
        typename##Array* array,                                                \
        const typeof((type){}.cmp_elem) search_element                         \
    ) {                                                                        \
        if (!array->flags.sorted)                                              \
            typename##_arr_sort_i(array);                                      \
                                                                               \
        const type* pos= bsearch(                                              \
            &search_element,                                                   \
            array->arr,                                                        \
            array->pos,                                                        \
            array->element_size,                                               \
            typename##_arr_search_cmp_i                                        \
        );                                                                     \
                                                                               \
        if (!pos) return -1;                                                   \
                                                                               \
        return (pos - array->arr);                                             \
    }                                                                          \
                                                                               \
    type* typename##_arr_search_ie(                                            \
        typename##Array* array,                                                \
        const typeof((type){}.cmp_elem) s_e                                    \
    ) {                                                                        \
        if (!array->flags.sorted)                                              \
            typename##_arr_sort_i(array);                                      \
                                                                               \
        return bsearch(                                                        \
            &s_e,                                                              \
            array->arr,                                                        \
            array->pos,                                                        \
            array->element_size,                                               \
            typename##_arr_search_cmp_i                                        \
        );                                                                     \
    }                                                                          \

#define ARRAY_ADD_CMP(type, typename, generic_cmp, cmp_elem)                   \
    ARRAY_ADD(type, typename)                                                  \
    ARRAY_CMP(type, typename, generic_cmp, cmp_elem)                           \

#define ARRAY_JOINT(type, typename)                                            \
    ARRAY_PROTO(type, typename)                                                \
    ARRAY_ADD(type, typename)                                                  \

#define ARRAY_JOINT_CMP(type, typename, generic_cmp, comparable_element)       \
    ARRAY_PROTO(type, typename)                                                \
    ARRAY_ADD(type, typename)                                                  \
    ARRAY_CMP(type, typename, generic_cmp, comparable_element)                 \

#endif //ARRAY_H
