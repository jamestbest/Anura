//
// Created by james on 07/11/23.
//

#ifndef ATOMIC_VECTOR_H
#define ATOMIC_VECTOR_H

#include "../Commons.h"

#define MIN_VEC_SIZE 10

typedef unsigned int uint;

typedef struct Vector {
    void** arr;
    size_t pos;
    size_t capacity;
} Vector;

static const Vector VEC_ERR;

typedef struct VecRet {
    void* data;
    uint retCode;
} VecRet;

typedef struct FindRes {
    void* elem;
    uint pos;
} FindRes;

extern const FindRes FIND_RES_ERR;

Vector vector_create();
Vector vector_construct(size_t element_count);
bool vector_at_capacity(const Vector* vec);
bool vector_verify(const Vector* vec);
bool vector_resize(Vector* vec, size_t new_element_count);
bool vector_add(Vector* vec, void* element);
bool vec_add_d(Vector* vec, const void* data, size_t data_s);
VecRet vector_remove(Vector* vec, uint index);
void* vector_remove_unsafe(Vector* vec, uint index);
VecRet vector_pop(Vector* vec);
void* vector_pop_unsafe(Vector* vec);
VecRet vector_get(const Vector* vec, uint index);
void* vector_get_unsafe(const Vector* vec, uint index);
void vector_destroy(Vector* vec);
void vector_disseminate_destruction(Vector* vec);

void vec_sort(const Vector* vec, int(*cmp)(const void* a, const void* b));
uint vec_search(Vector* vec, const void** search_element, int (*cmp)(const void* a, const void* b));
FindRes vec_search_e(Vector* vec, const void** s_e, int(*cmp)(const void* a, const void* b));

#define VECTOR_PROTO(type, type_name)                                               \
    typedef struct type_name##Vector {                                              \
        type** arr;                                                                 \
        size_t pos;                                                                 \
        size_t capacity;                                                            \
    } type_name##Vector;                                                            \
    bool type_name##_vec_add(type_name##Vector* vec, const type* elem);             \
    type* type_name##_vec_get_unsafe(type_name##Vector* vec, size_t idx);

#define VECTOR_ADD(type, type_name)                                                 \
    bool type_name##_vec_add(type_name##Vector* vec, const type* elem) {            \
        return vector_add((Vector*)vec, (void*)elem);                               \
    }                                                                               \
                                                                                    \
    type_name##Vector type_name##_vec_construct(size_t element_count) {             \
        type** data= malloc(sizeof(type*) * element_count);                         \
                                                                                    \
        if (!data) return (type_name##Vector) {                                     \
            .arr=NULL,                                                              \
            .pos= -1,                                                               \
           .capacity= -1                                                            \
        };                                                                          \
                                                                                    \
        return (type_name##Vector) {                                                \
            .arr= data,                                                             \
            .pos=0,                                                                 \
            .capacity=element_count                                                 \
        };                                                                          \
    }                                                                               \
                                                                                    \
    type_name##Vector type_name##_vec_create() {                                    \
        return type_name##_vec_construct(MIN_VEC_SIZE);                             \
    }                                                                               \
                                                                                    \
    type* type_name##_vec_get_unsafe(type_name##Vector* vec, size_t idx) {          \
        return vector_get_unsafe((Vector*)vec, idx);                                \
    }


#define VECTOR_CMP(type, type_name, generic_cmp, comparable_element)                \
    int type_name##_vec_cmp_i(                                                      \
            const void* a, const void* b                                            \
    ){                                                                              \
        return generic_cmp(                                                         \
            (*(type**)a)->comparable_element,                                         \
            (*(type**)b)->comparable_element                                          \
        );                                                                          \
    }                                                                               \
                                                                                    \
    void type_name##_vec_sort_i(                                                    \
        type_name##Vector* vec                                                      \
    ) {                                                                             \
        vec_sort((Vector*)vec, type_name##_vec_cmp_i);                              \
    }                                                                               \
                                                                                    \
    int type_name##_vec_search_cmp_i(                                               \
        const void* search_element,                                                 \
        const void* vector_element                                                  \
    ) {                                                                             \
        const void* vector_attribute=(*(type**)vector_element)->comparable_element;   \
        return generic_cmp(*(void**)search_element, vector_attribute);                       \
    }                                                                               \
                                                                                    \
    uint type_name##_vec_search_i(                                                  \
        type_name##Vector* vec,                                                     \
        const void* search_element                                                  \
    ) {                                                                             \
        return vec_search(                                                          \
            (Vector*)vec,                                                           \
            search_element,                                                         \
            type_name##_vec_search_cmp_i                                            \
        );                                                                          \
    }                                                                               \
                                                                                    \
    FindRes type_name##_vec_search_ie(                                              \
        type_name##Vector* vec,                                                     \
        const void** s_e                                                            \
    ) {                                                                             \
        return vec_search_e(                                                        \
            (Vector*)vec,                                                           \
            s_e,                                                                    \
            type_name##_vec_search_cmp_i                                            \
        );                                                                          \
    }

#define VECTOR_ADD_CMP(type, type_name, generic_cmp, comparable_element)            \
    VECTOR_ADD(type, type_name)                                                     \
    VECTOR_CMP(type, type_name, generic_cmp, comparable_element)

#endif //ATOMIC_VECTOR_H
