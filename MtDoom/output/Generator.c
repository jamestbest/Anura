//
// Created by jamestbest on 1/27/26.
//

#include "Generator.h"
#include "GeneratorInternal.h"
#include "Helper_String.h"

FILE* ofile;
FILE* hfile;

int generate_data_statement(DataNode* data);

int generate(RootNode* root, FILE* output_file, FILE* header_file) {
    if (!root || !output_file || !header_file) return FAIL;

    ofile= output_file;
    hfile= header_file;

    for (int i = 0; i < root->child_nodes.pos; ++i) {
        Node* child= vector_get_unsafe(&root->child_nodes, i);

        const int res= generate_statement(child);
        if (res != SUCCESS) return res;
    }

    return SUCCESS;
}

int generate_statement(Node* statement) {
    switch (statement->type) {
        case NT_DATA: generate_data_statement((DataNode*)statement);
        case NT_ALIAS: generate_alias_statement(statement);
        default: return error("Unexpected statement start node got %s", types_to_string(statement->type));
    }
}

int generate_data_statement(DataNode* data) {
    const char* cname= capitalised(data->name);
    fprintf(hfile,
        "typedef struct DATA_%s {\n",
        cname
    );

    for (int i = 0; i < data->fields.pos; ++i) {
        FieldNode* field= FieldNode_vec_get_unsafe(&data->fields, i);

        if (!field->named) {
            fprintf(hfile,"\t// IMM: ");
            fprint_lit_num(hfile, &field->num);
            fnewline(hfile);
        } else {
            const char* type= "uint8_t";
            if (field->named_info.bits > 32) type= "uint64_t";
            else if (field->named_info.bits > 16) type= "uint32_t";
            else if (field->named_info.bits > 8) type= "uint16_t";

            fprintf(hfile,
                "\t%s %s: %u;\n",
                type,
                field->named_info.name,
                field->named_info.bits
            );
        }
    }

    fprintf(hfile,
        "} DATA_%s;\n\n",
        cname
    );

    free((void*)cname);
}

int generate_alias_statement(Alias* alias) {
    fprintf(hfile,
        "AVAL AVAL_%s")
}