//
// Created by jamescoward on 07/06/2024.
//

#include <stdlib.h>
#include <string.h>

#include "Array.h"

// [[todo]] Generic usages of array should be updated to use typed
//  structure and functions

// [[todo]] updated the CMP extensions to include the bsearch and qsort
//   any usages of these in the other files should be updated to use new
//   extension functions

const static Array ARRAY_ERR = (Array) {
    .arr = NULL,
    .capacity = -1,
    .element_size = -1,
    .pos = -1,
};

Array arr_construct(const uint element_size, const uint min_element_count) {
    void* memory = malloc(element_size * min_element_count);

    if (!memory) exit(ENOMEM);

    const Array vec = (Array) {
        memory,
        min_element_count,
        0,
        element_size,
        {0}
    };

    return vec;
}

// todo: refactor to make more clear that the input is the element_size and not the array size
Array arr_create(const uint element_size) {
    return arr_construct(element_size, MIN_ARRAY_SIZE);
}

bool arr_is_at_capacity(const Array* array) {
    return array->pos == array->capacity;
}

void arr_resize(Array* array) {
    const uint new_capacity = array->capacity << 1;

    char* new_memory = realloc(array->arr, new_capacity * array->element_size);

    if (!new_memory) exit(ENOMEM);

    array->arr = new_memory;
    array->capacity = new_capacity;
}

void arr_set(Array* array, const size_t index, const void* element) {
    if (index >= array->pos) return;

    memcpy(array->arr + (index * array->element_size), element, array->element_size);

    array->flags.sorted= false;
}

void arr_set_dyn(Array* array, const size_t index, const void* element, const size_t element_size) {
    if (index >= array->pos) return;

    memset(&array->arr[index * array->element_size], 0, array->element_size);
    memcpy(&array->arr[index * array->element_size], element, element_size);

    array->flags.sorted= false;
}

void* arr_add_i(Array* arr) {
    if (arr_is_at_capacity(arr)) {
        arr_resize(arr);
    }

    void* res= &arr->arr[arr->pos * arr->element_size];
    memset(res, 0, arr->element_size);

    arr->pos++;
    arr->flags.sorted= false;

    return res;
}

void arr_add(Array* array, const void* element) {
    if (arr_is_at_capacity(array))
        arr_resize(array);

    memcpy(&array->arr[array->pos * array->element_size], element, array->element_size);

    array->pos++;
    array->flags.sorted= false;
}

// add to the array a value that is not the size of the array element size
void arr_add_dyn(Array* array, const void* element, const size_t element_size) {
    if (arr_is_at_capacity(array))
        arr_resize(array);

    memset(&array->arr[array->pos * array->element_size], 0, array->element_size);
    memcpy(&array->arr[array->pos * array->element_size], element, element_size);

    array->pos++;
    array->flags.sorted= false;
}

bool arr_remove(Array* array, const uint index) {
    if (index >= array->pos) return false;

    const uint elements_right = array->pos - index - 1;

    array->pos--;

    void* dst = &array->arr[index * array->element_size];
    const void* src = &array->arr[(index + 1) * array->element_size];

    memmove(dst, src, elements_right);

    return true;
}

void* arr_pop(Array* array) {
    if (array->pos == 0) return NULL;

    const uint index = array->pos - 1;
    void* element = arr_ptr(array, index);
    arr_remove(array, index);

    return element;
}

void* arr_peek(const Array* array) {
    if (array->pos == 0) return NULL;

    return arr_ptr(array, array->pos - 1);
}

void* arr_ptr(const Array* array, const uint index) {
    if (index >= array->pos) return NULL;

    return &array->arr[index * array->element_size];
}

void arr_destroy(Array* array) {
    free(array->arr);

    *array = ARRAY_ERR;
}

void arr_sort(Array* arr, int (*cmp_func)(const void* a, const void* b)) {
    if (arr->flags.sorted) return;

    qsort(
        arr->arr,
        arr->pos,
        arr->element_size,
        cmp_func
    );
    arr->flags.sorted= true;
}

uint arr_search(Array* array, const void* search_elem, int (*cmp_func)(const void* a, const void* b)) {
    if (!array->flags.sorted)
        arr_sort(array, cmp_func);

    const void* pos= bsearch(
        search_elem,
        array->arr,
        array->pos,
        array->element_size,
        cmp_func
    );

    if (!pos) return -1;

    return (pos - array->arr) / array->element_size;
}

void* arr_search_e(Array* array, const void* s_e, int (*cmp)(const void* a, const void* b)) {
    if (!array->flags.sorted)
        arr_sort(array, cmp);

    return bsearch(
        s_e,
        array->arr,
        array->pos,
        array->element_size,
        cmp
    );
}
