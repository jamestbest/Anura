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

static int error(const char* message, ...);
static const char* types_to_string(NodeType type);

int generate(RootNode* root, FILE* output_file, FILE* header_file) {
    if (!root || !output_file || !header_file) return FAIL;

    ofile= output_file;
    hfile= header_file;

    alias_defer= vector_create();

    generate_header_data();

    for (int i = 0; i < root->child_nodes.pos; ++i) {
        Node* child= vector_get_unsafe(&root->child_nodes, i);

        const int res= generate_statement(child);
        if (res != SUCCESS) return res;
    }

    generate_init_function();

    return SUCCESS;
}

int generate_header_data() {

}

int generate_statement(Node* statement) {
    switch (statement->type) {
        case NT_DATA: generate_data_statement((DataNode*)statement); break;
        case NT_ALIAS: generate_alias_statement((AliasNode*)statement); break;
        default: return error("Unexpected statement start node got %s", types_to_string(statement->type));
    }

    return SUCCESS;
}

int generate_data_statement(DataNode* data) {
    const char* cname= capitalised(data->name);
    fprintf(hfile,
        "typedef struct DATA_%s {\n",
        cname
    );

    for (int i = 0; i < data->all_fields.pos; ++i) {
        FieldNode* field= FieldNode_vec_get_unsafe(&data->all_fields, i);

        if (!field->named) {
            fprintf(hfile,"\t// IMM: ");
            fprint_simple_num(hfile, &field->num);
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

    return SUCCESS;
}

int generate_init_function() {
    fprintf(ofile,
        "int init() {\n"
    );
    for (int i = 0; i < alias_defer.pos; ++i) {
        fprintf(ofile,
            "   vector_create(&AVAL_%s.choices);\n"
        );
    }
    fprintf(ofile,
        "return 0;\n"
        "}\n\n"
    );

    return SUCCESS;
}

int generate_alias_statement(AliasNode* alias) {
    const char* cname= capitalised(alias->identifier);
    fprintf(hfile,
        "AVAL AVAL_%s= (AVAL){.choices={0}, .chosen_val= NULL, .chosen_idx= (uint8_t)-1};\n\n",
        cname
    );

    vector_add(&alias_defer, (void*)cname);

    generate_rules(&alias->rules.rules, 1, alias);

    return SUCCESS;
}

int generate_rules(RuleArray* rules, uint8_t depth, AliasNode* alias) {
    for (int i = 0; i < rules->pos; ++i) {
        Rule* rule= Rule_arr_ptr(rules, i);

        switch (rule->base.type) {
            case NT_RULE_IF: generate_if_rule((RuleNodeIf*)rule, depth, alias); break;
            case NT_RULE_WHEN: generate_when_rule((RuleNodeWhen*)rule, depth, alias); break;
            case NT_RULE_L: generate_l_rule((RuleNodeL*)rule, depth); break;
            case NT_RULE_LR: generate_lr_rule((RuleNodeLR*)rule, depth, alias); break;
            default: assert(false);
        }
    }

    return SUCCESS;
}

int generate_if_rule(RuleNodeIf* node, uint8_t depth, AliasNode* alias) {
    fprintf(ofile, "if (");
    generate_expression(node->condition);
    fprintf(ofile, ") {\n");
    generate_right_rules(&node->output, depth + 1, alias);
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

const char* link_name(Node* link) {
    switch (link->type) {
        case NT_FLAG: return ((FlagNode*)link)->name;
        case NT_ALIAS: return ((AliasNode*)link)->identifier;
        case NT_DATA: return ((DataNode*)link)->name;
        default: assert(false);
    }
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

int generate_binary(BinNode* node) {
    switch (node->op) {
        // FLAG == FLAG_ENUM e.g. mode == 64bit => flag_mode == FLAG_MODE_64bit
        // CANNOT DO ALIAS == x e.g. reg == 010  Aliases don't have exposed values... maybe one day
        //          e.g. could if the thing succeeds copy into the alias before the return using the values of
        //           the lower aliases and data and literals, but not needed for now
        // DATA == LIT e.g. Mod == 01 => data_Mod._value == 0b01
        case EQUALITY:
        case NEQUALITY:
        case STAR:
            generate_operand(&node->left);
            fprintf(ofile, " %s ", bin_op_to_symbol(node->op));
            generate_operand(&node->right);
            break;
        case DOT: {
            const DataNode* data= (DataNode*)node->left.ident->link;
            const IdentNode* ident= node->right.ident;

            fprintf(ofile, "data_%s.%s", data->name, ident->token->data.identifier);
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
    switch (((Node*)node)->type) {
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
        default: assert(false);
    }

    return SUCCESS;
}

int generate_lr_rule(RuleNodeLR* rule, uint8_t depth, AliasNode* alias) {
    generate_prefix(depth);
    generate_left_rules(&rule->left, depth, false);

    fprintf(ofile, "{");
    generate_right_rules(&rule->right, depth + 1, alias);
    fprintf(ofile, "%*s}", depth, "");

    return SUCCESS;
}

int generate_right_rules(RightRule* rule, uint8_t depth, AliasNode* alias) {
    generate_prefix(depth);
    switch (rule->base->type) {
        case NT_IDENT: {
            fprintf(ofile,
                "set_aval(&aval_%s, ",
                alias->capitalised
            );

            switch (rule->ident->link->type) {
                case NT_ALIAS: {
                    const AliasNode* r_alias= (AliasNode*)rule->ident->link;
                    fprintf(ofile,
                        "aval_%s",
                        r_alias->capitalised
                    );
                    break;
                }
                case NT_DATA: {
                    const DataNode* data= (DataNode*)rule->ident->link;
                    fprintf(ofile,
                        "data_to_aval(data_%s)",
                        data->capitalised
                    );
                    break;
                }
                default: assert(false);
            }

            fprintf(ofile, ");");
            break;
        }

        case NT_LIT_STRING: {
            const LitNode* lit= rule->lit;
            fprintf(ofile,
                "set_aval(&aval_%s, ",
                alias->capitalised
            );
            generate_string_eval(&lit->data.lit_string);
            fprintf(ofile, "); // ");
            fputz_sanitize(ofile, lit->data.lit_string.string);
            fnewline(ofile);
            break;
        }
        case NT_BRACED_RULES: {
            BracedRules* rules= rule->brace;
            generate_rules(&rules->rules, depth + 1, alias);

            break;
        }
    }

    // [[todo]] check this
    fprintf(ofile, "return (ParseRet){.success=true};");

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
                    "buffer_fconcat(buffer, \"AVAL_%s.chosen_val\");",
                    alias->capitalised
                );
                break;
            }
            case NT_DATA: {
                const DataNode* data= (DataNode*)node;
                fprintf(ofile,
                    "buffer_concat(buffer, \"0x\");"
                    "for (int i= 0; i < %lu; i++;) {\n"
                    "   buffer_fconcat(buffer, \"%%x\", DATA_%s._value[i]);\n"
                    "}\n",
                    (data->bits >> 3) + 1,
                    data->capitalised
                );
                break;
            }
            case NT_EXPR: {
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
    fprintf(ofile, ");\n");

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
                    "parse_%s().success",
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
            ") return (ParseRet) {.output_string="", .success=true};"
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

const char* types_to_string(NodeType type) {
    switch (type) {
        case NT_ROOT: return "NT_ROOT";
        case NT_ALIAS: return "NT_ALIAS";
        case NT_DATA: return "NT_DATA";
        case NT_STRUCTURE: return "NT_STRUCTURE_STMT";
        case NT_STATEMENTS: return "NT_STATEMENTS";
        case NT_FLAG: return "NT_FLAG_STATEMENT";
        case NT_BRACED_RULES: return "NT_BRACED_RULES";
        case NT_RULE_IF: return "NT_RULE_IF";
        case NT_RULE_WHEN: return "NT_RULE_WHEN";
        case NT_RULE_L: return "NT_RULE_L";
        case NT_RULE_LR: return "NT_RULE_LR";
        case NT_IDENT: return "NT_IDENT";
        case NT_FIELD: return "NT_FIELD";
        case NT_LIT_STRING: return "NT_LIT_STRING";
        case NT_LIT_NUM: return "NT_LIT_NUM";
        default: return "<<ERROR>> unknown node type <<ERROR>>";
    }
}
