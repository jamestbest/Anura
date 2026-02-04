//
// Created by james on 20/11/25.
//

#include "MtDoom.h"

#include "Errors.h"
#include "Helper_File.h"
#include "lexer/Lexer.h"
#include "output/Generator.h"
#include "parser/Parser.h"

int main() {
    return generate_dissassembler("/home/jamestbest/Anura/x64.txt", "/home/jamestbest/Anura/x64") == SUCCESS;
}

int generate_dissassembler(const char* isdl_file_path, char* output_folder) {
    LexRet lex_res= lex(isdl_file_path);

    if (lex_res.succ != SUCCESS) {
        return FAIL;
    }

    Parsed parse_res= parse(&lex_res.tokens);

    if (parse_res.succ != SUCCESS) {
        return FAIL;
    }

    const char* ofile_path= make_path(output_folder, parse_res.root.meta.name, "c");
    const char* hfile_path= make_path(output_folder, parse_res.root.meta.name, "h");

    FILE* ofile= fopen(ofile_path, "w");
    FILE* hfile= fopen(hfile_path, "w");

    if (!ofile || !hfile) {
        return FAIL;
    }

    return generate(&parse_res.root, ofile, hfile);
}

const char* disassemble_to_str(const uint8_t* data) {
    lex("/home/jamestbest/Anura/x64.txt");

    return NULL;
}

