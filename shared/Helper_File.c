//
// Created by jamescoward on 11/11/2023.
//

#include "Helper_File.h"
#include <malloc.h>
#include <string.h>

FILE* open_file(const char* cwd, const char* filename, const char* mode) {
    FILE* fp = fopen(filename, mode);

    if (fp != NULL) return fp;

    const char* path = get_path(cwd, filename);

    fp = fopen(filename, mode);

    free((char*) path);

    return fp;
}

char* get_dir(char* file) {
    // /dir/dir2/dir3/file = /dir/dir2/dir3/

    int loc = find_last(file, path_sep_c);

    uint length = loc + 1;

    char* dir = malloc((length + 1) * sizeof(char));

    memcpy(dir, file, length);
    dir[length] = '\0';

    return dir;
}

char* get_file_name_stripped(const char* file_path) {
    // /dir/dir2/dir3/file.x = file

    const int loc = find_last(file_path, '/');

    const char* start = NULL;

    if (loc == -1) start = file_path;
    else start = &file_path[loc + 1];

    uint len = 0;

    while (start[len] != '\0' && start[len] != '.') len++;

    char* file_name = malloc(sizeof (char) * (len + 1));
    memcpy(file_name, start, len);
    file_name[len] = '\0';

    return file_name;
}

char* get_file_name(const char* file_path) {
    // /dir/dir2/dir3/file.x = file.x

    const int loc = find_last(file_path, '/');

    const char* start = NULL;

    if (loc == -1) start = file_path;
    else start = &file_path[loc + 1];

    const uint len = strlen(start);
    char* file_name = malloc(sizeof (char) * (len + 1));
    memcpy(file_name, start, len);
    file_name[len] = '\0';

    return file_name;
}

char* get_path(const char* dir, const char* file) {
    const uint l1 = len(dir);
    const uint l2 = len(file);

    char* ret = malloc((l1 + l2 + 1) * sizeof(char));

    memcpy(ret, dir, l1);
    memcpy(&ret[l1], file, l2);

    ret[l1 + l2] = '\0';

    return ret;
}

bool get_line(FILE* file, Buffer* buffer) {
    uint pos = 0;
    char temp_buff[BUFF_MIN];

    temp_buff[0] = '\0';

    buffer_clear(buffer);

    do {
        if (feof(file) != 0) return buffer->pos != 0;

        const char* fres = fgets(temp_buff, BUFF_MIN, file);

        if (fres == NULL) {
            return pos != 0;
        }

        pos = buffer->pos;

        int res = buffer_concat(buffer, temp_buff);

        if (res != 0) return false;

    } while (strchr(&buffer->data[pos], '\n') == NULL);

    return true;
}
