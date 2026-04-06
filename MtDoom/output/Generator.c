//
// Created by James Coward on 1/27/26.
//

#include "Generator.h"

#include "Buffer.h"
#include "GeneratorInternal.h"
#include "Helper_String.h"

FILE* ofile;
FILE* hfile;

Vector alias_defer;

static int generate_header_data();

static int generate_data_statement(DataNode* data);
static int generate_init_function();
static int generate_string_functions(Vector strings);
static int generate_disassembly_func(StructureNode* structure);

static int generate_statement(Node* statement);
static int generate_flag(FlagNode* node);
static int generate_calculate(CalcNode* calc);
static int generate_alias_statement(AliasNode* alias);
static int generate_l_rule(RuleNodeL* rule, uint8_t depth);
static int generate_left_rules(LeftRules* left_rules, uint8_t depth, bool include_return);
static int generate_rules(RuleArray* rules, uint8_t depth, AliasNode* alias);
static int generate_lr_rule(RuleNodeLR* rule, uint8_t depth, AliasNode* alias);
static int generate_right_rules(RightRule* rule, uint8_t depth, AliasNode* alias);
static int generate_if_rule(RuleNodeIf* node, uint8_t depth, AliasNode* alias);
static int generate_when_rule(RuleNodeWhen* node, uint8_t depth, AliasNode* alias);
static int generate_string_eval(const LitStringData* string);
static int generate_expression(Node* expr);
static int generate_expr(ExprNode* expr);
static int generate_expression_unwrapped(Node* base);
static int generate_expression_value(Node* expr);
static int generate_expression_output(Node* expr);
static int generate_expression_output_unwrapped(const Node* base, const TYPE type);
static int generate_ident(const IdentNode* ident);
static int generate_ident_as_value(const IdentNode* ident);
static int generate_operand(OperandNode* node);
static int generate_binary(BinNode* node);
static int generate_unary(UnaryNode* unary);
static int generate_rule(RuleRightNode* rule);
static int generate_var(VarNode* var);
static int generate_with_rule(RuleNodeWith* node, uint8_t depth, AliasNode* alias);
static int generate_with_functions(const Vector* withs);
static int generate_reset_func(RootNode* root);

static void generate_prefix(uint8_t depth);

static int error(const char* string, ...);
static int info(const char* message, ...);

// todo add restore for parsing data

int generate(RootNode* root, FILE* output_file, FILE* header_file, const char* ISA_name) {
    if (!root || !output_file || !header_file) return FAIL;

    ofile= output_file;
    hfile= header_file;

    alias_defer= vector_create();

    generate_header_data();

    fprintf(ofile,
        "#include \"%s.h\"\n\n",
        ISA_name
    );

    generate_disassembly_func(root->structure);
    generate_reset_func(root);

    generate_string_functions(root->strings);
    generate_with_functions(&root->withs);

    for (int i = 0; i < root->child_nodes.pos; ++i) {
        Node* child= vector_get_unsafe(&root->child_nodes, i);

        const int res= generate_statement(child);
        if (res != SUCCESS)
            return res;
    }

    generate_init_function();

    return SUCCESS;
}

int generate_reset_func(RootNode* root) {
    fprintf(ofile, "void reset() {\n");
    for (int i = 0; i < root->child_nodes.pos; ++i) {
        Node* child= vector_get_unsafe(&root->child_nodes, i);

        switch (child->type) {
            case NT_DATA: {
                const DataNode* data= (DataNode*)child;

                fprintf(ofile, "\tdata_%s.parsed= false;\n", data->name);
                if (data->non_fielded) {
                    fprintf(ofile, "\tdata_%s._value= 0;\n", data->name);
                } else {
                    bool has_any_defaults= false;
                    fprintf(ofile, "\tdata_%s= (DATA_%s){\n", data->name, data->capitalised);
                    for (int j = 0; j < data->all_fields.pos; ++j) {
                        const FieldNode* field= FieldNode_vec_get_unsafe(&data->all_fields, j);

                        if (!field->named) assert(false);
                        if (field->named_info.has_default) {
                            has_any_defaults= true;
                            fprintf(ofile, "\t\t.%s= ", field->named_info.name);
                            fprint_simple_num(ofile, &field->named_info.default_value);
                            fprintf(ofile, ",\n");
                        }
                    }
                    if (!has_any_defaults) fprintf(ofile, "\t\t0\n");
                    fprintf(ofile, "\t};\n");
                }
                break;
            }
            case NT_ALIAS: {
                const AliasNode* alias= (AliasNode*)child;
                fprintf(ofile, "\tclear_aval(&aval_%s);\n", alias->identifier);
                break;
            }
            case NT_FLAG: {
                const FlagNode* flag= (FlagNode*)child;
                fprintf(ofile, "\tflag_%s= FLAG_%s_VALUE_%s;\n", flag->name, flag->capitalised, (char*)vector_get_unsafe(&flag->enum_values, flag->default_value));
                fprintf(ofile, "\tflag_calculated_%s= false;\n", flag->name);
                break;
            }
        }
    }
    fprintf(ofile, "}\n\n");

    return SUCCESS;
}

int generate_disassembly_func(StructureNode* structure) {
    fprintf(ofile,
        "int disassemble(const char** output, uintptr_t rip){\n"
        "\treset();\n\n"
        "\tdata_rip._value= rip;\n"
        "\tdata_rip.parsed= true;\n\n");
    for (int i = 0; i < structure->rules.pos; ++i) {
        const MarkedIdent* ident= MarkedIdent_arr_ptr(&structure->rules, i);

        switch (ident->type) {
            case MARKED_QUESTION:
                fprintf(ofile, "\tparse_%s(&top_stream);\n", link_name(ident->ident));
                break;
            case MARKED_STAR:
                fprintf(ofile, "\twhile (parse_%s(&top_stream)) {}\n", link_name(ident->ident));
                break;
            case MARKER_NONE:
                fprintf(ofile, "\tif (!parse_%s(&top_stream)) return false;\n", link_name(ident->ident));
                break;
        }

        if (i != structure->rules.pos - 1) fnewline(ofile);
    }
    switch (structure->output.base->type) {
        case NT_IDENT: {
            const IdentNode* alias= (IdentNode*)structure->output.single_out;
            fprintf(ofile, "\t*output= get_aval_%s().chosen_val;\n", alias->name);
            break;
        }
        case NT_LIT_STRING: {
            const LitNode* lit= (LitNode*)structure->output.single_out;
            fprintf(ofile, "\t*output= ");
            generate_string_eval(&lit->data.lit_string);
            fprintf(ofile, ";\n");
        }
    }
    fprintf(ofile,"\treturn true;\n"
                   "}\n\n");

    return SUCCESS;
}

int generate_header_data() {
    fprintf(hfile, "#include \"default.h\"\n\n");

    return SUCCESS;
}

int generate_statement(Node* statement) {
    if (!statement) {
        return info("Skipping NULL statement");
    }

    switch (statement->type) {
        case NT_DATA: return generate_data_statement((DataNode*)statement);
        case NT_ALIAS: return generate_alias_statement((AliasNode*)statement);
        case NT_FLAG: return generate_flag((FlagNode*)statement);
        case NT_RULE_RIGHT_STMT: return generate_rule((RuleRightNode*)statement);
        case NT_CALCULATE: return generate_calculate((CalcNode*)statement);
        case NT_VAR: return generate_var((VarNode*)statement);
        case NT_STRUCTURE:
            return SUCCESS;
        default: return error("Unexpected statement start node got %s", types_to_string(statement->type));
    }
}

const char* size_type(uint64_t size) {
    if (size > 32) return "uint64_t";
    if (size > 16) return "uint32_t";
    if (size > 8) return "uint16_t";
    return "uint8_t";
}

int generate_var(VarNode* var) {
    switch (var->link->type) {
        case NT_DATA: {
            const DataNode* data= (DataNode*)var->link;
            fprintf(hfile, "DATA_%s var_%s= ", data->capitalised, var->identifier);
            generate_expr(var->value);
            break;
        }
        case NT_ALIAS: {
            const AliasNode* alias= (AliasNode*)var->link;
            fprintf(hfile, "AVAL var_%s= (AVAL){.chosen_val=", var->identifier);
            generate_expression_output((Node*)var->value);
            fprintf(hfile, ", .chosen_idx= AVAL_STATUS_SELECTED}");

            break;
        }
        case NT_FLAG: {
            const FlagNode* flag= (FlagNode*)var->link;
            fprintf(hfile, "FLAG_%s var_%s= ", flag->capitalised, var->identifier);
            const IdentNode* ident= (IdentNode*)var->value->expr;
            if (ident->link->type == NT_FLAG_VALUE) {
                const FlagValueNode* fv= (FlagValueNode*)ident->link;
                fprintf(hfile, "FLAG_%s_VALUE_%s", flag->capitalised, fv->name);
            } else {
                generate_expr(var->value);
            }
            break;
        }
        default: assert(false);
    }

    fprintf(hfile, ";\n");
    return SUCCESS;
}

static int generate_if_braced_rules(const IfBracedRules* rules, const char* fname, const char* fcap, uint8_t depth);

int generate_if_braced_output(const Node* output, const char* fname, const char* fcap, uint8_t depth) {
    switch (output->type) {
        case NT_VAR: {
            generate_prefix(depth);
            const VarNode* var= (VarNode*)output;
            fprintf(ofile, "flag_%s= var_%s; return;\n", fname, var->identifier);
            break;
        }
        case NT_FLAG_VALUE: {
            generate_prefix(depth);
            const FlagValueNode* fv= (FlagValueNode*)output;
            fprintf(ofile, "flag_%s= FLAG_%s_VALUE_%s; return;\n",
               fname,
               fcap,
               fv->name
            );
            break;
        }
        case NT_IF_BRACED_RULES: {
            const IfBracedRules* rs= (IfBracedRules*)output;
            generate_if_braced_rules(rs, fname, fcap, depth);
            break;
        }
        default: assert(false);
    }

    return SUCCESS;
}

int generate_if_braced_rules(const IfBracedRules* rules, const char* fname, const char* fcap, uint8_t depth) {
    for (int i = 0; i < rules->rules.pos; ++i) {
        const IfFlagRule* rule= IfFlagRule_arr_ptr(&rules->rules, i);

        generate_prefix(depth);
        switch (rule->base->type) {
            case NT_RULE_IF: {
                const RuleNodeIf* if_node= rule->if_rule;
                fprintf(ofile, "if (");
                generate_expression_value(if_node->condition);
                fprintf(ofile, "){\n");
                generate_if_braced_output(if_node->output.base, fname, fcap, depth + 1);
                generate_prefix(depth);
                fprintf(ofile, "}\n");
                break;
            }
            case NT_VAR:
            case NT_FLAG_VALUE:
            case NT_IF_BRACED_RULES:
                generate_if_braced_output(rule->base, fname, fcap, depth + 1);
                break;

            default: assert(false);
        }
    }

    return SUCCESS;
}

int generate_calculate(CalcNode* calc) {
    fprintf(hfile, "void calculate_flag_%s();", calc->identifier);

    fprintf(ofile, "void calculate_flag_%s() {\n", calc->identifier);

    generate_if_braced_rules(&calc->rules, calc->identifier, calc->capitalised, 1);

    fprintf(ofile, "}\n");

    return SUCCESS;
}

int generate_rule(RuleRightNode* rule) {
    fprintf(hfile, "int calculate_rule_right_%u();\n\n", rule->id);

    fprintf(ofile, "int calculate_rule_right_%u() {\n"
                   "\tuint16_t choice= 0;\n\n", rule->id);
    for (int i = 0; i < rule->expressions.pos; ++i) {
        Node* expr= Node_vec_get_unsafe(&rule->expressions, i);

        fprintf(ofile, "\tif (");
        generate_expression_value(expr);
        fprintf(ofile, ") choice= %u;\n", i);
    }
    fprintf(ofile, "\treturn choice;\n"
                   "}\n\n"
    );

    return SUCCESS;
}

int generate_data_statement(DataNode* data) {
    const char* cname= data->capitalised;
    fprintf(hfile,
        "typedef struct DATA_%s {\n"
        "\tuint8_t parsed: 1;\n",
        cname
    );

    if (data->non_fielded) {
        fprintf(hfile, "\t%s _value: %lu;\n", size_type(data->bits), data->bits);
    } else {
        for (int i = 0; i < data->all_fields.pos; ++i) {
            const FieldNode* field= FieldNode_vec_get_unsafe(&data->all_fields, i);

            if (!field->named) {
                fprintf(hfile,"\t// IMM: ");
                fprint_simple_num(hfile, &field->num);
                fnewline(hfile);
            } else {
                const char* type= size_type(field->named_info.bits);
                fprintf(hfile,
                    "\t%s %s: %u;\n",
                    type,
                    field->named_info.name,
                    field->named_info.bits
                );
            }
        }
    }

    fprintf(hfile,
        "} DATA_%s;\n",
        cname
    );

    fprintf(hfile,
        "DATA_%s data_%s= {",
        cname,
        data->name
    );
    bool has_any_defaults= false;
    for (int i = 0; i < data->all_fields.pos; ++i) {
        FieldNode* field= FieldNode_vec_get_unsafe(&data->all_fields, i);
        if (!field->named) continue;
        if (!field->named_info.has_default) continue;

        has_any_defaults= true;
        fprintf(hfile, "\n\t.%s= %lu,\n", field->named_info.name, field->named_info.default_value.value);
    }
    if (!has_any_defaults) fprintf(hfile, "0");
    fprintf(hfile, "};\n");

    fprintf(hfile,
        "bool parse_%s(ByteStream* stream);\n\n",
        data->name
    );

    fprintf(ofile,
        "DATA_%s parse_%s_(ByteStream* stream) {\n",
        data->capitalised,
        data->name
    );

    if (data->non_fielded) {
        fprintf(ofile,
            "\tuint64_t res= read_bits(stream, %lu);\n"
            "\treturn (DATA_%s){._value= res, .parsed= true};\n",
            data->bits,
            data->capitalised
        );
    } else {
        fprintf(ofile, "\tDATA_%s res;\n\n", data->capitalised);

        for (int i = 0; i < data->rows.pos; ++i) {
            const FieldNodeVector* row= FieldNodeVector_arr_ptr(&data->rows, i);

            fprintf(ofile, "\tconst size_t pos_save%u= stream->pointer;\n\n", i);
            fprintf(ofile, "\tif (");

            for (int j = 0; j < row->pos; ++j) {
                const FieldNode* field= FieldNode_vec_get_unsafe(row, j);
                if (!field->named) {
                    fprintf(ofile, "EXPECT_%s(", field->num.show_as_bin ? "BITS" : "BYTE");
                    fprint_simple_num(ofile, &field->num);
                    fprintf(ofile, ", stream)");
                } else {
                    fprintf(ofile, "(res.%s= read_bits(stream, %u), true)",
                        field->named_info.name,
                        field->named_info.bits
                    );
                }
                if (j != row->pos - 1) {
                    fprintf(ofile, " &&\n\t\t");
                }
            }

            fprintf(ofile, "\n\t) {\n"
                           "\t\tres.parsed= true;\n"
                           "\t\treturn res;\n\t}\n\n"
            );

            fprintf(ofile, "\tstream->pointer= pos_save%u;\n\n", i);
        }

        fprintf(ofile, "\tres.parsed= false;\n"
                       "\treturn res;\n");
    }

    fprintf(ofile, "}\n\n");

    fprintf(ofile,
        "bool parse_%s(ByteStream* stream) {\n"
        "\tconst DATA_%s res= parse_%s_(stream);\n"
        "\tif (!res.parsed) return false;\n"
        "\tdata_%s= res;\n"
        "\treturn true;\n"
        "}\n\n",
        data->name,
        data->capitalised,
        data->name,
        data->name
    );

    return SUCCESS;
}

int generate_init_function() {
    fprintf(ofile,
        "int init() {\n"
    );
    for (int i = 0; i < alias_defer.pos; ++i) {
        fprintf(ofile,
            "\taval_%s.choices= vector_create();\n",
            (char*)vector_get_unsafe(&alias_defer, i)
        );
    }
    fprintf(ofile,
        "\treturn 0;\n"
        "}\n\n"
    );

    return SUCCESS;
}

int generate_string_functions(const Vector strings) {
    for (int i = 0; i < strings.pos; ++i) {
        const LitNode* string= vector_get_unsafe(&strings, i);
        const LitStringData data= string->data.lit_string;

        if (data.expressions.pos == 0) continue;

        for (int j = 0; j < data.expressions.pos; ++j) {
            fprintf(hfile, "void eval_string_%u_field_%u(Buffer* buff);\n", data.id, j);
        }
        fnewline(hfile);

        for (int j = 0; j < data.expressions.pos; ++j) {
            fprintf(ofile, "void eval_string_%u_field_%u(Buffer* buff) {\n", data.id, j);
            fprintf(ofile, "\tbuffer_concat(buff, ");
            generate_expression_output(Node_vec_get_unsafe(&data.expressions, j));
            fprintf(ofile, ");\n}\n\n");
        }
    }

    return SUCCESS;
}

int generate_alias_statement(AliasNode* alias) {
    fprintf(hfile,
        "AVAL aval_%s= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1, .parsed_successfully= %s};\n",
        alias->identifier,
        alias->is_flat ? "true" : "false"
    );
    fprintf(hfile,
        "AVAL get_aval_%s();\n"
        "bool parse_%s(ByteStream* stream);\n"
        "ParseRet parse_%s_(ByteStream* stream);\n\n",
        alias->identifier,
        alias->identifier,
        alias->identifier
    );

    vector_add(&alias_defer, (void*)alias->identifier);

    fprintf(ofile, "ParseRet parse_%s_(ByteStream* stream) {\n", alias->identifier);
    generate_rules(&alias->rules.rules, 1, alias);
    fprintf(ofile,"\treturn PARSE_FAIL; \n}\n\n");

    fprintf(ofile, "bool parse_%s(ByteStream* stream) {\n", alias->identifier);
    fprintf(ofile, "\tParseRet res= parse_%s_(stream);\n", alias->identifier);
    fprintf(ofile, "\tif (!res.success) return res.success;\n"
                   "\taval_%s= res.aval;\n",
                   alias->identifier
    );

    if (alias->linked_rule) {
        fprintf(ofile, "\tif (res.aval.chosen_idx == AVAL_STATUS_NONE) {\n"
                       "\t\taval_%s.chosen_idx= calculate_rule_right_%u();\n"
                       "\t\taval_%s.chosen_val= vector_get_unsafe(&aval_%s.choices, aval_%s.chosen_idx);\n"
                       "\t}\n",
                       alias->identifier,
                       alias->linked_rule->id,
                       alias->identifier,
                       alias->identifier,
                       alias->identifier
        );
    }

    fprintf(ofile,
        "\treturn res.success;\n"
        "}\n\n"
    );

    fprintf(ofile, "AVAL get_aval_%s() {\n", alias->identifier);
    if (alias->is_flat) {
        fprintf(ofile, "\tparse_%s(&top_stream);\n", alias->identifier);
    }
    if (alias->linked_rule) {
        fprintf(ofile, "\tint choice= calculate_rule_right_%u();\n", alias->linked_rule->id);
        fprintf(ofile, "\tchar* data= vector_get_unsafe(&aval_%s.choices, choice);\n", alias->identifier);
        fprintf(ofile, "\taval_%s.chosen_val= data;\n"
                       "\taval_%s.chosen_idx= choice;\n", alias->identifier, alias->identifier);
    }
    fprintf(ofile, "\treturn aval_%s;\n", alias->identifier);
    fprintf(ofile, "}\n\n");

    return SUCCESS;
}

int generate_rules(RuleArray* rules, uint8_t depth, AliasNode* alias) {
    for (int i = 0; i < rules->pos; ++i) {
        Rule* rule= Rule_arr_ptr(rules, i);

        generate_prefix(depth);
        fprintf(ofile, "size_t pos_save_%u= stream->pointer;\n\n", i);

        switch (rule->base.type) {
            case NT_RULE_IF: generate_if_rule(&rule->data.rule_if, depth, alias); break;
            case NT_RULE_WHEN: generate_when_rule(&rule->data.rule_when, depth, alias); break;
            case NT_RULE_L: generate_l_rule(&rule->data.rule_l, depth); break;
            case NT_RULE_LR: generate_lr_rule(&rule->data.rule_lr, depth, alias); break;
            case NT_RULE_WITH: generate_with_rule(&rule->data.rule_with, depth, alias); break;
            default: assert(false);
        }
        fnewline(ofile);

        generate_prefix(depth);
        fprintf(ofile, "stream->pointer= pos_save_%u;\n\n", i);
    }

    return SUCCESS;
}

int generate_with_functions(const Vector* withs) {
    for (int i = 0; i < withs->pos; ++i) {
        const RuleNodeWith* with= vector_get_unsafe(withs, i);
        fprintf(hfile, "ParseRet parse_with_%zu(ByteStream* stream);\n\n", with->id);

        fprintf(ofile, "ParseRet parse_with_%zu(ByteStream* stream) {\n", with->id);
        generate_rules(&with->brace->rules, 1, with->alias);
        fprintf(ofile, "return PARSE_FAIL;\n}\n\n");
    }

    return SUCCESS;
}

int generate_restore(Node* node, const uint8_t depth, const size_t prefix) {
    const IdentNode* left= (IdentNode*)node;
    generate_prefix(depth);

    bool is_var= left->link->type == NT_VAR;
    Node* base= base_link(left);
    switch (base->type) {
        case NT_DATA: {
            const DataNode* data= (DataNode*)base;
            fprintf(ofile, "%s_%s= save_%s_%zu", is_var ? "var" : "data", left->name, left->name, prefix);
            break;
        }
        case NT_ALIAS: {
            const AliasNode* alias= (AliasNode*)base;
            fprintf(ofile, "%s_%s= save_%s_%zu", is_var ? "var" : "aval", left->name, left->name, prefix);
            break;
        }
        case NT_FLAG: {
            const FlagNode* flag= (FlagNode*)base;
            fprintf(ofile, "%s_%s= save_%s_%zu", is_var ? "var" : "flag", left->name, left->name, prefix);
            break;
        }
        default: assert(false);
    }

    fprintf(ofile, ";\n");

    return SUCCESS;
}

int generate_save(Node* node, const uint8_t depth, const size_t prefix) {
    const IdentNode* left= (IdentNode*)node;
    generate_prefix(depth);

    bool is_var= left->link->type == NT_VAR;
    Node* base= base_link(left);
    switch (base->type) {
        case NT_DATA: {
            const DataNode* data= (DataNode*)base;
            fprintf(ofile, "DATA_%s save_%s_%zu= %s_%s", data->capitalised, left->name, prefix, is_var ? "var" : "data", left->name);
            break;
        }
        case NT_ALIAS: {
            const AliasNode* alias= (AliasNode*)base;
            fprintf(ofile, "AVAL save_%s_%zu= %s_%s", left->name, prefix, is_var ? "var" : "aval", left->name);
            break;
        }
        case NT_FLAG: {
            const FlagNode* flag= (FlagNode*)base;
            fprintf(ofile, "FLAG_%s save_%s_%zu= %s_%s", flag->capitalised, left->name, prefix, is_var ? "var" : "flag", left->name);
            break;
        }
        default: assert(false);
    }

    fprintf(ofile, ";\n");

    return SUCCESS;
}

int generate_assign(const AssignNode* assign, const uint8_t depth) {
    const IdentNode* left= (IdentNode*)assign->left;

    generate_prefix(depth);
    bool is_var= left->link->type == NT_VAR;
    Node* base= base_link(left);
    switch (base->type) {
        case NT_ALIAS: {
            fprintf(ofile, "%s_%s.chosen_val= ", is_var ? "var" : "aval", left->name);
            generate_expression_output((Node*)assign->right);
            break;
        }
        case NT_DATA: {
            fprintf(ofile, "%s_%s._value= ", is_var ? "var" : "data", left->name);
            generate_expression_value((Node*)assign->right);
            break;
        }
        case NT_FLAG: {
            fprintf(ofile, "%s_%s= ", is_var ? "var" : "flag", left->name);
            const FlagNode* flag= (FlagNode*)base;
            const ExprNode* expr= assign->right;
            const IdentNode* ident= (IdentNode*)expr->expr;
            if (ident->link->type == NT_FLAG_VALUE) {
                fprintf(ofile, "FLAG_%s_VALUE_%s", flag->capitalised, ident->name);
            } else generate_ident(ident);
            break;
        }
        default: assert(false);
    }

    fprintf(ofile, ";\n");

    return SUCCESS;
}

int generate_with_rule(RuleNodeWith* node, uint8_t depth, AliasNode* alias) {
    /*
    WITH default_opmode = 64bit { }
    */
    // for each assign save the original value
    // assign the new value
    // call inner with function
    // restore values
    for (int i = 0; i < node->assignNodes.pos; ++i) {
        const AssignNode* assign= vector_get_unsafe(&node->assignNodes, i);

        generate_save((Node*)assign->left, depth, i);
        generate_assign(assign, depth);

        generate_prefix(depth);
        fprintf(ofile, "ParseRet res_%zu= parse_with_%zu(stream);\n", node->id, node->id);
        generate_restore((Node*)assign->left, depth, i);

        generate_prefix(depth);
        fprintf(ofile, "if (res_%zu.success) return res_%zu;\n", node->id, node->id);
    }

    return SUCCESS;
}

int generate_if_rule(RuleNodeIf* node, uint8_t depth, AliasNode* alias) {
    fprintf(ofile, "if (");
    generate_expression_value(node->condition);
    fprintf(ofile, ") {\n");
    generate_right_rules(&node->output, depth, alias);
    fnewline(ofile);
    generate_prefix(depth);
    fprintf(ofile, "}\n");

    return SUCCESS;
}

int generate_when_rule(RuleNodeWhen* node, uint8_t depth, AliasNode* alias) {
    fprintf(ofile, "if (");
    generate_expression_value(node->condition);
    fprintf(ofile, ") {\n");
    generate_right_rules(&node->output, depth + 1, alias);
    fprintf(ofile, "}\n");

    return SUCCESS;
}

int generate_flag(FlagNode* node) {
    fprintf(hfile, "typedef enum FLAG_%s{\n", node->capitalised);
    for (int i = 0; i < node->enum_values.pos; ++i) {
        const char* name= vector_get_unsafe(&node->enum_values, i);

        fprintf(hfile, "\tFLAG_%s_VALUE_%s%c\n", node->capitalised, name, i == node->enum_values.pos - 1 ? ' ' : ',');
    }
    fprintf(hfile, "} FLAG_%s;\n", node->capitalised);
    const char* default_value= vector_get_unsafe(&node->enum_values, node->default_value);
    fprintf(hfile, "FLAG_%s flag_%s= FLAG_%s_VALUE_%s;\n", node->capitalised, node->name, node->capitalised, default_value);
    fprintf(hfile, "FLAG_%s get_flag_%s();\n", node->capitalised, node->name);
    fprintf(hfile, "bool flag_calculated_%s= false;\n\n", node->name);

    fprintf(ofile, "FLAG_%s get_flag_%s() {\n"
                   "\tif (flag_calculated_%s) return flag_%s;\n",
                   node->capitalised, node->name,
                   node->name, node->name
    );
    if (node->linked_calc) {
        fprintf(ofile, "\tcalculate_flag_%s();\n", node->name);
    }
    fprintf(ofile,
        "\tflag_calculated_%s= true;\n"
        "\treturn flag_%s;\n"
        "}\n\n",
        node->name,
        node->name
    );

    return SUCCESS;
}

int generate_expression_value_unwrapped(const Node* base) {
    switch (base->type) {
        case NT_IDENT: {
            generate_ident_as_value((IdentNode*)base);
            break;
        }
        case NT_LIT_NUM: {
            fprint_simple_num(ofile, &((LitNode*)base)->data.lit_number);
            break;
        }
        case NT_LIT_STRING: {
            break;
        }
        case NT_BIN_EXPR: {
            generate_binary((BinNode*)base);
            break;
        }
        case NT_UNARY_EXPR: {
            generate_unary((UnaryNode*)base);
            break;
        }
    }

    return SUCCESS;
}

int generate_expression_value(Node* expr) {
    const ExprNode* e= (ExprNode*)expr;
    const Node* base= e->expr;

    return generate_expression_value_unwrapped(base);
}

int generate_expression_output_unwrapped(const Node* base, const TYPE type) {
    bool end_paren= false;
    switch (type) {
        case TYPE_NUMBER: fprintf(ofile, "data_to_string("); end_paren = true; break;
    }

    switch (base->type) {
        case NT_IDENT: {
            generate_ident_as_value((IdentNode*)base);
            break;
        }
        case NT_LIT_NUM: {
            fprint_simple_num(ofile, &((LitNode*)base)->data.lit_number);
            break;
        }
        case NT_LIT_STRING: {
            break;
        }
        case NT_BIN_EXPR: {
            generate_binary((BinNode*)base);
            break;
        }
        case NT_UNARY_EXPR: {
            generate_unary((UnaryNode*)base);
            break;
        }
    }

    if (end_paren)
        fprintf(ofile, ")");

    return SUCCESS;
}

int generate_expression_output(Node* expr) {
    const ExprNode* e= (ExprNode*)expr;
    const Node* base= e->expr;

    return generate_expression_output_unwrapped(base, e->type.base);
}

int generate_expression_unwrapped(Node* base) {
    switch (base->type) {
        case NT_IDENT: {
            generate_ident((IdentNode*)base);
            break;
        }
        case NT_LIT_NUM: {
            fprint_simple_num(ofile, &((LitNode*)base)->data.lit_number);
            break;
        }
        case NT_LIT_STRING: {
            assert(false);
        }
        case NT_BIN_EXPR: {
            generate_binary((BinNode*)base);
            break;
        }
        case NT_UNARY_EXPR: {
            generate_unary((UnaryNode*)base);
            break;
        }
    }

    return SUCCESS;
}

int generate_expr(ExprNode* expr) {
    return generate_expression((Node*)expr);
}

int generate_expression(Node* expr) {
    const ExprNode* e= (ExprNode*)expr;
    const Node* base= e->expr;
    return generate_expression_unwrapped(base);
}

int generate_unary(UnaryNode* unary) {
    switch (unary->op) {
        case NOT:
            fprintf(ofile, "!(");
            generate_operand(&unary->operand);
            fprintf(ofile, ")");
            break;
        case EXISTS: {
            const IdentNode* ident= unary->operand.ident;
            fprintf(ofile,
                "aval_%s.parsed_successfully",
                link_name(ident->link)
            );
            break;
        }
    }
    return SUCCESS;
}

const char* bin_op_to_symbol(BinaryOperator op) {
    switch (op) {
        case EQUALITY: return "==";
        case NEQUALITY: return "!=";
        case DOT: return ".";
        case STAR: return "*";
        case AND: return "&&";
        case ADD: return "+";
        case SUB: return "-";
        case BINARY_OP_COUNT:
        case POW:
        default:
            return "<<ERROR>>;{assert(false);};";
    }
}

int generate_flag_equality(IdentNode* left, IdentNode* right, bool is_neq) {
    const bool left_is_enum_lit= left->link->type == NT_FLAG_VALUE;
    const bool right_is_enum_lit= right->link->type == NT_FLAG_VALUE;

    if (left_is_enum_lit && right_is_enum_lit) {
        return error("Cannot compare enum literals together");
    }

    if (!left_is_enum_lit && !right_is_enum_lit) {
        // these are both different flags or exactly the same
        const FlagNode* left_flag= (FlagNode*)left->link;
        const FlagNode* right_flag= (FlagNode*)right->link;
        return error("Cannot compare different flags together (`%s` and `%s`), must be flag and enum literal",
            left_flag->name,
            right_flag->name
        );
    }

    FlagNode* canonical_flag= NULL;
    if (!left_is_enum_lit) {
        canonical_flag = (FlagNode*)left->link;
    }
    if (!right_is_enum_lit) {
        canonical_flag = (FlagNode*)right->link;
    }

    bool valid= false;
    const FlagLinkPosArray* links= NULL;
    if (left_is_enum_lit) {
        const FlagValueNode* flag_value= (FlagValueNode*)left->link;
        links= &flag_value->links;
    }
    if (right_is_enum_lit) {
        const FlagValueNode* flag_value= (FlagValueNode*)right->link;
        links= &flag_value->links;
    }

    size_t enum_pos= 0;
    for (int i = 0; i < links->pos; ++i) {
        const FlagLinkPos* link= FlagLinkPos_arr_ptr(links, i);
        if (link->flag == canonical_flag) {
            valid= true;
            enum_pos= link->enum_pos;
        }
    }

    if (!valid) {
        return error("Canonical flag is `%s` used in expression with enum literal that does not share a enum value",
            canonical_flag->name
        );
    }

    const char* enum_name= vector_get_unsafe(&canonical_flag->enum_values, enum_pos);
    fprintf(ofile,
        "get_flag_%s() %c= FLAG_%s_VALUE_%s",
        canonical_flag->name,
        is_neq ? '!' : '=',
        canonical_flag->capitalised,
        enum_name
    );

    return SUCCESS;
}

bool check_is_flag_ish(Node* node) {
    if (node->type == NT_IDENT) {
        const IdentNode* ident= (IdentNode*)node;
        if (ident->link->type == NT_FLAG_VALUE || ident->link->type == NT_FLAG) {
            return true;
        }
    }
    return false;
}

int generate_binary(BinNode* node) {
    switch (node->op) {
        // FLAG == FLAG_ENUM e.g. mode == 64bit => flag_mode == FLAG_MODE_64bit
        // CANNOT DO ALIAS == x e.g. reg == 010  Aliases don't have exposed values... maybe one day
        //          e.g. could if the thing succeeds copy into the alias before the return using the values of
        //           the lower aliases and data and literals, but not needed for now
        // DATA == LIT e.g. Mod == 01 => data_Mod._value == 0b01
        case EQUALITY:
        case NEQUALITY: {
            const bool special_case= check_is_flag_ish(node->left.node) && check_is_flag_ish(node->right.node);
            if (special_case) return generate_flag_equality(node->left.ident, node->right.ident, node->op == NEQUALITY);
            // FALL THROUGH ON PURPOSE
        }
        case STAR:
        case AND:
        case ADD:
        case SUB:
            generate_operand(&node->left);
            fprintf(ofile, " %s ", bin_op_to_symbol(node->op));
            generate_operand(&node->right);
            break;
        case DOT: {
            const DataNode* data= (DataNode*)node->left.ident->link;
            const IdentNode* ident= node->right.ident;

            fprintf(ofile, "data_%s.%s", data->name, ident->name);
            break;
        }
        case POW:
            fprintf(ofile, "pow(");
            generate_operand(&node->left);
            fprintf(ofile, ", ");
            generate_operand(&node->right);
            fprintf(ofile, ")");
            break;
        case BINARY_OP_COUNT:
        default:
            assert(false);
    }

    return SUCCESS;
}

int generate_operand(OperandNode* node) {
    switch (node->node->type) {
        case NT_IDENT: {
            generate_ident_as_value(node->ident);
            break;
        }
        case NT_LIT_NUM: {
            fprint_simple_num(ofile, &node->lit->data.lit_number);
            break;
        }
        case NT_LIT_STRING: {
            assert(false);
        }
        case NT_BIN_EXPR:
            generate_binary(node->bin);
            break;
        case NT_UNARY_EXPR:
            generate_unary(node->unary);
            break;
        default: assert(false);
    }

    return SUCCESS;
}

int generate_ident_as_value(const IdentNode* ident) {
    generate_ident(ident);
    switch (ident->link->type) {
        case NT_ALIAS: fprintf(ofile, ".chosen_val"); break;
        case NT_DATA: fprintf(ofile, "._value"); break;
        default: assert(false);
    }
    return SUCCESS;
}

int generate_ident(const IdentNode* ident) {
    switch (ident->link->type) {
        case NT_ALIAS: {
            const AliasNode* alias= (AliasNode*)ident->link;
            fprintf(ofile, "get_aval_%s()", alias->identifier);
            break;
        }
        case NT_DATA: {
            const DataNode* data= (DataNode*)ident->link;
            fprintf(ofile, "data_%s", data->name);
            break;
        }
        case NT_FLAG: {
            const FlagNode* flag= (FlagNode*)ident->link;
            fprintf(ofile, "get_flag_%s()", flag->name);
            break;
        }
        case NT_VAR: {
            const VarNode* var= (VarNode*)ident->link;
            fprintf(ofile, "var_%s", var->identifier);
            break;
        }
        case NT_FLAG_VALUE: assert(false);
        default: assert(false);
    }

    return SUCCESS;
}

int generate_lr_rule(RuleNodeLR* rule, uint8_t depth, AliasNode* alias) {
    generate_prefix(depth);
    generate_left_rules(&rule->left, depth, false);

    fprintf(ofile, "{\n");
    generate_right_rules(&rule->right, depth + 1, alias);
    fnewline(ofile);
    generate_prefix(depth);
    fprintf(ofile, "}");

    return SUCCESS;
}

int generate_multi(Node* node, AliasNode* alias) {
    switch (node->type) {
        case NT_IDENT: {
            const IdentNode* ident= (IdentNode*)node;

            switch (ident->link->type) {
                case NT_ALIAS: {
                    const AliasNode* r_alias= (AliasNode*)ident->link;
                    if (!r_alias->linked_rule) {
                        fprintf(ofile, "vector_destroy(&res.choices);\n");
                        fprintf(ofile, "res.choices= aval_%s.choices;", r_alias->identifier);
                    } else {
                        fprintf(ofile, "vector_add(&res.choices, calculate_rule_right_%u());", r_alias->linked_rule->id);
                    }

                    break;
                }
                default: {
                    fprintf(ofile, "vector_add(&res.choices, ");
                    generate_ident_as_value(ident);
                    fprintf(ofile, ");");

                    break;
                }
            }

            break;
        }
        case NT_LIT_STRING: {
            const LitNode* lit= (LitNode*)node;
            const LitStringData* sd= &lit->data.lit_string;

            fprintf(ofile, "vector_add(&res.choices, ");
            generate_string_eval(sd);
            fprintf(ofile, ");\n");

            break;
        }
    }
}

int generate_single_multi(Node* node, AliasNode* alias) {
    switch (node->type) {
        case NT_IDENT: {
            fprintf(ofile,
                "return PARSE_SUCC("
            );

            IdentNode* ident= (IdentNode*)node;
            switch (ident->link->type) {
                case NT_ALIAS: {
                    const AliasNode* r_alias= (AliasNode*)ident->link;
                    fprintf(ofile,
                        "get_aval_%s()",
                        r_alias->identifier
                    );
                    break;
                }
                case NT_DATA: {
                    const DataNode* data= (DataNode*)ident->link;
                    fprintf(ofile,
                        "data_to_aval(data_%s._value)",
                        data->name
                    );
                    break;
                }
                default: assert(false);
            }

            fprintf(ofile, ");");
            break;
        }

        case NT_LIT_STRING: {
            const LitNode* lit= (LitNode*)node;
            fprintf(ofile,
                "return PARSE_SUCC(to_aval("
            );
            generate_string_eval(&lit->data.lit_string);
            fprintf(ofile, ")); /* ");
            fputz_sanitize(ofile, lit->data.lit_string.string);
            fputz(ofile, " */");
            break;
        }
        default:
            assert(false);
    }

    return SUCCESS;
}

int generate_right_rules(RightRule* rule, uint8_t depth, AliasNode* alias) {
    generate_prefix(depth);
    switch (rule->base->type) {
        case NT_MULTI: {
            MultiNode* multi= (MultiNode*)rule->base;
            if (multi->multis.pos == 1) return generate_single_multi(Node_vec_get_unsafe(&multi->multis, 0), alias);

            if (!alias->linked_rule) {
                fprintf(ofile, "AVAL res= (AVAL) {\n"
                               "\t.choices= vector_construct(%zu),\n"
                               "\t.chosen_idx= AVAL_STATUS_NONE,\n"
                               "\t.chosen_val= NULL\n"
                               "};\n", multi->multis.pos);
                for (int i = 0; i < multi->multis.pos; ++i) {
                    generate_prefix(depth);
                    generate_multi(Node_vec_get_unsafe(&multi->multis, i), alias);
                }

                generate_prefix(depth);
                fprintf(ofile, "return PARSE_SUCC(res);\n");

                return SUCCESS;
                // return error("Unable to find linked rule for alias with multiple outputs");
            }

            fprintf(ofile,
                "switch(calculate_rule_right_%u()) {\n", alias->linked_rule->id);

            for (uint i = 0; i < multi->multis.pos; ++i) {
                Node* node= Node_vec_get_unsafe(&multi->multis, i);
                generate_prefix(depth + 1);
                fprintf(ofile, "case %u: ", i);
                generate_single_multi(node, alias);
                fprintf(ofile, "; break;\n");
            }
            generate_prefix(depth);
            fprintf(ofile, "}");
            break;
        }
        case NT_BRACED_RULES: {
            BracedRules* rules= rule->brace;
            generate_rules(&rules->rules, depth + 1, alias);

            break;
        }
        case NT_MAP: {
            MapNode* map= (MapNode*)rule->base;

            fprintf(ofile, "ByteStream stream= stream_create();\n");
            for (int i = 0; i < map->stream.pos; ++i) {
                Node* node= Node_vec_get_unsafe(&map->stream, i);

                generate_prefix(depth);
                fprintf(ofile, "stream_add(&stream, ");
                switch (node->type) {
                    case NT_LIT_NUM: {
                        const LitNode* lit= (LitNode*)node;
                        const SimpleNumData num= lit->data.lit_number;

                        fprint_simple_num(ofile, &num);
                        fprintf(ofile, ", %u);\n", num.bits);
                        break;
                    }
                    case NT_IDENT: {
                        const IdentNode* ident= (IdentNode*)node;
                        const DataNode* data= (DataNode*)ident->link;
                        fprintf(ofile, "data_%s._value, %lu);\n", data->name, data->bits);
                        break;
                    }
                    case NT_BIN_EXPR: {
                        const BinNode* expr= (BinNode*)node;
                        const IdentNode* left= expr->left.ident;
                        const IdentNode* right= expr->right.ident;
                        const DataFieldNode* data_field= (DataFieldNode*)right->link;
                        const FieldNode* field= FieldNode_vec_get_unsafe(&data_field->data->all_fields, data_field->pos);

                        fprintf(ofile, "data_%s.%s, %hu);\n", left->name, right->name, field->named_info.bits);
                        break;
                    }
                    default: assert(false);
                }
            }

            generate_prefix(depth);
            fprintf(ofile, "bool res= parse_%s(&stream);\n", map->destination->identifier);
            generate_prefix(depth);
            fprintf(ofile, "stream_destroy(&stream);\n");
            generate_prefix(depth);
            fprintf(ofile, "return res == 1 ? PARSE_SUCC(aval_%s) : PARSE_FAIL;", map->destination->identifier);
            // fprintf(ofile, "return aval_%s;", map->destination->identifier);
            break;
        }
        default: assert(false);
    }

    // [[todo]] check this
    // fprintf(ofile, "return (ParseRet){.success=true};");

    return SUCCESS;
}

int generate_string_eval_function(LitStringData* string) {
    for (int i = 0; i < string->expressions.pos; ++i) {
        Node* node= Node_vec_get_unsafe(&string->expressions, i);
        fprintf(ofile,
            "void eval_string_%u_field_%u(Buffer* buffer) {\n",
            string->id,
            i
        );

        switch (node->type) {
            case NT_ALIAS: {
                const AliasNode* alias= (AliasNode*)node;
                fprintf(ofile,
                    "buffer_fconcat(buffer, \"get_aval_%s().chosen_val\");",
                    alias->identifier
                );
                break;
            }
            case NT_DATA: {
                const DataNode* data= (DataNode*)node;
                fprintf(ofile,
                    "buffer_concat(buffer, \"0x\");"
                    "for (int i= 0; i < %lu; i++;) {\n"
                    "   buffer_fconcat(buffer, \"%%x\", data_%s._value[i]);\n"
                    "}\n",
                    (data->bits >> 3) + 1,
                    data->name
                );
                break;
            }
            case NT_BIN_EXPR:
            case NT_UNARY_EXPR: {
                //todo impl
                fprintf(ofile, "assert(false;)");
            }
        }

        fprintf(ofile, "}\n\n");
    }

    return SUCCESS;
}

int generate_string_eval(const LitStringData* string) {
    // turning a string like
    //  "{sibsi} + {disp32}" into "rax + 0xF3000000"
    //  this can mean getting an alias value e.g. sibsi from AVAL_SIBSI.chosen_val
    //  or getting the value of data e.g. disp32 from DATA_DISP32
    //  or getting the value of a data field e.g. REX.w
    fprintf(ofile, "evaluate_string(\"%s\"", string->string);
    for (int i = 0; i < string->expressions.pos; ++i) {
        fprintf(ofile, ", eval_string_%u_field_%u", string->id, i);
    }
    fprintf(ofile, ")");

    return SUCCESS;
}

int generate_l_rule(RuleNodeL* rule, uint8_t depth) {
    generate_prefix(depth);
    generate_left_rules(&rule->rules, depth, true);

    return SUCCESS;
}

int generate_ident_as_aval(IdentNode* ident) {
    switch (ident->link->type) {
        case NT_ALIAS: fprintf(ofile, "get_aval_%s()", link_name(ident->link)); break;
        case NT_DATA: fprintf(ofile, "data_to_aval(data_%s._value)", link_name(ident->link)); break;
        default: assert(false);
    }
    return SUCCESS;
}

int generate_parse_succ(LeftRules* lr) {
    if (lr->pos != 1) goto generate_parse_succ_base_case;

    const LeftRule* first= LeftRule_arr_ptr(lr, 0);
    if (first->base->type != NT_IDENT) goto generate_parse_succ_base_case;

    IdentNode* ident= first->ident;

    switch (ident->link->type) {
        case NT_ALIAS:
            fprintf(ofile, "PARSE_SUCC(get_aval_%s())", link_name(ident->link));
            break;
        case NT_DATA: {
            DataNode* data= (DataNode*)ident->link;
            if (data->non_fielded) {
                fprintf(ofile, "PARSE_SUCC(data_to_aval(data_%s._value))", link_name(ident->link));
                break;
            }
            goto generate_parse_succ_base_case;
        }
    }

    return SUCCESS;

generate_parse_succ_base_case:
    fprintf(ofile,
        "PARSE_SUCC_HIDDEN;"
    );
    return SUCCESS;
}

int generate_left_rules(LeftRules* left_rules, uint8_t depth, bool include_return) {
    // an l rule is made of smaller rules e.g. 11 reg 100 SIB
    fprintf(ofile,
        "if ("
    );

    if (left_rules->pos == 0) fprintf(ofile, "1");

    for (int i = 0; i < left_rules->pos; ++i) {
        const LeftRule* sub_rule= LeftRule_arr_ptr(left_rules, i);

        switch (sub_rule->base->type) {
            case NT_IDENT: {
                const IdentNode* ident= sub_rule->ident;
                const char* name;
                const bool is_inverted= ident->inverted;

                fprintf(ofile,
                    "parse_%s(stream)",
                    link_name(ident->link)
                );

                if (base_link(ident)->type == NT_DATA && is_inverted) {
                    const bool is_var= ident->link->type == NT_VAR;
                    fprintf(ofile, "&& ((%s_%s._value= ~%s_%s._value) || 1)",
                        is_var ? "var" : "data",
                        link_name(ident->link),
                        is_var ? "var" : "data",
                        link_name(ident->link)
                    );
                }

                break;
            }

            case NT_LIT_NUM: {
                const LitNode* lit= sub_rule->lit;
                if (!lit->data.lit_number.show_as_bin) {
                    fprintf(ofile, "EXPECT_BYTE(");
                } else {
                    fprintf(ofile,"EXPECT_BITS(");
                }

                fprint_simple_num(ofile, &lit->data.lit_number);
                fprintf(ofile,", stream)");
                break;
            }
        }
        if (i != left_rules->pos - 1) {
            fprintf(ofile, " && ");
        }
    }
    if (include_return) {
        fprintf(ofile, ") return ");
        generate_parse_succ(left_rules);
        fprintf(ofile, ";");
    }
    else fprintf(ofile, ")");

    return SUCCESS;
}

void generate_prefix(uint8_t depth) {
    fprintf(ofile, "%*s", depth << 2, "");
}

int error(const char* message, ...) {
    va_list args;
    va_start(args, message);
    printf("<<ERROR>> ");
    vprintf(message, args);
    newline();
    va_end(args);

    return FAIL;
}

int info(const char* message, ...) {
    va_list args;
    va_start(args, message);
    printf("|INFO| ");
    vprintf(message, args);
    newline();
    va_end(args);

    return SUCCESS;
}

