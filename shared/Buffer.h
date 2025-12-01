//
// Created by james on 20/11/23.
//

#ifndef ATOMIC_BUFFER_H
#define ATOMIC_BUFFER_H

#include "../Commons.h"

#include "Helper_String.h"

#define BUFF_MIN 100
#define BUFF_ALIGN 100

typedef unsigned int uint;

typedef struct Buffer {
    char* data;
    uint size;
    uint pos;
} Buffer;

Buffer buffer_create(uint size);
int buffer_resize(Buffer* buffer, uint size);
int buffer_concat(Buffer *buffer, char *to_add);
int buffer_nconcat(Buffer* buffer, char* to_add, uint32_t d_size);
int buffer_fconcat(Buffer* buffer, const char* format, ...);
char* buffer_steal(Buffer* buffer, uint new_size);
char* buffer_copy(Buffer* buffer);
void buffer_clear(Buffer* buffer);
void buffer_destroy(Buffer* buffer);

#endif //ATOMIC_BUFFER_H
