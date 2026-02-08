//
// Created by jamestbest on 1/27/26.
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

static int generate_statement(Node* statement);
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
static int generate_ident(const IdentNode* ident);
static int generate_ident_as_value(const IdentNode* ident);
static int generate_operand(OperandNode* node);
static int generate_binary(BinNode* node);
static int generate_unary(UnaryNode* unary);

static void generate_prefix(uint8_t depth);

static int error(const char* string, ...);
static int info(const char* message, ...);

int generate(RootNode* root, FILE* output_file, FILE* header_file, const char* ISA_name) {
    if (!root || !output_file || !header_file) return FAIL;

    ofile= output_file;
    hfile= header_file;

    alias_defer= vector_create();

    generate_header_data();

    fprintf(ofile,
        "#include \"%s.h\"\n\n"
        "int disassemble(){\n}\n\n",
        ISA_name
    );

    for (int i = 0; i < root->child_nodes.pos; ++i) {
        Node* child= vector_get_unsafe(&root->child_nodes, i);

        const int res= generate_statement(child);
        if (res != SUCCESS) return res;
    }

    generate_init_function();

    return SUCCESS;
}

int generate_header_data() {
    fprintf(hfile, "#include \"default.h\"\n\n");
}

int generate_statement(Node* statement) {
    if (!statement) {
        return info("Skipping NULL statement");
    }

    switch (statement->type) {
        case NT_DATA: generate_data_statement((DataNode*)statement); break;
        case NT_ALIAS: generate_alias_statement((AliasNode*)statement); break;
        case NT_FLAG:
        case NT_RULE_RIGHT_STMT:
        case NT_CALCULATE:
            return SUCCESS;
        case NT_STRUCTURE:
            return SUCCESS;
        default: return error("Unexpected statement start node got %s", types_to_string(statement->type));
    }

    return SUCCESS;
}

const char* size_type(uint64_t size) {
    if (size > 32) return "uint64_t";
    if (size > 16) return "uint32_t";
    if (size > 8) return "uint16_t";
    return "uint8_t";
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
        "DATA_%s data_%s;\n",
        cname,
        data->name
    );

    fprintf(hfile,
        "bool parse_%s();\n\n",
        data->name
    );

    fprintf(ofile,
        "DATA_%s parse_%s_() {\n",
        data->capitalised,
        data->name
    );

    if (data->non_fielded) {
        fprintf(ofile,
            "\tuint64_t res= read_bits(&stream, %lu);\n"
            "\treturn (DATA_%s){._value= res, .parsed= true};\n",
            data->bits,
            data->capitalised
        );
    } else {

    }

    fprintf(ofile, "}\n\n");

    fprintf(ofile,
        "bool parse_%s() {\n"
        "\tconst DATA_%s res= parse_%s_();\n"
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

int generate_alias_statement(AliasNode* alias) {
    fprintf(hfile,
        "AVAL aval_%s= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};\n\n",
        alias->identifier
    );

    vector_add(&alias_defer, (void*)alias->identifier);

    fprintf(ofile, "ParseRet parse_%s_() {\n", alias->identifier);
    generate_rules(&alias->rules.rules, 1, alias);
    fprintf(ofile,"\treturn PARSE_FAIL; \n}\n\n");

    fprintf(ofile, "bool parse_%s() {\n", alias->identifier);
    fprintf(ofile, "\tParseRet res= parse_%s_();\n", alias->identifier);
    fprintf(ofile, "\tif (!res.success) return res.success;\n"
                   "\taval_%s= res.aval;\n"
                   "\treturn res.success;\n"
                   "}\n\n",
                   alias->identifier
    );

    return SUCCESS;
}

int generate_rules(RuleArray* rules, uint8_t depth, AliasNode* alias) {
    for (int i = 0; i < rules->pos; ++i) {
        Rule* rule= Rule_arr_ptr(rules, i);

        switch (rule->base.type) {
            case NT_RULE_IF: generate_if_rule(&rule->data.rule_if, depth, alias); break;
            case NT_RULE_WHEN: generate_when_rule(&rule->data.rule_when, depth, alias); break;
            case NT_RULE_L: generate_l_rule(&rule->data.rule_l, depth); break;
            case NT_RULE_LR: generate_lr_rule(&rule->data.rule_lr, depth, alias); break;
            default: assert(false);
        }
        fnewline(ofile);
    }

    return SUCCESS;
}

int generate_if_rule(RuleNodeIf* node, uint8_t depth, AliasNode* alias) {
    fprintf(ofile, "if (");
    generate_expression(node->condition);
    fprintf(ofile, ") {\n");
    generate_right_rules(&node->output, depth + 1, alias);
    fnewline(ofile);
    generate_prefix(depth);
    fprintf(ofile, "}\n");

    return SUCCESS;
}

int generate_when_rule(RuleNodeWhen* node, uint8_t depth, AliasNode* alias) {
    fprintf(ofile, "if (");
    generate_expression(node->condition);
    fprintf(ofile, ") {\n");
    generate_right_rules(&node->output, depth + 1, alias);
    fprintf(ofile, "}\n");

    return SUCCESS;
}

int generate_expression(Node* expr) {
    switch (expr->type) {
        case NT_IDENT: {
            generate_ident((IdentNode*)expr);
            break;
        }
        case NT_LIT_NUM: {
            break;
        }
        case NT_LIT_STRING: {
            break;
        }
        case NT_BIN_EXPR: {
            generate_binary((BinNode*)expr);
            break;
        }
        case NT_UNARY_EXPR: {
            generate_unary((UnaryNode*)expr);
            break;
        }
    }

    return SUCCESS;
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
        "flag_%s %c= FLAG_%s_VALUE_%s",
        canonical_flag->name,
        is_neq ? '!' : '=',
        canonical_flag->name,
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
        default: assert(false);
    }

    return SUCCESS;
}

int generate_ident_as_value(const IdentNode* ident) {
    generate_ident(ident);
    switch (ident->link->type) {
        case NT_ALIAS: assert(false);
        case NT_DATA: fprintf(ofile, "._value"); break;
    }
    return SUCCESS;
}

int generate_ident(const IdentNode* ident) {
    switch (ident->link->type) {
        case NT_ALIAS: {
            const AliasNode* alias= (AliasNode*)ident->link;
            fprintf(ofile, "aval_%s", alias->identifier);
            break;
        }
        case NT_DATA: {
            const DataNode* data= (DataNode*)ident->link;
            fprintf(ofile, "data_%s", data->name);
            break;
        }
        case NT_FLAG: {
            const FlagNode* flag= (FlagNode*)ident->link;
            fprintf(ofile, "flag_%s", flag->name);
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
            fprintf(ofile,
                "return PARSE_SUCC("
            );

            IdentNode* ident= (IdentNode*)node;
            switch (ident->link->type) {
                case NT_ALIAS: {
                    const AliasNode* r_alias= (AliasNode*)ident->link;
                    fprintf(ofile,
                        "aval_%s",
                        r_alias->identifier
                    );
                    break;
                }
                case NT_DATA: {
                    const DataNode* data= (DataNode*)ident->link;
                    fprintf(ofile,
                        "data_to_aval(data_%s)",
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
}

int generate_right_rules(RightRule* rule, uint8_t depth, AliasNode* alias) {
    generate_prefix(depth);
    switch (rule->base->type) {
        case NT_MULTI: {
            MultiNode* multi= (MultiNode*)rule->base;
            if (multi->multis.pos == 1) return generate_multi(Node_vec_get_unsafe(&multi->multis, 0), alias);

            if (!alias->linked_rule) return error("Unable to find linked rule for alias with multiple outputs");
            fprintf(ofile,
                "switch(calculate_rule_right_%u()) {\n", alias->linked_rule->id);

            for (uint i = 0; i < multi->multis.pos; ++i) {
                Node* node= Node_vec_get_unsafe(&multi->multis, i);
                generate_prefix(depth + 1);
                fprintf(ofile, "case %u: ", i);
                generate_multi(node, alias);
                fprintf(ofile, "; break;\n");
            }
            generate_prefix(depth);
            fprintf(ofile, "}\n");
            break;
        }
        case NT_BRACED_RULES: {
            BracedRules* rules= rule->brace;
            generate_rules(&rules->rules, depth + 1, alias);

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
                    "buffer_fconcat(buffer, \"aval_%s.chosen_val\");",
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

int generate_string_eval_functions(Vector string_data) {
    // these are the field eval functions
    //  e.g. eval_string_xy_field_ab
    for (int i = 0; i < string_data.pos; ++i) {
        LitStringData* data= vector_get_unsafe(&string_data, i);

        const int res= generate_string_eval_function(data);
        if (res != SUCCESS) return res;
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

int generate_left_rules(LeftRules* left_rules, uint8_t depth, bool include_return) {
    // an l rule is made of smaller rules e.g. 11 reg 100 SIB
    fprintf(ofile,
        "if ("
    );

    for (int i = 0; i < left_rules->pos; ++i) {
        const LeftRule* sub_rule= LeftRule_arr_ptr(left_rules, i);

        switch (sub_rule->base->type) {
            case NT_IDENT: {
                const IdentNode* ident= sub_rule->ident;
                const char* name;

                switch (ident->link->type) {
                    case NT_ALIAS: {
                        const AliasNode* alias= (AliasNode*)ident->link;
                        name= alias->identifier;
                        break;
                    }
                    case NT_DATA: {
                        const DataNode* data= (DataNode*)ident->link;
                        name= data->name;
                        break;
                    }
                    default: assert(false);
                }

                fprintf(ofile,
                    "parse_%s()",
                    name
                );
                break;
            }

            case NT_LIT_NUM: {
                const LitNode* lit= sub_rule->lit;
                fprintf(ofile,
                    "EXPECT_BITS("
                );
                fprint_simple_num(ofile, &lit->data.lit_number);
                fprintf(ofile,")");
                break;
            }
        }
        if (i != left_rules->pos - 1) {
            fprintf(ofile, " && ");
        }
    }
    if (include_return)
        fprintf(ofile,
            ") return PARSE_SUCC_HIDDEN;"
        );
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

