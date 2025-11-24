//
// Created by jamescoward on 11/11/2023.
//

#ifndef ATOMIC_HELPER_FILE_H
#define ATOMIC_HELPER_FILE_H

#include "../Commons.h"


#include "Buffer.h"

typedef unsigned int uint;

#ifdef _WIN32
#define path_sep_c '\\'
#define path_sep_s "\\"
#else
#define path_sep_c '/'
#define path_sep_s "/"
#endif

char* get_dir(char* file);
char* get_path(const char* dir, const char* file);
bool get_line(FILE* file, Buffer* buffer);
char* get_file_name(const char* file_path);
char* get_file_name_stripped(const char* file_path);

FILE* open_file(const char* cwd, const char* filename, const char* mode);

#endif //ATOMIC_HELPER_FILE_H
