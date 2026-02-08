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

    if (!lex_res.succ) {
        return FAIL;
    }

    Parsed parse_res= parse(&lex_res.tokens);

    if (!parse_res.succ) {
        return FAIL;
    }

    print_root(&parse_res.root);

    const MetaData* meta= &parse_res.root.meta->meta;
    const char* ofile_path= make_path(output_folder, meta->name, "c");
    const char* hfile_path= make_path(output_folder, meta->name, "h");

    FILE* ofile= fopen(ofile_path, "w");
    FILE* hfile= fopen(hfile_path, "w");

    if (!ofile || !hfile) {
        return FAIL;
    }

    setbuf(ofile, NULL);
    setbuf(hfile, NULL);

    int gen_res= generate(&parse_res.root, ofile, hfile, meta->name);

    fflush(ofile);
    fflush(hfile);

    fclose(ofile);
    fclose(hfile);

    return gen_res;
}

const char* disassemble_to_str(const uint8_t* data) {
    lex("/home/jamestbest/Anura/x64.txt");

    return NULL;
}

