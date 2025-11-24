//
// Created by james on 20/11/23.
//

#include "Buffer.h"
#include <malloc.h>
#include <stdlib.h>
#include <string.h>

Buffer buffer_create(uint size) {
    char* data = malloc(size * sizeof(char));

    if (data == NULL) {
        return (Buffer) {NULL, -1, -1};
    }

    return (Buffer) {data, size, 0};
}

int buffer_resize_aligned(Buffer* buffer, uint new_size, size_t alignment) {
    new_size += alignment;
    new_size &= ~alignment;

    return buffer_resize(buffer, new_size);
}

int buffer_resize(Buffer* buffer, uint new_size) {
    char* new_data = realloc(buffer->data, new_size);

    if (new_data == NULL) {
        return EXIT_FAILURE;
    }

    buffer->data = new_data;
    buffer->size = new_size;

    return EXIT_SUCCESS;
}

int buffer_nconcat(Buffer* buffer, char* to_add, uint32_t d_size) {
    if (buffer->pos + d_size + 1 >= buffer->size) {
        const uint new_size = buffer->size + d_size + 1;

        const int res = buffer_resize_aligned(buffer, new_size, BUFF_ALIGN);

        if (res != 0) return res;
    }

    if (buffer->pos > 0 && buffer->data[buffer->pos - 1] == '\0') {
        buffer->pos--;
    }

    memcpy(&buffer->data[buffer->pos], to_add, d_size + 1);

    buffer->pos += d_size;
    buffer->data[buffer->pos++] = 0;

    return 0;
}

int buffer_concat(Buffer *buffer, char *to_add) {
    // does this matter? its O(2n + c) but the alternative is to not use memcpy, surely that is slower?
    const uint size_to_add = strlen(to_add);
    return buffer_nconcat(buffer, to_add, size_to_add);
}

int buffer_fconcat(Buffer* buffer, const char* format, ...) {
    va_list args;
    va_list args_copy;
    va_start(args, format);
    va_copy(args_copy, args);

    if (buffer->pos > 0 && buffer->data[buffer->pos - 1] == '\0') {
        buffer->pos--;
    }

    const size_t nchars = vsnprintf(NULL, 0, format, args) + 1;

    if (nchars > buffer->size - buffer->pos) {
        const int res = buffer_resize_aligned(buffer, buffer->size + nchars, BUFF_ALIGN);

        if (res != EXIT_SUCCESS) return EXIT_FAILURE;
    }
    char* to_write_to = &buffer->data[buffer->pos];

    vsnprintf(to_write_to, nchars, format, args_copy);
    buffer->pos += nchars;

    va_end(args);

    return EXIT_SUCCESS;
}

char* buffer_steal(Buffer* buffer, uint new_size) {
    //can no longer assume that buffer.data is not NULL as a theft from buffer can leave realloced result as null
    //alt - could return NULL from steal - ?
    //multiple thefts would pick up issue as NULL returned but not the last
    char* new_data = NULL;
    if (new_size > 0) new_data = malloc(new_size);

    char* ret = buffer->data;

    *buffer = (Buffer){new_data, new_size, 0};

    return ret;
}

char* buffer_copy(Buffer* buffer) {
    char* newLine = malloc(buffer->size * sizeof(char));

    memcpy(newLine, buffer->data, buffer->pos);

    return newLine;
}

void buffer_clear(Buffer* buffer) {
    buffer->pos = 0;
}

void buffer_destroy(Buffer* buffer) {
    free(buffer->data);

    *buffer = (Buffer){NULL, -1, -1};
}
