//
// Created by jamestbest on 11/24/25.
//

#include "Parser.h"

#include "Buffer.h"
#include "Errors.h"

#include <stdio.h>
#include <string.h>

#include "ShuntingYard.h"

const ParseRet PARSE_RET_FAIL= (ParseRet) {.succ= false, .node= NULL};
const ParseRet PARSE_RET_SUCC= (ParseRet) {.succ= true, .node= NULL};

static RootNode create_root();
static Vector create_children();

static Token* expect(TokenType type);
static Token* expect_keyword(keyword kw);
static Token* expect_binary_op(BinaryOperator op);
static Token* expect_unary_op(UnaryOperator op);
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
static ParseRet parse_flag();
static ParseRet parse_structure();
static ParseRet parse_meta();
static ParseRet parse_braced_rules(BracedRules* results);
static ParseRet parse_braced_if_flags(IfBracedRules* rules, FlagNode* flag);
static ParseRet parse_l_or_lr_rule(RuleArray* rules);

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

static CalcNode create_calc_node(const char* identifier);
static CalcNode* add_calc_node(const char* identifier);
static Rule create_rule(NodeType type);
static RuleNodeIf create_rule_node_if(Node* expr, RightRule output);
static IdentNode create_ident_node(Token* tok, Node* link);
static BracedRules create_braced_rules();
static RuleNodeLR create_rule_lr(LeftRules left, RightRule right);
static RuleNodeL create_rule_l(LeftRules left);
static IfBracedRules create_if_braced_rules();

uint64_t convert_lit_num_to_base10(struct LitNumData lit_num, bool expecting_base10);

VECTOR_ADD(IdentNode, IdentNode)
ARRAY_ADD(LeftRule, LeftRule)
VECTOR_ADD(FieldNode, FieldNode)
ARRAY_ADD(MarkedIdent, MarkedIdent)
VECTOR_ADD(Node, Node)
ARRAY_ADD(Rule, Rule)
ARRAY_ADD(IfFlagRule, IfFlagRule)

TokenArray* tokens;
size_t t_i;

size_t string_id= 0;
Vector string_data;

uint8_t* node_buffer;
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

MetaData meta_data;

Parsed parse(TokenArray* ts) {
    RootNode root= create_root();
    int ret_code= SUCCESS;

    tokens= ts;
    t_i= 0;

    string_id= 0;
    string_data= vector_create();

    symbol_table= Symbol_arr_create();
    meta_data= (MetaData){.name= NULL};

    Symbol_arr_search_ie(&symbol_table, "symbol_a");

    while (t_i < tokens->pos) {
        ParseRet res;
        if (res= parse_toplevel(), res.succ != SUCCESS) {
            ret_code= res.succ;
            vector_add(&root.child_nodes, res.node);
            break;
        }
    }

    if (ret_code != SUCCESS) {
        free(node_buffer);
        return (Parsed){.succ=false, .root= {0}, .node_buffer= NULL};
    }

    return (Parsed) {
        .succ= true,
        .root= root,
        .node_buffer= node_buffer
    };
}

ParseRet parse_toplevel() {
    Token* t= current();

    if (t->type != KEYWORD) {
        return error_with_token("Expected keyword at toplevel", t);
    }

    switch (t->data.keyword) {
        case ALIAS: return parse_alias();
        case CALCULATE: return parse_calc();
        case DATA: return parse_data();
        case FLAG: return parse_flag();
        case STRUCTURE: return parse_structure();
        case META: return parse_meta();
        default:
            return error_with_token("Expected keyword token to be in (ALIAS, CALCULATE, DATA, FLAG, STRUCTURE)",t);
    }
}

ParseRet parse_meta_row(MetaData* md) {
    if (!expect_binary_op(DOT)) return unexpected("Meta statement row starter, expected `.`", current());

    const Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Meta statement row after `.`, expected identifier for meta data field", current());

    if (strcmp(ident->data.identifier, "name") == 0) {
        if (!expect_binary_op(EQUALITY)) return unexpected("Name field of meta data statement, expected `=`", current());

        const Token* name_str= expect(LIT_STRING);
        if (!name_str) return unexpected("Name field of meta data statement after `=`, expected string", current());

        meta_data.name= name_str->data.lit_string;
    }
}

ParseRet parse_meta() {
    consume(); // eat `META`

    if (meta_data.parsed) return error("Meta data statement already exists in file");
    meta_data.parsed= true;

    do {
        parse_meta_row(&meta_data);
        expect(DELIMITER);
    } while (!expect(RBRACE));
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

    StructureNode* structure= add_structure();
    do {
        const ParseRet res= parse_marked_ident(&structure->rules);
        if (!res.succ) return res;
    } while (!expect(DELIMITER));

    return (ParseRet){
        .succ= true,
        .node= (Node*)structure
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

    field->named_info= (struct NamedFieldInfo){
        .name= ident->data.identifier,
        .bits= bits,
    };

    if (expect_binary_op(EQUALITY)) {
        const Token* dvalue= expect(LIT_NUM);

        if (!dvalue) return funexpected("DATA field `%s` default value expects literal number after `=`", current(), ident->data.identifier);

        field->named_info.has_default= true;
        field->named_info.default_value= complex_to_simple_num(dvalue->data.lit_num, true);
    }

    const size_t pos= has_field_data(all_fields, ident->data.identifier);
    if (pos != -1) {
        if (pos < first_row_pos) {
            // this is allowed, but must be the same number of bits and have the same default value
            FieldNode* efield= FieldNode_vec_get_unsafe(fields, pos);

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
    FieldNode_vec_add(all_fields, field);

    return PARSE_RET_SUCC;
}

ParseRet parse_data_row(FieldNodeVector* all_fields, FieldNodeVectorArray* rows, const char* data_name) {
    //   1100 0100 .R .X .B .m(5) .W .v(4) .L .pp(2)
    const size_t first_row_pos= all_fields->pos;
    FieldNodeVector row= FieldNode_vec_create();
    FieldNodeVector_arr_add(rows, row);

    while (!expect(DELIMITER)) {
        ParseRet res= parse_data_field(all_fields, &row, data_name, first_row_pos);

        if (!res.succ) return res;
    }

    return PARSE_RET_SUCC;
}

ParseRet parse_data() {
    consume(); // eat `DATA`

    Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Expected identifier after DATA keyword", current());
    if (!expect_binary_op(EQUALITY)) return unexpected("Expected `=` after identifier in DATA statement", current());
    if (!expect(LBRACE)) return unexpected("Expected `{` after `=` in DATA statement", current());

    DataNode* data= add_data_node(ident->data.identifier);
    expect(DELIMITER);

    while (!expect(RBRACE)) {
        ParseRet res= parse_data_row(&data->all_fields, &data->rows, data->name);
        if (!res.succ) return res;

        expect(DELIMITER);
    }

    return PARSE_RET_SUCC;
}

size_t find_enum_value(Vector* enum_values, const char* enum_name) {
    for (int i = 0; i < enum_values->pos; ++i) {
        const char* cmp= vector_get_unsafe(enum_values, i);
        if (strcmp(cmp, enum_name) == 0) return i;
    }
    return -1;
}

ParseRet parse_flag() {
    consume(); // eat `FLAG`

    const Token* ident= expect(IDENTIFIER);
    if (!ident) return unexpected("Flag statement, expected identifier after keyword", current());

    if (!expect_binary_op(EQUALITY)) return unexpected("Flag statement after identifier, expected `=`", current());

    FlagNode* flag= add_flag_node(ident->data.identifier);

    do {
        const Token* enum_ident= expect(IDENTIFIER);
        if (!enum_ident) return unexpected("Flag enum list, expected identifier at start or after `|`", current());

        vector_add(&flag->enum_values, (void*)enum_ident->data.identifier);
    } while (expect_binary_op(PIPE));

    if (!expect_keyword(DEFAULT)) return unexpected("Flag enum list after enum values, expected `default` keyword with default value", current());
    const Token* default_value= expect(IDENTIFIER);
    if (!default_value) return unexpected("Flag enum list after default keyword, expected identifier for default value", current());

    size_t default_pos= find_enum_value(&flag->enum_values, default_value->data.identifier);
    if (default_pos == -1) return error("Cannot find flag enum value `%s` in flag `%s`", default_value->data.identifier, flag->name);

    flag->default_value= default_pos;

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
    return shunt();
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

    if (!expect_binary_op(EQUALITY)) {
        return unexpected("Expected `=` after calculate identifier", current());
    }

    CalcNode* calc_node= add_calc_node(ident->data.identifier);
    const ParseRet res= parse_braced_if_flags(&calc_node->rules, flag);

    if (!res.succ) return PARSE_RET_FAIL;

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

    IdentNode* ident_node= add_ident_node(ident, sym->node);
    return (ParseRet) {
        .succ= true,
        .node= (Node*)ident_node
    };
}

ParseRet parse_right_rule() {
    // can be
    // struct BracedRules* brace;
    // struct IdentNode* ident;
    // struct LitNode* lit;
    Token* next= peek();
    if (!next) return PARSE_RET_FAIL;

    switch (next->type) {
        case IDENTIFIER: return parse_ident();
        case LBRACE: {
            BracedRules* br= add_braced_rules();
            const ParseRet res= parse_braced_rules(br);
            return (ParseRet) {.succ= res.succ, .node= (Node*)br};
        }
        case LIT_STRING: {
            LitNode* lit= add_lit_string_node(next->data.lit_string);
            return (ParseRet) {
                .succ= true,
                .node= (Node*)lit
            };
        }
            break;
        default: return PARSE_RET_FAIL;
    }
}

RightRule node_to_right_rule(Node* node) {
    RightRule rule;
    rule.base= node;
    return rule;
}

ParseRet parse_if_rule(RuleArray* rules) {
    consume(); // eat the 'if'

    const ParseRet expr= parse_expr();

    if (!expr.succ) return expr;

    if (!expect_keyword(THEN)) {
        return unexpected_keyword("If rule after expression", current()->data.keyword);
    }

    const ParseRet right= parse_right_rule();

    if (!right.succ) return PARSE_RET_FAIL;

    skip(DELIMITER);

    Rule r= create_rule(NT_RULE_IF);
    r.data.rule_if= create_rule_node_if(expr.node, node_to_right_rule(right.node));

    Rule_arr_add(rules, r);

    return PARSE_RET_SUCC;
}

ParseRet parse_if_flag_rule(IfFlagRuleArray* rules, FlagNode* flag) {
    IfFlagRule rule;

    Token* ident= expect(IDENTIFIER);
    if (ident) {
        Symbol* sym= Symbol_arr_search_ie(&symbol_table, ident->data.identifier);

        if (!sym) return error("Unable to find symbol `%s` in CALCULATE statement", ident->data.identifier);
        if (sym->node->type != NT_FLAG_VALUE) return error("Symbol %s is not a flag value in the result of a calculate statement", ident->data.identifier);

        FlagValueNode* fv= (FlagValueNode*)sym->node;
        if (fv->flag != flag) return error("FLAG value `%s` in CALCULATE `%s` is not from the same FLAG but rather from `%s`",
            ident->data.identifier,
            flag->name,
            fv->flag->name
        );

        rule.flag= fv;
        IfFlagRule_arr_add(rules, rule);
        return PARSE_RET_SUCC;
    }

    rule.brace= add_if_braced_rules();
    const ParseRet res= parse_braced_if_flags(rule.brace, flag);

    if (!res.succ) return PARSE_RET_FAIL;

    IfFlagRule_arr_add(rules, rule);

    return PARSE_RET_SUCC;
}

ParseRet parse_braced_if_flag(IfFlagRuleArray* rules, FlagNode* flag) {
    // this can only be an if statement and the output a braced_if_flags or just a flag identifier
    Token* c= expect_keyword(IF);
    if (!c) {
        return unexpected("If/When rule in calculate statement, expected IF/WHEN keyword", current());
    }

    consume(); // eat the 'if'

    const ParseRet expr= parse_expr();

    if (!expr.succ) return PARSE_RET_FAIL;

    if (!expect_keyword(THEN)) {
        return unexpected_keyword("If rule after expression expects THEN keyword", current()->data.keyword);
    }

    const ParseRet right_res= parse_if_flag_rule(rules, flag);
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

ParseRet parse_braced_rule(RuleArray* rules) {
    // Either an
    // L rule
    // or an LR rule
    // or an if rule
    // or a when rule

    Token* c= current();
    if (c->type == KEYWORD) {
        if (c->data.keyword == IF) return parse_if_rule(rules);

        return unexpected_keyword("Braced rule, expected either IF, WHEN, or L/LR rule", current()->data.keyword);
    }

    return parse_l_or_lr_rule(rules);
}

ParseRet parse_left_rules(LeftRules* lr) {
    const Token* next= peek();

    while (true) {
        switch (next->type) {
            case LIT_NUM: {
                Token* num= consume();
                const LeftRule rule= (LeftRule){.lit= add_lit_number_node(num->data.lit_num, num)};
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
                const LeftRule rule= (LeftRule){.ident= add_ident_node(ident, sym->node)};
                LeftRule_arr_add(lr, rule);
                break;
            }
            default:
                break;
        }
        next= peek();
    }

    return PARSE_RET_SUCC;
}

ParseRet parse_l_or_lr_rule(RuleArray* rules) {
    LeftRules lr= LeftRule_arr_create();
    const ParseRet l_rule= parse_left_rules(&lr);

    if (!l_rule.succ) return PARSE_RET_FAIL;

    if (expect_binary_op(EQUALITY)) {
        Rule rule= create_rule(NT_RULE_LR);
        ParseRet r_rule= parse_right_rule();

        rule.data.rule_lr= create_rule_lr(lr, (RightRule){.base= r_rule.node});

        Rule_arr_add(rules, rule);

        return PARSE_RET_SUCC;
    }

    Rule rule= create_rule(NT_RULE_L);
    rule.data.rule_l= create_rule_l(lr);
    Rule_arr_add(rules, rule);

    return PARSE_RET_SUCC;
}

ParseRet parse_braced_rules(BracedRules* results) {
    if (!expect(LBRACE)) {
        return unexpected("Braced rules statement after equality sign", current());
    }

    expect(DELIMITER);

    while (!expect(RBRACE)) {
        ParseRet res;
        if (res= parse_braced_rule(&results->rules), !res.succ) {
            return res;
        }
        expect(DELIMITER);
    }

    expect(DELIMITER);

    if (results->rules.pos == 0) {
        return error("Cannot have an empty braced rules list");
    }

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

ParseRet parse_alias() {
    consume(); // eat the ALIAS keyword

    Token* ident= expect(IDENTIFIER);

    if (!ident) {
        return unexpected("Expected identifier after alias keyword for alias name", current());
    }

    // This could be the size of the alias
    const Token* size= expect(LIT_NUM);
    uint32_t size_in_bits= 0;
    if (size) {
        Token* byte= expect_keyword(BYTES);
        Token* bits= expect_keyword(BITS);

        if (byte && bits) return unexpected("How did we get here, bytes followed by bits?! Don't do that", current());
        if (!byte && !bits) return unexpected("Expected size quantifier `BIT(S)` or `BYTE(S)` following size in alias statement", current());

        size_in_bits= convert_lit_num_to_base10(size->data.lit_num, true);
        if (byte) size_in_bits *= 8;
    }

    if (!expect_binary_op(EQUALITY)) {
        return unexpected("Expected `=` after alias identifier / alias size", current());
    }

    AliasNode* alias= add_alias_node(ident->data.identifier);
    alias->bits= size_in_bits;

    if (expect(LBRACE)) {
        const ParseRet res= parse_braced_rules(&alias->rules);
        if (!res.succ) return PARSE_RET_FAIL;

        goto parse_alias_end;
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

    AliasNode* r_alias= (AliasNode*)symbol->node;
    IdentNode* i= add_ident_node(ident, symbol->node);

    Rule r= {
        .base.type= NT_RULE_LR,
        .data.rule_lr= (RuleNodeLR) {
            .base.type= NT_RULE_LR,
            .left= LeftRule_arr_construct(1),
            .right= (RightRule) {
                .ident= i
            }
        }
    };

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

    Symbol_arr_add_sorted(&symbol_table, (Symbol) {
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

Token* expect(TokenType type) {
    if (current()->type != type) {
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

    return PARSE_RET_FAIL;
}

void create_node_buffer() {
    node_buffer= calloc(1, MIN_NODE_BUFFER_SIZE);
    node_buffer_size= 0;
    node_buffer_capacity= MIN_NODE_BUFFER_SIZE;
}

void resize_node_buffer() {
    void* new_ptr= realloc(node_buffer, node_buffer_capacity << 1);

    if (!new_ptr) {
        error("Unable to realloc node buffer, this may be because of my `<< 1` on full buffer :)");

        node_buffer= NULL; // push the problem down the line :)
        return;
    }

    node_buffer= new_ptr;
    node_buffer_capacity <<= 1;
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
        .rows= FieldNodeVector_arr_create()
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
        .child_nodes= create_children()
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

IdentNode create_ident_node(Token* tok, Node* link) {
    return (IdentNode) {
        .base= {.type= NT_IDENT},
        .link= link,
        .token= tok
    };
}

IdentNode* add_ident_node(Token* tok, Node* link) {
    IdentNode ident_node= create_ident_node(tok, link);
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
        .bits= -1
    };
}

AliasNode* add_alias_node(const char* name) {
    AliasNode alias= create_alias_node(name);
    return add_node(&alias, sizeof(alias));
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

    vector_add(&string_data, &node->data.lit_string);

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

LitNode* add_lit_number_node(struct LitNumData num, Token* tok) {
    SimpleNumData simple= complex_to_simple_num(num, true);

    LitNode lit= (LitNode) {
        .base= {
            .type= NT_LIT_NUM
        },
        .data.lit_number= simple,
        .token= tok
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

Vector create_children() {
    return vector_create();
}

void fprint_simple_num(FILE* file, const SimpleNumData* num) {
    if (num->show_as_bin) {
        fprintf(file, "0b");
        for (int i = 0; i < num->bits; ++i) {
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

