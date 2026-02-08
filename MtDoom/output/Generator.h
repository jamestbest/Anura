//
// Created by jamestbest on 1/27/26.
//

#ifndef ANURA_GENERATOR_H
#define ANURA_GENERATOR_H

#include "parser/Parser.h"
#include <stdio.h>

int generate(RootNode* root, FILE* output_file, FILE* header_file, const char* ISA_name);

#endif //ANURA_GENERATOR_H