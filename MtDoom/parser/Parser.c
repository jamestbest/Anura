//
// Created by James Coward on 11/24/25.
//

#include "Parser.h"

#include "Buffer.h"
#include "Errors.h"

#include <stdio.h>
#include <string.h>

#include "Colours.h"
#include "ShuntingYard.h"

const ParseRet PARSE_RET_FAIL= (ParseRet) {.succ= false, .node= NULL};
const ParseRet PARSE_RET_SUCC= (ParseRet) {.succ= true, .node= NULL};

static RootNode create_root();
static Vector create_children();

static Token* consume();
static Token* current();
static Token* expect(TokenType type);
static Token* expect_keyword(keyword kw);
static Token* expect_binary_op(BinaryOperator op);
static Token* expect_unary_op(UnaryOperator op);
static Token* check(TokenType type) ;
static void skip(TokenType type);
static Token* peek();
static Token* peer(uint distance);

static ParseRet parse_toplevel();

static ParseRet error(const char* message, ...);
static ParseRet error_with_token(const char* message, Token* tok, ...);
static ParseRet unexpected(const char* context, Token* unexpected_token);
static ParseRet funexpected(const char* context, Token* unexpected_token, ...);
static ParseRet unexpected_keyword(const char* context, keyword kw);

static ParseRet parse_alias();
static ParseRet parse_calc();
static ParseRet parse_data();
static ParseRet parse_var();
static ParseRet parse_flag();
static ParseRet parse_structure();
static ParseRet parse_meta();
static ParseRet parse_expr();
static ParseRet parse_rule_right_stmt();
static ParseRet parse_braced_rules(BracedRules* results, AliasNode* alias);
static ParseRet parse_braced_if_flags(IfBracedRules* rules, FlagNode* flag);
static ParseRet parse_l_or_lr_rule(RuleArray* rules, AliasNode* alias);
static ParseRet parse_size(uint32_t* res);
static ParseRet parse_with(RuleArray* rules, AliasNode* alias);

static AliasNode* add_alias_node(const char* name);
static FlagNode* add_flag_node(const char* name);
static DataNode* add_data_node(const char* name);
static FieldNode* add_field_node();
static StructureNode* add_structure();
static Rule* add_rule(NodeType type);
static RuleNodeIf* add_rule_node_if(Node* expr, RightRule output);
static BracedRules* add_braced_rules();
static RuleNodeLR* add_rule_lr(LeftRules left, RightRule right);
static RuleNodeL* add_rule_l(LeftRules left);
static IfBracedRules* add_if_braced_rules();
static FlagValueNode* add_flag_value_node();
static MultiNode* add_multi_node();
static RuleRightNode* add_rule_right_node();
static MetaNode* add_meta_node();
static MapNode* add_map_node();
static VarNode* add_var_node(const char* identifier, Node* link);
static AssignNode* add_assign_node();
static RuleNodeWith* add_with_node(AliasNode* alias);

static CalcNode create_calc_node(const char* identifier);
static CalcNode* add_calc_node(const char* identifier);
static Rule create_rule(NodeType type);
static RuleNodeIf create_rule_node_if(Node* expr, RightRule output);
static IdentNode create_ident_node(const char* name, Node* link);
static BracedRules create_braced_rules();
static RuleNodeLR create_rule_lr(LeftRules left, RightRule right);
static RuleNodeL create_rule_l(LeftRules left);
static IfBracedRules create_if_braced_rules();
static FlagValueNode create_flag_value_node();
static MultiNode create_multi_node();
static RuleRightNode create_rule_right_node();
static DataFieldNode create_data_field_node(DataNode* data, size_t pos);
static MetaNode create_meta_node();
static ExprNode create_expr_node();
static MapNode create_map_node();
static VarNode create_var_node(const char* identifier, Node* link);
static AssignNode create_assign_node();
static RuleNodeWith create_with_node(AliasNode* alias);

static void print_symbol_table();

static ParseRet error(const char* message, ...);

uint64_t convert_lit_num_to_base10(struct LitNumData lit_num, bool expecting_base10);

VECTOR_ADD(IdentNode, IdentNode)
ARRAY_ADD(LeftRule, LeftRule)
VECTOR_ADD(FieldNode, FieldNode)
ARRAY_ADD(MarkedIdent, MarkedIdent)
VECTOR_ADD(Node, Node)
ARRAY_ADD(Rule, Rule)
ARRAY_ADD(IfFlagRule, IfFlagRule)
ARRAY_ADD(FieldNodeVector, FieldNodeVector)
ARRAY_ADD(FlagLinkPos, FlagLinkPos)

TokenArray* tokens;
size_t t_i;

size_t string_id= 0;

size_t with_id= 0;
size_t next_with_id() {return with_id++;}

size_t rule_right_stmt_id= 0;

uint8_t* node_buffer;
Vector node_buffers;
size_t node_buffer_capacity;
size_t node_buffer_size;

#define MIN_NODE_BUFFER_SIZE 10 // [[todo]]: change to ~500 after testing

typedef struct Symbol {
    const char* name;
    Node* node;
} Symbol;

ARRAY_PROTO_CMP(Symbol, Symbol, strcmp, name)
ARRAY_ADD_CMP(Symbol, Symbol, strcmp, name)

SymbolArray symbol_table;

RootNode root;

bool should_add_to_root(NodeType type) {
    return type != NT_STRUCTURE && type != NT_META;
}

Parsed parse(TokenArray* ts) {
    root= create_root();
    int ret_code= true;

    tokens= ts;
    t_i= 0;

    string_id= 0;
    with_id= 0;
    rule_right_stmt_id= 0;

    symbol_table= Symbol_arr_create();

    node_buffer= malloc(1000 * sizeof(uint8_t));
    node_buffer_capacity= 1000;
    node_buffer_size= 0;

    while (t_i < tokens->pos - 1) {
        ParseRet res;
        if (res= parse_toplevel(), res.succ) {
            if (res.node && should_add_to_root(res.node->type))
                vector_add(&root.child_nodes, res.node);
        } else {
            ret_code= res.succ;
            break;
        }
    }

    if (!root.meta->meta.parsed || !root.structure) {
        error("Meta data and structure statements are required");
        ret_code= false;
    }

    if (!ret_code) {
        free(node_buffer);
        return (Parsed){.succ=false, .root= {0}, .node_buffer= NULL};
    }

    print_symbol_table();

    return (Parsed) {
        .succ= true,
        .root= root,
        .node_buffer= node_buffer
    };
}

void eat_all_whitespace() {
    while (current() && current()->type == DELIMITER) consume();
}

ParseRet parse_toplevel() {
    eat_all_whitespace();

    Token* t= current();

    if (!t) return PARSE_RET_SUCC;

    if (t->type != KEYWORD) {
        return error_with_token("Expected keyword at toplevel", t);
    }

    switch (t->data.keyword) {
        case FLAT:
        case ALIAS: return parse_alias();
        case CALCULATE: return parse_calc();
        case DATA: return parse_data();
        case FLAG: return parse_flag();
        case STRUCTURE: return parse_structure();
        case RULE: return parse_rule_right_stmt();
        case META: return parse_meta();
        case VAR: return parse_var();
        default:
            return error_with_token("Expected keyword token to be in (ALIAS, CALCULATE, DATA, FLAG, STRUCTURE)",t);
    }
}

bool flag_links_has_flag(const FlagValueNode* fv, const FlagNode* flag) {
    bool found= false;
    for (int i = 0; i < fv->links.pos; ++i) {
        const FlagLinkPos* link= FlagLinkPos_arr_ptr(&fv->links, i);
        if (link->flag == flag) found= true;
    }
    return found;
}

Node* base_link(const IdentNode* node) {
    if (node->link->type == NT_VAR) {
        const VarNode* var= (VarNode*)node->link;
        return var->link;
    }
    return node->link;
}

// the left link is the base type that it points to
//  e.g. NT_ALIAS, NT_DATA, NT_DATA_FIELD
bool check_assignment(Node* left_link, Node* right_expr) {
    switch (left_link->type) {
        case NT_ALIAS: {
            // if it's an alias then it can only be = STRING or ALIAS IDENT or ALIAS VAR
            if (right_expr->type == NT_LIT_STRING) return true;
            if (right_expr->type == NT_IDENT) {
                const IdentNode* ident= (IdentNode*)right_expr;
                Node* base= base_link(ident);
                return base->type == NT_ALIAS;
            }
            return false;
        }
        case NT_DATA: {
            // when data it can only be assigned to a BIN or UN expr or LIT_NUM or DATA IDENT or DATA VAR
            if (right_expr->type == NT_BIN_EXPR || right_expr->type == NT_UNARY_EXPR) return true;
            if (right_expr->type == NT_LIT_NUM) return true;
            if (right_expr->type == NT_IDENT) {
                const IdentNode* ident= (IdentNode*)right_expr;
                Node* base= base_link(ident);
                return base->type == NT_DATA;
            }
            return false;
        }
        case NT_FLAG: {
            // this can only be a flag literal, or flag var of the same
            const FlagNode* flag= (FlagNode*)left_link;
            if (right_expr->type == NT_IDENT) {
                const IdentNode* ident= (IdentNode*)right_expr;
                const Node* base= base_link(ident);

                if (base->type == NT_FLAG_VALUE) {
                    const FlagValueNode* fv= (FlagValueNode*)base;
                    return flag_links_has_flag(fv, flag);
                }
                if (base->type == NT_FLAG) {
                    const FlagNode* r_flag= (FlagNode*)base;
                    return flag == r_flag;
                }
            }
            return false;
        }
        default: assert(false);
    }
}

ParseRet parse_var() {
    consume(); // eat `VAR`
    const Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Var statement, expected identifier after `VAR`", current());
    const Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident->data.identifier);
    if (sym) return error("Symbol with name `%s` already exits");

    if (!expect_keyword(OF)) return unexpected("Var statement after identifier, expected `OF`", current());

    const Token* link= expect(IDENTIFIER);
    if (!link) return unexpected("Var statement after `OF`, expected link name", current());
    sym= Symbol_arr_search_ie(&symbol_table, link->data.identifier);
    if (!sym) return error("Symbol with name `%s` does not exist in VAR statement", link->data.identifier);

    VarNode* var_node= add_var_node(ident->data.identifier, sym->node);

    add_to_symbol_table(ident->data.identifier, (Node*)var_node);

    if (expect(ASSIGN)) {
        const ParseRet default_value= parse_expr();
        if (!default_value.succ) return default_value;

        var_node->value= (ExprNode*)default_value.node;
    }

    Node* expr= ((ExprNode*)var_node->value)->expr;
    if (!check_assignment(sym->node, expr)) {
        return error("Cannot verify the assignment types match in VAR statement `%s`", var_node->identifier);
    }

    return (ParseRet) {
        .succ= true,
        .node= (Node*)var_node
    };
}

const char* type_to_str(const TYPE type) {
    switch (type) {
        case TYPE_NUMBER: return "NUMBER";
        case TYPE_STRING: return "STRING";
        case TYPE_ENUM: return "ENUM";
        case TYPE_ALIAS: return "ALIAS";
        case TYPE_ERROR: return "ERROR";
        default: return "<<ERROR>> Unknown TYPE <<ERROR>>";
    }
}

ParseRet parse_meta_row(MetaData* md) {
    if (!expect_binary_op(DOT)) return unexpected("Meta statement row starter, expected `.`", current());

    const Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Meta statement row after `.`, expected identifier for meta data field", current());

    if (strcmp(ident->data.identifier, "name") == 0) {
        if (!expect(ASSIGN)) return unexpected("Name field of meta data statement, expected `=`", current());

        const Token* name_str= expect(LIT_STRING);
        if (!name_str) return unexpected("Name field of meta data statement after `=`, expected string", current());

        root.meta->meta.name= name_str->data.lit_string;

        return PARSE_RET_SUCC;
    }

    if (strcmp(ident->data.identifier, "endianness") == 0) {
        if (!expect(ASSIGN)) return unexpected("Endianness field of meta data statement, expected `=`", current());

        const Token* endian_tok= expect(LIT_STRING);
        if (!endian_tok) return unexpected("Endianness field of meta data statement after `=`, expected string (LITTLE, BIG)", current());

        if (strcmp(endian_tok->data.identifier, "LITTLE") == 0) {
            root.meta->meta.endianness= ENDIAN_LITTLE;
        } else if (strcmp(endian_tok->data.identifier, "BIG") == 0) {
            root.meta->meta.endianness= ENDIAN_BIG;
        } else {
            return unexpected("Endianness field of meta data statement, expected LITTLE or BIG", current());
        }

        return PARSE_RET_SUCC;
    }

    return PARSE_RET_FAIL;
}

ParseRet parse_meta() {
    consume(); // eat `META`

    root.meta= add_meta_node();
    MetaData* meta= &root.meta->meta;
    if (meta->parsed) return error("Meta data statement already exists in file");

    if (!expect(ASSIGN)) return unexpected("After meta keyword, expected `=`", current());
    if (!expect(LBRACE)) return unexpected("After `=` in meta statement, expected `{`", current());

    eat_all_whitespace();
    do {
        if (!parse_meta_row(meta).succ) return PARSE_RET_FAIL;
        eat_all_whitespace();
    } while (!expect(RBRACE));

    meta->parsed= true;
    return (ParseRet) {
        .succ= true,
        .node= NULL
    };
}

ParseRet parse_marked_ident(MarkedIdentArray* idents) {
    const Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Structure statement, expected identifier for rule", current());

    Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident->data.identifier);
    if (!sym) return error("Unable to find identifier %s in scope in structure statement", ident->data.identifier);

    MarkedIdent mi= (MarkedIdent) {
        .ident= sym->node,
        .type= MARKER_NONE
    };

    if (expect_binary_op(STAR)) {
        mi.type= MARKED_STAR;
    } else if (expect_unary_op(EXISTS)) {
        mi.type= MARKED_QUESTION;
    }

    MarkedIdent_arr_add(idents, mi);

    return PARSE_RET_SUCC;
}

ParseRet parse_structure() {
    consume(); // eat `STRUCTURE`

    if (root.structure) return error("Structure of ISA is already defined in file");

    root.structure= add_structure();
    do {
        const ParseRet res= parse_marked_ident(&root.structure->rules);
        if (!res.succ) return res;
    } while (!expect(ASSIGN));

    const Token* output= consume();
    switch (output->type) {
        case IDENTIFIER: {
            Symbol* sym= Symbol_arr_search_ie(&symbol_table, output->data.identifier);
            if (!sym) return error("Cannot find symbol `%s` in output of structure", output->data.identifier);
            if (sym->node->type != NT_ALIAS) return error("Output of structure statement can only be an alias or string");
            const IdentNode* ident_out= add_ident_node(output->data.identifier, sym->node);

            root.structure->output.single_out= (Node*)ident_out;
            break;
        }
        case LIT_STRING: {
            const LitNode* lit_out= add_lit_string_node(output->data.lit_string);
            root.structure->output.single_out= (Node*)lit_out;
            break;
        }
        default: return error("Output of structure can only be lit string or identifier");
    }

    return (ParseRet){
        .succ= true,
        .node= (Node*)root.structure
    };
}

ParseRet parse_rule_right_row(uint64_t expected_idx) {
    if (!expect_keyword(CHOOSE)) return unexpected("Right rule row, expects `CHOOSE` at start", current());

    const Token* num= expect(LIT_NUM);
    if (!num) return unexpected("Right rule row after `CHOOSE`, expects literal number", current());

    const uint64_t val= convert_lit_num_to_base10(num->data.lit_num, true);
    if (val != expected_idx) {
        return error("Indexes in RULE RIGHT statements must be in order, from 0..n. Expected %lu got %lu",
            expected_idx,
            val
        );
    }

    if (!expect_keyword(IF)) return unexpected("Right rule row after index, expects `IF`", current());
    return parse_expr();
}

ParseRet parse_rule_right_stmt() {
    consume(); // eat `RULE`
    if (!expect_keyword(RIGHT)) return unexpected("Rule statement after rule keyword, expected `RIGHT` keyword", current());
    if (!expect_keyword(ON)) return unexpected("Rule statement after `RULE RIGHT`, expected ON", current());

    Vector alias_links= vector_create();

    do {
        const Token* ident= expect(IDENTIFIER);
        if (!ident) return unexpected("Rule statement alias list, expects identifier after `ON` or `,`", current());

        Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident->data.identifier);
        if (!sym) return error("Unable to find alias `%s` in scope in rule statement", ident->data.identifier);
        if (sym->node->type != NT_ALIAS) return error("Identifier `%s` in rule statement is not an alias", ident->data.identifier);

        vector_add(&alias_links, sym->node);
    } while (expect(COMMA));

    if (!expect(LBRACE)) return unexpected("Rule statement after alias list, expects `{`", current());
    eat_all_whitespace();

    RuleRightNode* rule_right= add_rule_right_node();

    uint i= 0;
    do {
        const ParseRet res= parse_rule_right_row(i++);
        if (!res.succ) return res;
        Node_vec_add(&rule_right->expressions, res.node);
        eat_all_whitespace();
    } while (!expect(RBRACE));

    for (i = 0; i < alias_links.pos; ++i) {
        AliasNode* alias= vector_get_unsafe(&alias_links, i);
        alias->linked_rule= rule_right;
    }

    return (ParseRet) {
        .succ= true,
        .node= (Node*)rule_right
    };
}

size_t has_field_data(FieldNodeVector* fields, const char* field_name) {
    for (int i = 0; i < fields->pos; ++i) {
        const FieldNode* field= FieldNode_vec_get_unsafe(fields, i);

        if (!field->named) continue;
        if (strcmp(field->named_info.name, field_name) == 0) {
            return i;
        }
    }
    return -1;
}

bool simple_nums_are_equal(SimpleNumData* a, SimpleNumData* b) {
    return a->value == b->value;
}

bool named_fields_are_same(FieldNode* a, FieldNode* b) {
    if (strcmp(a->named_info.name, b->named_info.name) != 0) return false;
    if (a->named_info.bits != b->named_info.bits) return false;

    if (a->named_info.has_default ^ b->named_info.has_default) return false;
    if (a->named_info.has_default) {
        return simple_nums_are_equal(&a->named_info.default_value, &b->named_info.default_value);
    }

    return true;
}

bool fields_are_same(FieldNode* a, FieldNode* b) {
    if (a->named ^ b->named) return false;

    if (a->named) return named_fields_are_same(a,b);
    return simple_nums_are_equal(&a->num, &b->num);
}

ParseRet parse_data_field(FieldNodeVector* all_fields, FieldNodeVector* fields, const char* data_name, size_t first_row_pos) {
    FieldNode* field= add_field_node();

    const Token* num= expect(LIT_NUM);
    if (num) {
        field->named= false;
        field->num= complex_to_simple_num(num->data.lit_num, true);

        FieldNode_vec_add(fields, field);
        return PARSE_RET_SUCC;
    }

    const Token* dot= expect_binary_op(DOT);
    if (!dot) return unexpected("DATA statement row, expected either literal number or field", current());

    const Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("DATA statement row, expected identifier after dot e.g. `.v`", current());

    uint8_t bits= 1;
    if (expect(LPAREN)) {
        const Token* bit_tok= expect(LIT_NUM);

        if (!bit_tok) return unexpected("DATA field bit size, expected literal number for size after `(`", current());

        bits= convert_lit_num_to_base10(bit_tok->data.lit_num, true);

        if (!expect(RPAREN)) return unexpected("DATA field bit size, expected closing paren `)`", current());
    }

    field->named= true;
    field->named_info= (struct NamedFieldInfo){
        .name= ident->data.identifier,
        .bits= bits,
    };

    if (expect(ASSIGN)) {
        const Token* dvalue= expect(LIT_NUM);

        if (!dvalue) return funexpected("DATA field `%s` default value expects literal number after `=`", current(), ident->data.identifier);

        field->named_info.has_default= true;
        field->named_info.default_value= complex_to_simple_num(dvalue->data.lit_num, true);
    }

    const size_t pos= has_field_data(all_fields, ident->data.identifier);
    if (pos != -1) {
        if (pos < first_row_pos) {
            // this is allowed, but must be the same number of bits and have the same default value
            FieldNode* efield= FieldNode_vec_get_unsafe(all_fields, pos);

            if (!fields_are_same(efield, field)) return error(
                "Fields `%s` in DATA statement `%s` are not equal across rows, this is a requirement for multirow data, consider using seperate DATA statements for ones where the fields should be of different dimensions or default values",
                ident->data.identifier,
                data_name
            );
        } else {
            return error("Field `%s` already exists in same row", ident->data.identifier);
        }
    }

    FieldNode_vec_add(fields, field);
    if (pos == -1)
        FieldNode_vec_add(all_fields, field);

    return PARSE_RET_SUCC;
}

ParseRet parse_data_row(FieldNodeVector* all_fields, FieldNodeVectorArray* rows, const char* data_name) {
    //   1100 0100 .R .X .B .m(5) .W .v(4) .L .pp(2)
    const size_t first_row_pos= all_fields->pos;
    FieldNodeVector row= FieldNode_vec_create();

    while (!expect(DELIMITER)) {
        const ParseRet res= parse_data_field(all_fields, &row, data_name, first_row_pos);

        if (!res.succ) return res;
    }

    FieldNodeVector_arr_add(rows, row);

    return PARSE_RET_SUCC;
}

ParseRet parse_data() {
    consume(); // eat `DATA`

    Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Expected identifier after DATA keyword", current());

    uint32_t size_in_bits= 0;
    ParseRet size_res= parse_size(&size_in_bits);
    if (!size_res.succ) return size_res;

    DataNode* data= add_data_node(ident->data.identifier);
    data->bits= size_in_bits;

    if (!expect(ASSIGN)) {
        data->non_fielded= true;

        if (size_in_bits == 0) return error("Cannot create non-fielded data without explicit size");
        goto parse_data_res;
    }

    if (!expect(LBRACE)) return unexpected("Expected `{` after `=` in DATA statement", current());

    expect(DELIMITER);

    while (!expect(RBRACE)) {
        ParseRet res= parse_data_row(&data->all_fields, &data->rows, data->name);
        if (!res.succ) return res;

        expect(DELIMITER);
    }

parse_data_res:
    add_to_symbol_table(data->name, (Node*)data);
    return (ParseRet) {
        .succ= true,
        .node= (Node*)data
    };
}

size_t find_enum_value(Vector* enum_values, const char* enum_name) {
    for (int i = 0; i < enum_values->pos; ++i) {
        const char* cmp= vector_get_unsafe(enum_values, i);
        if (strcmp(cmp, enum_name) == 0) return i;
    }
    return -1;
}

ParseRet get_or_add_enum(const char* ident, FlagLinkPos link) {
    Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident);
    if (!sym) {
        FlagValueNode* fv= add_flag_value_node();
        FlagLinkPos_arr_add(&fv->links, link);
        fv->name= ident;
        Symbol_arr_add(&symbol_table, (Symbol) {
            .name= ident,
            .node= (Node*)fv
        });

        return PARSE_RET_SUCC;
    }

    if (sym->node->type != NT_FLAG_VALUE) return error("Unable to add enum value `%s` when non enum identifier with same name exists");
    FlagValueNode* fv= (FlagValueNode*)sym->node;
    FlagLinkPos_arr_add(&fv->links, link);
    return PARSE_RET_SUCC;
}

ParseRet parse_flag() {
    consume(); // eat `FLAG`

    const Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Flag statement, expected identifier after keyword", current());

    if (!expect(ASSIGN)) return unexpected("Flag statement after identifier, expected `=`", current());

    FlagNode* flag= add_flag_node(ident->data.identifier);

    do {
        const Token* enum_ident= expect(IDENTIFIER);
        if (!enum_ident) return unexpected("Flag enum list, expected identifier at start or after `|`", current());

        vector_add(&flag->enum_values, (void*)enum_ident->data.identifier);

        const FlagLinkPos link= (FlagLinkPos){
            .flag= flag,
            .enum_pos= flag->enum_values.pos - 1
        };

        get_or_add_enum(enum_ident->data.identifier, link);
    } while (expect_binary_op(PIPE));

    if (!expect_keyword(DEFAULT)) return unexpected("Flag enum list after enum values, expected `default` keyword with default value", current());
    const Token* default_value= expect(IDENTIFIER);
    if (!default_value) return unexpected("Flag enum list after default keyword, expected identifier for default value", current());

    size_t default_pos= find_enum_value(&flag->enum_values, default_value->data.identifier);
    if (default_pos == -1) return error("Cannot find flag enum value `%s` in flag `%s`", default_value->data.identifier, flag->name);

    flag->default_value= default_pos;

    add_to_symbol_table(flag->name, (Node*)flag);
    return (ParseRet) {
        .succ= true,
        .node= (Node*)flag
    };
}

ParseRet parse_expr() {
    // operators: ==, !=, ., and, or
    // constants: Enums, int literals
    // keyword: default which is just True
    // identifiers: flags, aliases
    const ShuntRet res= shunt(tokens, t_i);
    if (!res.succ) return PARSE_RET_FAIL;

    t_i= res.idx;
    return (ParseRet) {
        .succ= true,
        .node= res.node
    };
}

ParseRet parse_calc() {
    consume(); // eat the `CALCULATE`

    Token* ident= expect(IDENTIFIER);
    if (!ident) {
        return unexpected("Expected the flag identifier to calculate after calculate keyword", current());
    }

    Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident->data.identifier);
    if (!sym) return error("Unable to find flag `%s` in symbol table", ident->data.identifier);
    if (sym->node->type != NT_FLAG) return error("Symbol `%s` is not a flag in calculate statement", ident->data.identifier);
    FlagNode* flag= (FlagNode*)sym->node;

    if (!expect(ASSIGN)) {
        return unexpected("Expected `=` after calculate identifier", current());
    }

    CalcNode* calc_node= add_calc_node(ident->data.identifier);
    const ParseRet res= parse_braced_if_flags(&calc_node->rules, flag);

    if (!res.succ) return PARSE_RET_FAIL;

    flag->linked_calc= calc_node;

    return (ParseRet) {
        .succ= true,
        .node= (Node*)calc_node
    };
}

ParseRet parse_ident() {
    Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Expected identifier", current());

    Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident->data.identifier);
    if (!sym) return error("Unable to find symbol `%s` in the current symbol table");

    IdentNode* ident_node= add_ident_node(ident->data.identifier, sym->node);
    return (ParseRet) {
        .succ= true,
        .node= (Node*)ident_node
    };
}

ParseRet parse_lit_string() {
    const Token* str= expect(LIT_STRING);
    if (!str) return unexpected("Literal string, expected literal string token", current());

    LitNode* lit= add_lit_string_node(str->data.lit_string);

    size_t idx= 0, start= 0;
    char* string= str->data.lit_string;
    char c;
    while (c= string[idx], c != '\0') {
        idx++;

        if (c != '{') {
            continue;
        }

        set_lex_pos(&string[idx]);
        TokenArray expr_tokens= Token_arr_create();
        TokenRet res;
        do {
            res= lex_token();

            if (res.succ && res.addable) {
                if (res.token.type == LIT_NUM)
                    res.token.data.lit_num.explicit_base10= true;

                Token_arr_add(&expr_tokens, res.token);
            }
        } while (res.succ && res.token.type != RBRACE);

        const ShuntRet shunt_res= shunt(&expr_tokens, 0);

        if (!shunt_res.succ) return PARSE_RET_FAIL;
        if (shunt_res.idx != expr_tokens.pos - 1) return error("Did not consume all tokens in expression");

        printf("String %u expression tokens:\n", lit->data.lit_string.id);
        for (int i = 0; i < expr_tokens.pos; ++i) {
            print_token(Token_arr_ptr(&expr_tokens, i));
        }

        Node_vec_add(&lit->data.lit_string.expressions, shunt_res.node);
    }

    vector_add(&root.strings, lit);

    return (ParseRet) {
        .succ= true,
        .node= (Node*)lit
    };
}

ParseRet parse_map() {
    const Token* dst= expect(IDENTIFIER);
    if (!dst) return unexpected("Map statement, expected destination identifier", current());
    Symbol* sym= Symbol_arr_search_ie(&symbol_table, dst->data.identifier);
    if (!sym) return error("Cannot find symbol `%s` in map statement", dst->data.identifier);
    if (sym->node->type != NT_ALIAS) return error("Identifier `%s` in map statement's destination is not an alias", dst->data.identifier);

    MapNode* map= add_map_node();
    map->destination= (AliasNode*)sym->node;
    do {
        ShuntRet res= shunt(tokens, t_i);
        if (!res.succ) return error("Failed to parse expression in MAP stream");
        t_i= res.idx;

        res.node= ((ExprNode*)res.node)->expr;
        switch (res.node->type) {
            case NT_LIT_NUM:
                Node_vec_add(&map->stream, res.node);
                break;
            case NT_IDENT: {
                const IdentNode* ident= (IdentNode*)res.node;
                switch (ident->link->type) {
                    case NT_ALIAS: return error("Cannot stream an alias into MAP statement, got alias `%s`", link_name(res.node));
                    case NT_DATA: {
                        DataNode* data= (DataNode*)ident->link;
                        if (!data->non_fielded) return error("Cannot stream data that contains fields, got data `%s`",
                                                             link_name(res.node));
                        break;
                    }
                    default:
                        return error("Cannot stream identifier that is not a non-fielded data");
                }

                Node_vec_add(&map->stream, res.node);
                break;
            }
            case NT_BIN_EXPR: {
                BinNode* expr= (BinNode*)res.node;
                if (expr->op != DOT) return error("Only struct access `.` expressions are allowed in MAP stream");

                Node_vec_add(&map->stream, res.node);
                break;
            }
            default:
                return error("Map stream must contain literal numbers, identifiers, or data field access");
        }
    } while (!expect(DELIMITER));

    return (ParseRet) {
        .succ= true,
        .node= (Node*)map
    };
}

ParseRet parse_right_rule(AliasNode* alias) {
    // can be
    // struct BracedRules* brace;
    // MultiNode* multi_out;
    // Node* single_out;
    // MapNode* map;

    Token* next= current();
    if (!next) return PARSE_RET_FAIL;

    if (next->type == LBRACE) {
        BracedRules* br= add_braced_rules();
        const ParseRet res= parse_braced_rules(br, alias);
        return (ParseRet) {.succ= res.succ, .node= (Node*)br};
    }

    if (expect_keyword(MAP)) {
        const ParseRet map_res= parse_map();
        return map_res;
    }

    MultiNode* multi= add_multi_node();
    do {
        switch (next->type) {
            case IDENTIFIER: {
                const ParseRet res= parse_ident();
                if (!res.succ) return res;
                Node_vec_add(&multi->multis, res.node);
                break;
            }
            case LIT_STRING: {
                const ParseRet res= parse_lit_string();
                if (!res.succ) return res;
                Node_vec_add(&multi->multis, res.node);
                break;
            }
            default: return PARSE_RET_FAIL;
        }
    } while (expect(COMMA));

    if (alias->right_output_count == -1) {
        alias->right_output_count= multi->multis.pos;
    }

    if (alias->right_output_count != multi->multis.pos) {
        return error("Alias statements must have the same number of outputs on the right side from previous rule alias `%s` expects %zu outputs but read %zu",
            alias->identifier,
            alias->right_output_count,
            multi->multis.pos
        );
    }

    return (ParseRet) {
        .succ= true,
        .node= (Node*)multi
    };
}

RightRule node_to_right_rule(Node* node) {
    RightRule rule;
    rule.base= node;
    return rule;
}

ParseRet parse_if_rule(RuleArray* rules, AliasNode* alias) {
    consume(); // eat the 'if'

    const ParseRet expr= parse_expr();

    if (!expr.succ) return expr;

    if (!expect_keyword(THEN)) {
        return unexpected("If rule after expression", current());
    }

    const ParseRet right= parse_right_rule(alias);

    if (!right.succ) return PARSE_RET_FAIL;

    skip(DELIMITER);

    Rule r= create_rule(NT_RULE_IF);
    r.data.rule_if= create_rule_node_if(expr.node, node_to_right_rule(right.node));

    Rule_arr_add(rules, r);

    return PARSE_RET_SUCC;
}

ParseRet parse_if_flag_rule(IfFlagRuleArray* rules, FlagNode* flag, Node* expr) {
    IfFlagRule rule;

    const Token* ident= expect(IDENTIFIER);
    if (ident) {
        Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident->data.identifier);

        if (!sym) return error("Unable to find symbol `%s` in CALCULATE statement", ident->data.identifier);
        if (sym->node->type != NT_FLAG_VALUE) return error("Symbol %s is not a flag value in the result of a calculate statement", ident->data.identifier);

        FlagValueNode* fv= (FlagValueNode*)sym->node;
        bool found= flag_links_has_flag(fv, flag);

        if (!found) return error("FLAG value `%s` in CALCULATE `%s` is not from the same FLAG",
            ident->data.identifier,
            flag->name
        );

        rule.if_rule= add_rule_node_if(expr, (RightRule){.single_out= (Node*)fv});
        IfFlagRule_arr_add(rules, rule);
        return PARSE_RET_SUCC;
    }

    IfBracedRules* braced= add_if_braced_rules();
    rule.if_rule= add_rule_node_if(expr, (RightRule){.ifbrace= braced});
    const ParseRet res= parse_braced_if_flags(braced, flag);

    if (!res.succ) return PARSE_RET_FAIL;

    IfFlagRule_arr_add(rules, rule);

    return PARSE_RET_SUCC;
}

ParseRet parse_braced_if_flag(IfFlagRuleArray* rules, FlagNode* flag) {
    // this can only be an if statement and the output a braced_if_flags or just a flag identifier
    Token* c= expect(IDENTIFIER);
    if (c) {
        Symbol* sym= Symbol_arr_search_ie(&symbol_table, c->data.identifier);
        if (!sym) return error("Unable to find symbol `%s` in flag calculate rule `%s`",
            c->data.identifier,
            flag->name
        );
        if (sym->node->type == NT_VAR) {
            const VarNode* var= (VarNode*)sym->node;
            if (var->link != (Node*)flag) return error("Output of `%s`'s calculate statement cannot be a variable of a different type than the flag");
        } else if (sym->node->type != NT_FLAG_VALUE) return error("Output of `%s`'s calculate statement must be an enum value of the same type got identifier `%s` of non matching type",
            flag->name,
            c->data.identifier
        );

        IfFlagRule_arr_add(rules, (IfFlagRule) {
            .flag= (FlagValueNode*)sym->node,
        });

        return PARSE_RET_SUCC;
    }

    c= expect_keyword(IF);
    if (!c) {
        return unexpected("If/When rule in calculate statement, expected IF/WHEN keyword or enum value", current());
    }

    const ParseRet expr= parse_expr();

    if (!expr.succ) return PARSE_RET_FAIL;

    if (!expect_keyword(THEN)) {
        return unexpected("If rule after expression expects THEN keyword", current());
    }

    const ParseRet right_res= parse_if_flag_rule(rules, flag, expr.node);
    if (!right_res.succ) return PARSE_RET_FAIL;

    skip(DELIMITER);

    return PARSE_RET_SUCC;
}

ParseRet parse_braced_if_flags(IfBracedRules* rules, FlagNode* flag) {
    // this is a very specific function for parsing the rules for the calculate statement
    // where the rules can only be ifs or braces
    // and the output can only be a flag identifier
    if (!expect(LBRACE)) {
        return unexpected("Braced rules statement after equality sign", current());
    }

    expect(DELIMITER);

    while (!expect(RBRACE)) {
        ParseRet res;
        if (res= parse_braced_if_flag(&rules->rules, flag), !res.succ) {
            return res;
        }
        expect(DELIMITER);
    }

    expect(DELIMITER);

    if (rules->rules.pos == 0) {
        return error("Cannot have an empty braced rules list");
    }

    return PARSE_RET_SUCC;
}

ParseRet parse_braced_rule(RuleArray* rules, AliasNode* alias) {
    // Either an
    // L rule
    // or an LR rule
    // or an if rule
    // or a with rule
    // or a when rule
    Token* c= current();
    if (c->type == KEYWORD) {
        if (c->data.keyword == IF) return parse_if_rule(rules, alias);
        if (c->data.keyword == WITH) return parse_with(rules, alias);
        if (c->data.keyword == MAP) return parse_map();

        return unexpected_keyword("Braced rule, expected either IF, WHEN, or L/LR rule", current()->data.keyword);
    }

    return parse_l_or_lr_rule(rules, alias);
}

DataFieldNode* data_has_field(DataNode* data, const char* target_name) {
    for (int i = 0; i < data->all_fields.pos; ++i) {
        const FieldNode* field= FieldNode_vec_get_unsafe(&data->all_fields, i);

        if (strcmp(field->named_info.name, target_name) == 0) {
            return add_data_field_node(data, i);
        }
    }
    return NULL;
}

Node* link_of_var(VarNode* var) {
    return var->link;
}

ParseRet parse_assign() {
    const Token* c= expect(IDENTIFIER);
    if (!c) return unexpected("With statement's assign, expected identifier", current());
    Symbol* sym= Symbol_arr_search_ie(&symbol_table, c->data.identifier);
    if (!sym) return error("Cannot find symbol `%s` in with statement's assign", c->data.identifier);
    Node* base= sym->node;
    if (base->type == NT_VAR) {base= ((VarNode*)base)->link;}

    IdentNode* left;
    Node* left_link;
    NodeType left_link_type;
    if (expect_binary_op(DOT)) {
        if (base->type != NT_DATA) return error("Cannot field access into a variable that is not a data variable");
        DataNode* data= (DataNode*)base;
        const Token* field= expect(IDENTIFIER);
        if (!field) return unexpected("Assign after `.`, expected field identifier", current());
        DataFieldNode* res= data_has_field(data, field->data.identifier);
        if (!res) return error("Data variable `%s` does not have field `%s`", link_name(base), field->data.identifier);
        left= add_ident_node(c->data.identifier, (Node*)res);
        left_link= (Node*)res;
        left_link_type= NT_DATA_FIELD;
    } else {
        left= add_ident_node(c->data.identifier, sym->node);
        left_link= base;
        left_link_type= base->type;
    }

    if (!expect(ASSIGN)) return unexpected("Assign of WITH statement, expected `=`", current());

    const ParseRet right= parse_expr();
    if (!right.succ) return right;

    Node* expr= ((ExprNode*)right.node)->expr;
    if (!check_assignment(left_link, expr)) return error("Cannot verify assignment types in VAR `%s`", link_name(base));

    AssignNode* assign= add_assign_node();
    assign->left= left;
    assign->right= (ExprNode*)right.node;

    return (ParseRet) {
        .succ= true,
        .node= (Node*)assign
    };
}

ParseRet parse_with(RuleArray* rules, AliasNode* alias) {
    consume(); // eat `WITH`

    Rule* rule= Rule_arr_add_i(rules);
    *rule= create_rule(NT_RULE_WITH);

    RuleNodeWith with= create_with_node(alias);
    rule->data.rule_with= with;
    do {
        const ParseRet res= parse_assign();
        if (!res.succ) return res;
        vector_add(&rule->data.rule_with.assignNodes, res.node);
    } while (expect(COMMA));

    BracedRules* br= add_braced_rules();
    const ParseRet res= parse_braced_rules(br, alias);
    if (!res.succ) return res;
    rule->data.rule_with.brace= br;

    vector_add(&root.withs, &rule->data.rule_with);

    return PARSE_RET_SUCC;
}

ParseRet parse_left_rules(LeftRules* lr) {
    eat_all_whitespace();
    const Token* next= current();

    if (expect(UNDERSCORE)) {
        return PARSE_RET_SUCC;
    }

    bool running= true;
    while (running) {
        bool invert= expect(READINVERT);
        if (invert) {
            next= current();
        }
        switch (next->type) {
            case LIT_NUM: {
                Token* num= consume();
                const LeftRule rule= (LeftRule){.lit= add_lit_number_node(num->data.lit_num)};
                LeftRule_arr_add(lr, rule);
                break;
            }
            case IDENTIFIER: {
                Token* ident= consume();
                Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident->data.identifier);
                if (!sym) return error("Unable to find symbol `%s` in left rules", ident->data.identifier);
                switch (sym->node->type) {
                    case NT_FLAG:
                    case NT_FLAG_VALUE: {
                        return error("Left value cannot be a flag or flag value");
                    }
                    case NT_DATA:
                    case NT_DATA_FIELD:
                    case NT_ALIAS:
                        break;
                    default:
                        assert(false);
                }

                IdentNode* ident_node= add_ident_node(ident->data.identifier, sym->node);
                if (invert) {
                    const DataNode* data= (DataNode*)base_link(ident_node);
                    if (data->base.type != NT_DATA || !data->non_fielded) return error("Can only invert non-fielded data");
                }

                const LeftRule rule= (LeftRule){.ident= ident_node};
                rule.ident->inverted= invert;
                LeftRule_arr_add(lr, rule);
                break;
            }
            default:
                running= false;
                break;
        }
        next= current();
    }

    if (lr->pos == 0) return error("Unable to parse left rules");
    return PARSE_RET_SUCC;
}

ParseRet parse_l_or_lr_rule(RuleArray* rules, AliasNode* alias) {
    LeftRules lr= LeftRule_arr_create();
    const ParseRet l_rule= parse_left_rules(&lr);

    if (alias->is_flat) {
        if (lr.pos != 1 || LeftRule_arr_get(&lr, 0).base->type != NT_LIT_STRING) {
            return error("Flat aliases can only have literal strings as left rules, alias `%s` got problems", alias->identifier);
        }
    }

    if (!l_rule.succ) return PARSE_RET_FAIL;

    if (expect(ASSIGN)) {
        if (alias->is_flat) return error("Flat aliases cannot have lr rules, alias `%s` is in violation >:[", alias->identifier);
        Rule rule= create_rule(NT_RULE_LR);
        ParseRet r_rule= parse_right_rule(alias);
        if (!r_rule.succ) return r_rule;

        rule.data.rule_lr= create_rule_lr(lr, (RightRule){.base= r_rule.node});

        Rule_arr_add(rules, rule);

        return PARSE_RET_SUCC;
    }

    Rule rule= create_rule(NT_RULE_L);
    rule.data.rule_l= create_rule_l(lr);
    Rule_arr_add(rules, rule);

    return PARSE_RET_SUCC;
}

ParseRet parse_braced_rules(BracedRules* results, AliasNode* alias) {
    if (!expect(LBRACE)) {
        return unexpected("Braced rules statement after equality sign", current());
    }

    eat_all_whitespace();

    while (!expect(RBRACE)) {
        ParseRet res;
        if (res= parse_braced_rule(&results->rules, alias), !res.succ) {
            return res;
        }
        eat_all_whitespace();
    }

    eat_all_whitespace();

    if (results->rules.pos == 0) {
        return error("Cannot have an empty braced rules list");
    }

    return PARSE_RET_SUCC;
}

ParseRet parse_size(uint32_t* res) {
    // This could be the size of the alias
    const Token* size= expect(LIT_NUM);
    uint32_t size_in_bits= 0;
    if (size) {
        Token* byte= expect_keyword(BYTES);
        Token* bits= expect_keyword(BITS);

        if (byte && bits) return unexpected("How did we get here, bytes followed by bits?! Don't do that", current());
        if (!byte && !bits) return unexpected("Expected size quantifier `BIT(S)` or `BYTE(S)` following size in statement", current());

        size_in_bits= convert_lit_num_to_base10(size->data.lit_num, true);
        if (byte) size_in_bits *= 8;
    }
    *res= size_in_bits;

    return PARSE_RET_SUCC;
}

uint64_t convert_lit_num_to_base10(const struct LitNumData lit_num, const bool expecting_base10) {
    if (lit_num.explicit_base10) {
        return lit_num.base10.value;
    }

    if (expecting_base10) {
        return lit_num.base10.value;
    }

    return lit_num.base2.value;
}

ParseRet parse_flat_alias() {

}

ParseRet parse_alias() {
    const Token* first= consume(); // eat the ALIAS or FLAT keyword
    bool is_flat= first->data.keyword == FLAT;
    if (is_flat) {
        if (!expect_keyword(ALIAS)) return unexpected("Flat alias statement expected `ALIAS` after `FLAT`", current());
    }

    Token* ident= expect(IDENTIFIER);

    if (!ident) {
        return unexpected("Expected identifier after alias keyword for alias name", current());
    }

    uint32_t size_in_bits= 0;
    const ParseRet size_res= parse_size(&size_in_bits);
    if (!size_res.succ) return size_res;

    if (!expect(ASSIGN)) {
        return unexpected("Expected `=` after alias identifier / alias size", current());
    }

    AliasNode* alias= add_alias_node(ident->data.identifier);
    alias->bits= size_in_bits;
    alias->is_flat= is_flat;

    if (check(LBRACE)) {
        const ParseRet res= parse_braced_rules(&alias->rules, alias);
        if (!res.succ) return PARSE_RET_FAIL;

        goto parse_alias_end;
    }

    if (is_flat) {
        //[[todo]] this isn't true, it could be another flat alias, but that requires changing the logic below as well, as it would just be ={other_alias} not ={x=x}
        return error("Flat aliases cannot be = another alias, as that requires l rules");
    }

    Token* result_alias= expect(IDENTIFIER);
    if (!result_alias) {
        return unexpected("Expected identifier or `{` after alias", current());
    }

    Symbol* symbol= Symbol_arr_search_ie(&symbol_table, result_alias->data.identifier);

    if (!symbol) {
        return error("Cannot find symbol %s in existing symbol table", result_alias->data.identifier);
    }

    if (symbol->node->type != NT_ALIAS) {
        return error("When using alias with only one symbol on the right the symbol must be another alias node");
    }

    // todo check why this isn't us
    AliasNode* r_alias= (AliasNode*)symbol->node;
    IdentNode* i= add_ident_node(ident->data.identifier, symbol->node);

    Rule r= {
        .base.type= NT_RULE_LR,
        .data.rule_lr= (RuleNodeLR) {
            .base.type= NT_RULE_LR,
            .left= LeftRule_arr_construct(1),
            .right= (RightRule) {
                .multi_out= add_multi_node()
            }
        }
    };
    Node_vec_add(&r.data.rule_lr.right.multi_out->multis, (Node*)i);

    LeftRule_arr_add(&r.data.rule_lr.left, (LeftRule) {
        .ident= i
    });

    Rule_arr_add(&alias->rules.rules, r);

parse_alias_end:
    // this is done last so that cycles cannot exist
    if (!add_to_symbol_table(alias->identifier, (Node*)alias).succ) {
        return PARSE_RET_FAIL;
    }

    return (ParseRet) {
        .succ= true,
        .node= (Node*)alias
    };
}

ParseRet add_to_symbol_table(const char* name, Node* link) {
    if (Symbol_arr_search_i(&symbol_table, name) != ARR_NOT_FOUND) {
        return error("Symbol with name %s already exists", name);
    }

    Symbol_arr_add_sorted_i(&symbol_table, (Symbol) {
        .name= name,
        .node= link
    });

    return PARSE_RET_SUCC;
}

Token* current() {
    return Token_arr_ptr(tokens, t_i);
}

Token* peek() {
    return peer(1);
}

Token* peer(uint distance) {
    return Token_arr_ptr(tokens, t_i + distance);
}

Token* consume() {
    return Token_arr_ptr(tokens, t_i++);
}

Token* expect_keyword(keyword kw) {
    if (!current() || current()->type != KEYWORD || current()->data.keyword != kw) {
        return NULL;
    }

    return consume();
}

Token* expect_binary_op(BinaryOperator op) {
    if (!current() || current()->type != BINARY_OP || current()->data.bin_op != op) {
        return NULL;
    }

    return consume();
}

Token* expect_unary_op(UnaryOperator op) {
    if (!current() || current()->type != UNARY_OP || current()->data.unary_op != op) {
        return NULL;
    }

    return consume();
}

void skip(TokenType type) {
    expect(type);
}

Token* check(const TokenType type) {
    if (!current() || current()->type != type) return NULL;

    return current();
}

Token* expect(TokenType type) {
    if (!current() || current()->type != type) {
        return NULL;
    }

    return consume();
}

ParseRet unexpected_keyword(const char* context, keyword kw) {
    printf("<<ERROR>> unexpeceted keyword in %s got %s\n", context, keyword_string(kw));

    return PARSE_RET_FAIL;
}

ParseRet funexpected(const char* context, Token* unexpected_token, ...) {
    va_list args;
    va_start(args, unexpected_token);

    printf("<<ERROR>> Unexpected token in ");
    vprintf(context, args);
    printf(" got ");
    print_token(unexpected_token);
    newline();

    va_end(args);

    return PARSE_RET_FAIL;
}

ParseRet unexpected(const char* context, Token* unexpected_token) {
    printf("<<ERROR>> Unexpected token in %s got ", context);
    print_token(unexpected_token);
    printf("\n");

    return PARSE_RET_FAIL;
}

ParseRet error_with_token(const char* message, Token* tok, ...) {
    va_list args;

    va_start(args, tok);
    printf("<<ERROR>> AT ");
    print_token(tok);
    printf(" :");
    vprintf(message, args);
    va_end(args);

    return PARSE_RET_FAIL;
}

ParseRet error(const char* message, ...) {
    va_list args;

    va_start(args, message);
    printf("<<ERROR>>: ");
    vprintf(message, args);
    va_end(args);

    newline();

    return PARSE_RET_FAIL;
}

void create_node_buffer() {
    node_buffer= calloc(1, MIN_NODE_BUFFER_SIZE);
    node_buffer_size= 0;
    node_buffer_capacity= MIN_NODE_BUFFER_SIZE;

    node_buffers= vector_create();
    vector_add(&node_buffers, node_buffer);
}

void resize_node_buffer() {
    void* new_ptr= calloc(node_buffer_capacity << 1, sizeof(uint8_t));

    if (!new_ptr) {
        error("Unable to realloc node buffer, this may be because of my `<< 1` on full buffer :)");

        node_buffer= NULL; // push the problem down the line :)
        return;
    }

    vector_add(&node_buffers, new_ptr);

    node_buffer= new_ptr;
    node_buffer_capacity <<= 1;
    node_buffer_size= 0;
}

void* add_node(void* node_ptr, size_t node_bytes) {
    if (node_buffer_size + node_bytes > node_buffer_capacity) {
        resize_node_buffer();
    }

    void* res= node_buffer + node_buffer_size;

    memcpy(res, node_ptr, node_bytes);
    node_buffer_size += node_bytes;

    return res;
}

FlagNode create_flag_node(const char* name) {
    return (FlagNode) {
        .base= {.type= NT_FLAG},

        .name= name,
        .capitalised= capitalised(name),
        .enum_values= vector_create(),
        .default_value= -1
    };
}

FlagNode* add_flag_node(const char* name) {
    FlagNode node= create_flag_node(name);
    return add_node(&node, sizeof(node));
}

DataNode create_data_node(const char* name) {
    return (DataNode) {
        .base= {.type= NT_DATA},

        .name= name,
        .capitalised= capitalised(name),
        .bits= 0,
        .all_fields= FieldNode_vec_create(),
        .rows= FieldNodeVector_arr_create(),
        .non_fielded= false,
    };
}

DataNode* add_data_node(const char* name) {
    DataNode data= create_data_node(name);
    return add_node(&data, sizeof(data));
}

FieldNode create_field_node() {
    return (FieldNode) {
        .base= {.type= NT_FIELD},

        .named= false,
        .named_info= {0},
    };
}

FieldNode* add_field_node() {
    FieldNode field= create_field_node();
    return add_node(&field, sizeof(field));
}

MultiNode create_multi_node() {
    return (MultiNode) {
        .base= {.type= NT_MULTI},
        .multis= Node_vec_create(),
    };
}

MultiNode* add_multi_node() {
    MultiNode multi= create_multi_node();
    return add_node(&multi, sizeof(multi));
}

StructureNode create_structure_node() {
    return (StructureNode) {
        .base= {.type= NT_STRUCTURE},
        .rules= MarkedIdent_arr_create()
    };
}

StructureNode* add_structure() {
    StructureNode node= create_structure_node();
    return add_node(&node, sizeof(node));
}

RuleNodeLR create_rule_lr(LeftRules left, RightRule right) {
    return (RuleNodeLR) {
        .base= {.type= NT_RULE_LR},
        .left= left,
        .right= right,
    };
}

RuleNodeLR* add_rule_lr(LeftRules left, RightRule right) {
    RuleNodeLR node= create_rule_lr(left, right);
    return add_node(&node, sizeof(node));
}

RuleNodeL create_rule_l(LeftRules left) {
    return (RuleNodeL) {
        .base= {.type= NT_RULE_L},
        .rules= left,
    };
}

RuleNodeL* add_rule_l(LeftRules left) {
    RuleNodeL node= create_rule_l(left);
    return add_node(&node, sizeof(node));
}

RootNode create_root() {
    return (RootNode) {
        .base= {
            .type= NT_ROOT
        },
        .child_nodes= create_children(),
        .strings= vector_create(),
        .withs= vector_create()
    };
}

BracedRules create_braced_rules() {
    return (BracedRules) {
        .base= {
            .type= NT_BRACED_RULES
        },
        .rules= Rule_arr_create()
    };
}

BracedRules* add_braced_rules() {
    BracedRules br= create_braced_rules();
    return add_node(&br, sizeof(br));
}

IfBracedRules create_if_braced_rules() {
    return (IfBracedRules){
        .base= {.type= NT_IF_BRACED_RULES},
        .rules= IfFlagRule_arr_create(),
    };
}

IfBracedRules* add_if_braced_rules() {
    IfBracedRules br= create_if_braced_rules();
    return add_node(&br, sizeof(br));
}

Rule create_rule(NodeType type) {
    return (Rule) {
        .base= {.type= type}
    };
}

Rule* add_rule(NodeType type) {
    Rule rule= create_rule(type);
    return add_node(&rule, sizeof(rule));
}

RuleNodeIf create_rule_node_if(Node* expr, RightRule output) {
    return (RuleNodeIf) {
        .base= {.type= NT_RULE_IF},
        .condition= expr,
        .output= output
    };
}

RuleNodeIf* add_rule_node_if(Node* expr, RightRule output) {
    RuleNodeIf rule_node_if= create_rule_node_if(expr, output);
    return add_node(&rule_node_if, sizeof(rule_node_if));
}

size_t get_rule_right_stmt_id() {
    return rule_right_stmt_id++;
}

RuleRightNode create_rule_right_node() {
    return (RuleRightNode) {
        .base= {.type= NT_RULE_RIGHT_STMT},
        .expressions= Node_vec_create(),
        .id= get_rule_right_stmt_id()
    };
}

RuleRightNode* add_rule_right_node() {
    RuleRightNode rule_right_node= create_rule_right_node();
    return add_node(&rule_right_node, sizeof(rule_right_node));
}

IdentNode create_ident_node(const char* name, Node* link) {
    return (IdentNode) {
        .base= {.type= NT_IDENT},
        .link= link,
        .name= name,
        .inverted= false
    };
}

IdentNode* add_ident_node(const char* name, Node* link) {
    IdentNode ident_node= create_ident_node(name, link);
    return add_node(&ident_node, sizeof(ident_node));
}

CalcNode create_calc_node(const char* identifier) {
    return (CalcNode) {
        .base.type= NT_CALCULATE,
        .rules= create_if_braced_rules(),
        .identifier= identifier,
        .capitalised= capitalised(identifier),
    };
}

CalcNode* add_calc_node(const char* identifier) {
    CalcNode calc= create_calc_node(identifier);
    return add_node(&calc, sizeof(calc));
}

AliasNode create_alias_node(const char* identifier) {
    return (AliasNode) {
        .base= {
            .type= NT_ALIAS,
        },
        .rules= create_braced_rules(),
        .identifier= identifier,
        .capitalised= capitalised(identifier),
        .bits= -1,
        .linked_rule= NULL,
        .right_output_count= -1
    };
}

AliasNode* add_alias_node(const char* name) {
    AliasNode alias= create_alias_node(name);
    return add_node(&alias, sizeof(alias));
}

FlagValueNode create_flag_value_node() {
    return (FlagValueNode) {
        .base= {.type= NT_FLAG_VALUE},
        .links= FlagLinkPos_arr_create(),
    };
}

FlagValueNode* add_flag_value_node() {
    FlagValueNode flag_value_node= create_flag_value_node();
    return add_node(&flag_value_node, sizeof(flag_value_node));
}

LitNode* add_lit_string_node(char* string) {
    LitStringData data= {
        .string= string,
        .expressions= Node_vec_create(),
        .id= string_id++
    };

    LitNode lit= (LitNode) {
        .base=  {
            .type= NT_LIT_STRING
        },
        .data.lit_string= data
    };

    LitNode* node= add_node(&lit, sizeof(lit));

    return node;
}

SimpleNumData complex_to_simple_num(struct LitNumData num, bool context_binary) {
    SimpleNumData result;

    if (num.explicit_base10 || !context_binary) {
        result= (SimpleNumData){
            .value= num.base10.value,
            .bits= num.base10.digits,
            .show_as_bin= false
        };
    }
    else {
        result= (SimpleNumData){
            .value= num.base2.value,
            .bits= num.base2.digits,
            .show_as_bin= true
        };
    }

    return result;
}

LitNode* add_lit_number_node(LitNumData num) {
    SimpleNumData simple= complex_to_simple_num(num, true);

    LitNode lit= (LitNode) {
        .base= {
            .type= NT_LIT_NUM
        },
        .data.lit_number= simple,
    };

    LitNode* node= add_node(&lit, sizeof(lit));

    return node;
}

UnaryNode create_unary_node(const UnaryOperator op, OperandNode operand) {
    return (UnaryNode) {
        .base= {.type= NT_UNARY_EXPR},
        .op= op,
        .operand= operand
    };
}

UnaryNode* add_unary_node(const UnaryOperator op, OperandNode operand) {
    UnaryNode node= create_unary_node(op, operand);
    return add_node(&node, sizeof(node));
}

BinNode create_binary_node(const BinaryOperator op, OperandNode left, OperandNode right) {
    return (BinNode) {
        .base= {.type= NT_BIN_EXPR},
        .op= op,
        .left= left,
        .right= right
    };
}

BinNode* add_binary_node(const BinaryOperator op, OperandNode left, OperandNode right) {
    BinNode node= create_binary_node(op, left, right);
    return add_node(&node, sizeof(node));
}

VarNode create_var_node(const char* identifier, Node* link) {
    return (VarNode) {
        .base= {.type= NT_VAR},
        .identifier= identifier,
        .link= link,
        .value= NULL
    };
}

VarNode* add_var_node(const char* identifier, Node* link) {
    VarNode node= create_var_node(identifier, link);
    return add_node(&node, sizeof(node));
}

MapNode create_map_node() {
    return (MapNode) {
        .base= {.type= NT_MAP},
        .destination= NULL,
        .stream= Node_vec_create()
    };
}

MapNode* add_map_node() {
    MapNode node= create_map_node();
    return add_node(&node, sizeof(node));
}

DataFieldNode create_data_field_node(DataNode* data, size_t pos) {
    return (DataFieldNode) {
        .base= {.type= NT_DATA_FIELD},
        .pos= pos,
        .data= data
    };
}

DataFieldNode* add_data_field_node(DataNode* data, size_t pos) {
    DataFieldNode node= create_data_field_node(data, pos);
    return add_node(&node, sizeof(node));
}

ExprNode create_expr_node() {
    return (ExprNode) {
        .base= {.type= NT_EXPR},
        .type= {0},
        .expr= NULL
    };
}

ExprNode* add_expr_node() {
    ExprNode expr= create_expr_node();
    return add_node(&expr, sizeof(expr));
}

MetaNode create_meta_node() {
    return (MetaNode) {
        .base= {.type= NT_META},
        .meta= {
            .name= NULL,
            .endianness= ENDIAN_LITTLE,
            .parsed= false
        }
    };
}

MetaNode* add_meta_node() {
    MetaNode meta= create_meta_node();
    return add_node(&meta, sizeof(meta));
}

AssignNode create_assign_node() {
    return (AssignNode) {
        .base= {.type= NT_ASSIGN},
        .left= NULL,
        .right= NULL
    };
}

AssignNode* add_assign_node() {
    AssignNode node= create_assign_node();
    return add_node(&node, sizeof(node));
}

RuleNodeWith create_with_node(AliasNode* alias) {
    return (RuleNodeWith) {
        .base= {.type= NT_RULE_WITH},
        .brace= NULL,
        .assignNodes= vector_create(),
        .id= next_with_id(),
        .alias= alias
    };
}

RuleNodeWith* add_with_node(AliasNode* alias) {
    RuleNodeWith node= create_with_node(alias);
    return add_node(&node, sizeof(node));
}

Vector create_children() {
    return vector_create();
}

void print_simple_num(const SimpleNumData* num) {
    fprint_simple_num(stdout, num);
}

void fprint_simple_num(FILE* file, const SimpleNumData* num) {
    if (num->show_as_bin) {
        fprintf(file, "0b");
        assert(num->bits != 0);
        for (int32_t i = num->bits - 1; i >= 0; i--) {
            fprintf(file, "%c", num->value >> i & 1 ? '1' : '0');
        }
    } else {
        fprintf(file, "%#lx", num->value);
    }
}

Node* check_link(const char* identifier) {
    Symbol* sym= Symbol_arr_search_ie(&symbol_table, identifier);
    if (!sym) return NULL;

    return sym->node;
}

typedef enum NodePrintPrefix {
    NODE_PRINT_EMPTY, // "    "
    NODE_PRINT_LINK,  // "|---"
    NODE_PRINT_LAST,  // "`---"
    NODE_PRINT_SKIP,  // "|   "
} NodePrintPrefix;

ARRAY_PROTO(NodePrintPrefix, Prefix)
ARRAY_ADD(NodePrintPrefix, Prefix)

const char* prefix_string(const NodePrintPrefix prefix) {
    switch (prefix) {
        case NODE_PRINT_EMPTY: return "   ";
        case NODE_PRINT_LINK: return "|--";
        case NODE_PRINT_LAST: return "`--";
        case NODE_PRINT_SKIP: return "|  ";
        default: return "<<ERROR>> Node level prefix unknown";
    }
}

void print_prefix(const NodePrintPrefix prefix) {
    putz(prefix_string(prefix));
}

void print_prefixes(const PrefixArray* prefixes) {
    for (int i = 0; i < prefixes->pos; ++i) {
        const NodePrintPrefix prefix= Prefix_arr_get(prefixes, i);

        print_prefix(prefix);
    }
}

void print_node(Node* node, PrefixArray* prefixes);

void print_root(RootNode* root) {
    PrefixArray prefixes= Prefix_arr_create();

    print_node((Node*)root, &prefixes);

    Prefix_arr_destroy(&prefixes);
}

const char* types_to_colour(const NodeType type) {
    switch (type) {
        case NT_ROOT:
            return C_RED;
        case NT_ALIAS:
        case NT_DATA:
        case NT_STRUCTURE:
        case NT_FLAG:
        case NT_CALCULATE:
            return C_BLU;

        case NT_DATA_FIELD:
            break;
            break;
        case NT_RULE_RIGHT_STMT:
            break;
        case NT_STATEMENTS:
            break;
            break;
        case NT_FLAG_VALUE:
            break;
            break;
        case NT_BRACED_RULES:
            break;
        case NT_IF_BRACED_RULES:
            break;
        case NT_MULTI:
            break;
        case NT_RULE_IF:
            break;
        case NT_RULE_WHEN:
            break;
        case NT_RULE_L:
            break;
        case NT_RULE_LR:
            break;
            break;
        case NT_FIELD:
            break;
        case NT_BIN_EXPR:
        case NT_UNARY_EXPR:
        case NT_EXPR:
            return C_RED;
        case NT_LIT_STRING:
        case NT_LIT_NUM:
        case NT_IDENT:
            return C_CYN;
    }

    return C_RST;
}

const char* types_to_string(const NodeType type) {
    switch (type) {
        case NT_ROOT: return "NT_ROOT";
        case NT_ALIAS: return "NT_ALIAS";
        case NT_META: return "NT_META";
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
        case NT_DATA_FIELD: return "NT_DATA_FIELD";
        case NT_RULE_RIGHT_STMT: return "NT_RULE_RIGHT_STMT";
        case NT_FLAG_VALUE: return "NT_FLAG_VALUE";
        case NT_CALCULATE: return "NT_CALCULATE";
        case NT_IF_BRACED_RULES: return "NT_IF_BRACED_RULES";
        case NT_MULTI: return "NT_MULTI";
        case NT_BIN_EXPR: return  "NT_BIN_EXPR";
        case NT_UNARY_EXPR: return "NT_UNARY_EXPR";
        case NT_EXPR: return "NT_EXPR";
        case NT_MAP: return "NT_MAP";
        case NT_ASSIGN: return "NT_ASSIGN";
        case NT_VAR: return "NT_VAR";
        case NT_RULE_WITH: return "NT_WITH";

        default: return "<<ERROR>> unknown node type <<ERROR>>";
    }
}

void print_titled_child(Node* node, PrefixArray* prefixes, const bool last, const char* title) {
    print_prefixes(prefixes);

    print_prefix(last ? NODE_PRINT_LAST : NODE_PRINT_LINK);
    Prefix_arr_add(prefixes, last ? NODE_PRINT_EMPTY : NODE_PRINT_SKIP);
    if (title)
        printf("%s: ", title);
    print_node(node, prefixes);
    Prefix_arr_pop(prefixes);
}

void print_child(Node* node, PrefixArray* prefixes, const bool last) {
    print_titled_child(node, prefixes, last, NULL);
}

void print_child_(void* node, PrefixArray* prefixes, const bool last) {
    return print_child(node, prefixes, last);
}

void print_child_ptr_(void* node, PrefixArray* prefixes, const bool last) {
    return print_child(*(void**)node, prefixes, last);
}

typedef void (*print_child_func)(void* item, PrefixArray* prefixes, bool last);

void print_children_arr(const Array* children, PrefixArray* prefixes, print_child_func func) {
    if (children->pos == 0) return;

    for (int i = 0; i < children->pos; ++i) {
        const bool last= children->pos - 1 == i;

        void* child= arr_ptr(children, i);

        func(child, prefixes, last);
    }
}

void print_children(const Vector* children, PrefixArray* prefixes, print_child_func func) {
    if (children->pos == 0) return;

    for (int i = 0; i < children->pos; ++i) {
        const bool last= children->pos - 1 == i;

        void* child= vector_get_unsafe(children, i);

        func(child, prefixes, last);
    }
}

void print_meta(const MetaData* meta) {
    printf("Meta (%sparsed): ISA Name=%s\n", // technically not correct for children == 0 of root
        meta->parsed ? "" : "Not ",
        meta->name
    );
}

#define CHILD_STRING "|--"
#define LAST_CHILD_STRING "`--"

void print_prefix_as_if_child() {
    putz(CHILD_STRING);
}

void print_prefix_as_if_last_child() {
    putz(LAST_CHILD_STRING);
}

const char* link_name(Node* link) {
    switch (link->type) {
        case NT_FLAG: return ((FlagNode*)link)->name;
        case NT_ALIAS: return ((AliasNode*)link)->identifier;
        case NT_DATA: return ((DataNode*)link)->name;
        default: assert(false);
    }
}

void print_string_child(void* child, PrefixArray* prefixes, const bool last) {
    const char* str= child;

    print_prefixes(prefixes);

    print_prefix(last ? NODE_PRINT_LAST : NODE_PRINT_LINK);
    Prefix_arr_add(prefixes, last ? NODE_PRINT_EMPTY : NODE_PRINT_SKIP);
    printf("%s\n", str);
    Prefix_arr_pop(prefixes);
}

void print_flag_link_pos(void* child, PrefixArray* prefixes, const bool last) {
    const FlagLinkPos* flp= (FlagLinkPos*)child;

    print_prefixes(prefixes);

    print_prefix(last ? NODE_PRINT_LAST : NODE_PRINT_LINK);
    Prefix_arr_add(prefixes, last ? NODE_PRINT_EMPTY : NODE_PRINT_SKIP);
    printf("Flag link: %s\n", flp->flag->name);
    Prefix_arr_pop(prefixes);
}

void print_field_node(const FieldNode* field) {
    if (field->named) {
        printf(".%s(%u)", field->named_info.name, field->named_info.bits);
        if (field->named_info.has_default) {
            printf("=%#lx", field->named_info.default_value.value);
        }
    } else {
        print_simple_num(&field->num);
    }
}

void print_data_row(void* row, PrefixArray* prefixes, const bool last) {
    const FieldNodeVector* fnv= (FieldNodeVector*)row;

    print_prefixes(prefixes);

    print_prefix(last ? NODE_PRINT_LAST : NODE_PRINT_LINK);
    Prefix_arr_add(prefixes, last ? NODE_PRINT_EMPTY : NODE_PRINT_SKIP);

    for (int i = 0; i < fnv->pos; ++i) {
        const FieldNode* field= FieldNode_vec_get_unsafe(fnv, i);
        print_field_node(field);
        printf(" ");
    }
    newline();

    Prefix_arr_pop(prefixes);
}

void print_rule(void* rule, PrefixArray* prefixes, const bool last) {
    const Rule* r= (Rule*)rule;

    print_prefixes(prefixes);

    print_prefix(last ? NODE_PRINT_LAST : NODE_PRINT_LINK);
    Prefix_arr_add(prefixes, last ? NODE_PRINT_EMPTY : NODE_PRINT_SKIP);
    print_node((Node*)&r->data, prefixes);
    Prefix_arr_pop(prefixes);
}

#define PP print_prefixes(prefixes);
#define PPC PP print_prefix_as_if_child();
#define PPX(last) print_prefixes(prefixes), (last ? print_prefix_as_if_last_child() : print_prefix_as_if_child());

#define PrintAsChild_(stmts, last) \
    print_prefixes(prefixes);\
    print_prefix(last ? NODE_PRINT_LAST : NODE_PRINT_LINK);\
    Prefix_arr_add(prefixes, last ? NODE_PRINT_EMPTY : NODE_PRINT_SKIP);\
    {\
        stmts\
    }\
    Prefix_arr_pop(prefixes);

#define PrintAsChild(stmts) PrintAsChild_(stmts, false)
#define PrintAsLastChild(stmts) PrintAsChild_(stmts, true)

void print_node(Node* node, PrefixArray* prefixes) {
    if (!node) {
        printf("<<NULL>>\n");
        return;
    }

    printf("%s%s"C_RST"\n", types_to_colour(node->type), types_to_string(node->type));

    switch (node->type) {
        case NT_ROOT: {
            const RootNode* root= (RootNode*)node;
            PrintAsChild(print_meta(&root->meta->meta);)
            print_children(&root->child_nodes, prefixes, print_child_);
            break;
        }
        case NT_ALIAS: {
            const AliasNode* alias= (AliasNode*)node;
            PPC printf("Name: %s (%s)\n", alias->identifier, alias->capitalised);
            PPC printf("Bits: %u\n", alias->bits);
            PPC printf("Output count: %lu\n", alias->right_output_count);
            if (alias->linked_rule) {
                PPC printf("Calculate link: %u\n", alias->linked_rule->id);
            }

            print_child((Node*)&alias->rules, prefixes, true);
            break;
        }
        case NT_DATA: {
            const DataNode* data= (DataNode*)node;
            PPC printf("Name: %s (%s)\n", data->name, data->capitalised);
            if (!data->non_fielded) {
                PPC printf("Bits: %lu\n", data->bits);
                PPC printf("Row count: %zu\n", data->rows.pos);
                PrintAsChild(
                    printf("Rows:\n");
                    print_children_arr((Array*)&data->rows, prefixes, print_data_row);
                )
                PrintAsLastChild(
                    printf("All Fields:\n");
                    print_data_row((void*)&data->all_fields, prefixes, true);
                )
            } else {
                PrintAsLastChild(printf("Bits: %lu\n", data->bits);)
            }
            break;
        }
        case NT_DATA_FIELD:
            break;
        case NT_STRUCTURE: {
            const StructureNode* structure= (StructureNode*)node;
            PrintAsLastChild(printf("Rules: ");)
            for (int i = 0; i < structure->rules.pos; ++i) {
                const MarkedIdent* ident= MarkedIdent_arr_ptr(&structure->rules, i);
                printf("%s", link_name(ident->ident));
                switch (ident->type) {
                    case MARKED_QUESTION: putchar('?'); break;
                    case MARKED_STAR: putchar('*'); break;
                    case MARKER_NONE:
                    default:
                        break;
                }
                putchar(' ');
            }
            newline();
            break;
        }
        case NT_MAP: {
            const MapNode* map= (MapNode*)node;

            PPC printf("Destination: %s\n", map->destination->identifier);
            PrintAsLastChild(
                printf("Stream:\n");
                print_children((Vector*)&map->stream, prefixes, print_child_);
            );
            break;
        }
        case NT_RULE_RIGHT_STMT: {
            const RuleRightNode* rr= (RuleRightNode*)node;
            PPC printf("id: %u\n", rr->id);
            PPC printf("Rules (%zu):\n", rr->expressions.pos);
            print_children((Vector*)&rr->expressions, prefixes, print_child_);
            break;
        }
        case NT_STATEMENTS: assert(false);
        case NT_FLAG: {
            const FlagNode* flag = (FlagNode*)node;
            PPC printf("Name: %s (%s)\n", flag->name, flag->capitalised);
            PPC printf("Default: %s\n", (char*)vector_get_unsafe(&flag->enum_values, flag->default_value));
            PrintAsLastChild(
                printf("Enums:\n");
                print_children(&flag->enum_values, prefixes, print_string_child);
            )
            break;
        }
        case NT_FLAG_VALUE: {
            const FlagValueNode* fv= (FlagValueNode*)node;
            if (fv->links.pos == 0) assert(false);

            const FlagLinkPos* first= FlagLinkPos_arr_ptr(&fv->links, 0);
            PPC printf("Name: %s\n", (char*)vector_get_unsafe(&first->flag->enum_values, first->enum_pos));
            PrintAsLastChild(
                printf("Flag links:\n");
                print_children_arr((Array*)&fv->links, prefixes, print_flag_link_pos);
            )

            break;
        }
        case NT_CALCULATE: {
            const CalcNode* calc= (CalcNode*)node;
            PPC printf("Name: %s (%s)", calc->identifier, calc->capitalised);

            print_child((Node*)&calc->rules, prefixes, true);
            break;
        }
        case NT_BRACED_RULES: {
            const BracedRules* br= (BracedRules*)node;
            print_children_arr((Array*)&br->rules, prefixes, print_rule);
            break;
        }
        case NT_IF_BRACED_RULES: {
            const IfBracedRules* br= (IfBracedRules*)node;
            print_children_arr((Array*)&br->rules, prefixes, print_child_ptr_);
            break;
        }
        case NT_MULTI: {
            const MultiNode* multi= (MultiNode*)node;
            print_children((Vector*)&multi->multis, prefixes, print_child_);
            break;
        }
        case NT_RULE_IF:
        case NT_RULE_WHEN: {
            const RuleNodeIf* rule= (RuleNodeIf*)node;
            print_titled_child(rule->condition, prefixes, false, "Condition");
            print_titled_child(rule->output.base, prefixes, true, "Output");
            break;
        }
        case NT_RULE_L: {
            const RuleNodeL* rule= (RuleNodeL*)node;
            print_children_arr((Array*)&rule->rules, prefixes, print_child_ptr_);
            break;
        }
        case NT_RULE_LR: {
            const RuleNodeLR* rule= (RuleNodeLR*)node;
            PrintAsChild(
                printf("Left:\n");
                print_children_arr((Array*)&rule->left, prefixes, print_child_ptr_);
            )

            PrintAsLastChild(
                printf("Right:\n");
                print_child(rule->right.base, prefixes, true);
            )
            break;
        }
        case NT_IDENT: {
            const IdentNode* ident= (IdentNode*)node;
            PPC printf("Name: `%s`\n", ident->name);
            PrintAsLastChild(
                printf("Link: ");
                switch (ident->link->type) {
                    case NT_ALIAS: printf("alias"); break;
                    case NT_DATA: printf("data"); break;
                    case NT_FLAG: printf("flag"); break;
                    case NT_FLAG_VALUE: printf("flag value"); break;
                    case NT_DATA_FIELD: printf("data field"); break;
                    case NT_VAR: printf("var"); break;
                    default: assert(false);
                }
            )
            newline();
            break;
        }
        case NT_FIELD: {
            const FieldNode* field= (FieldNode*)node;
            print_field_node(field);
            newline();

            break;
        }
        case NT_EXPR: {
            const ExprNode* expr= (ExprNode*)node;
            PPC printf("Type: %s\n", type_to_str(expr->type.base));
            PPC printf("Size: %hu\n", expr->type.size);
            print_child(expr->expr, prefixes, true);
            break;
        }
        case NT_BIN_EXPR: {
            const BinNode* bin= (BinNode*)node;
            PPC printf("Op: %s\n", BINARY_OP_STRINGS[bin->op]);
            print_child(bin->left.node, prefixes, false);
            print_child(bin->right.node, prefixes, true);
            break;
        }
        case NT_UNARY_EXPR: {
            const UnaryNode* unary= (UnaryNode*)node;
            PPC printf("Op: %s\n", UNARY_OP_STRINGS[unary->op]);
            print_child(unary->operand.node, prefixes, true);
            break;
        }
        case NT_LIT_STRING: {
            const LitNode* lit= (LitNode*)node;
            const LitStringData* str= &lit->data.lit_string;

            PPC printf("Raw string: `%s`\n", str->string);
            if (str->expressions.pos == 0) {
                PrintAsLastChild(printf("Id: %u\n", str->id);)
            } else {
                PrintAsChild(printf("Id: %u\n", str->id);)
                print_children((Vector*)&str->expressions, prefixes, print_child_);
            }

            break;
        }
        case NT_LIT_NUM: {
            const LitNode* lit= (LitNode*)node;
            const SimpleNumData* num= &lit->data.lit_number;
            PrintAsLastChild(print_simple_num(num);)
            newline();
            break;
        }
        case NT_RULE_WITH: {
            const RuleNodeWith* with= (RuleNodeWith*)node;
            PrintAsChild(
                printf("Assignments:\n");
                print_children(&with->assignNodes, prefixes, print_child_);
            )
            PrintAsLastChild(
                printf("Brace:\n");
                print_child((Node*)with->brace, prefixes, true);
            )
            break;
        }
        case NT_ASSIGN: {
            const AssignNode* assign= (AssignNode*)node;
            PrintAsChild(
                printf("Left: ");
                print_child((Node*)assign->left, prefixes, false);
            )
            PrintAsLastChild(
                printf("Right: ");
                print_child((Node*)assign->right, prefixes, true);
            )
            break;
        }
        case NT_VAR: {
            const VarNode* var= (VarNode*)node;
            PPC printf("Name: %s\n", var->identifier);
            PPC printf("Link: %s (%s)\n", link_name(var->link), types_to_string(var->link->type));
            PrintAsLastChild(
                printf("Value:\n");
                print_child((Node*)var->value, prefixes, true);
            )
            break;
        }
        default: assert(false);
    }
}

void print_symbol_table() {
    printf("SYM TABLE (%zu entries):\n", symbol_table.pos);
    for (size_t i = 0; i < symbol_table.pos; i++) {
        const Symbol* sym= Symbol_arr_ptr(&symbol_table, i);
        printf("\t- %s LINK: %p (%s)\n", sym->name, sym->node, types_to_string(sym->node->type));
    }
    printf("--SYM END--\n\n");
}
