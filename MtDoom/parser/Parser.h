//
// Created by jamestbest on 11/24/25.
//

#ifndef ANURA_PARSER_H
#define ANURA_PARSER_H

#include "shared/Vector.h"
#include "MtDoom/lexer/Lexer.h"

typedef enum NodeType {
    NT_ROOT,
    NT_ALIAS,
    NT_DATA,
    NT_DATA_FIELD,
    NT_STRUCTURE,
    NT_STATEMENTS,
    NT_FLAG,
    NT_FLAG_VALUE,
    NT_CALCULATE,

    NT_BRACED_RULES,
    NT_IF_BRACED_RULES,

    NT_RULE_IF,
    NT_RULE_WHEN,
    NT_RULE_L,
    NT_RULE_LR,

    NT_IDENT,
    NT_FIELD,

    NT_EXPR,
    NT_BIN_EXPR,
    NT_UNARY_EXPR,

    NT_LIT_STRING,
    NT_LIT_NUM,
} NodeType;

typedef struct Node {
    NodeType type;
} Node;

typedef struct SimpleNumData {
    uint64_t value;
    uint32_t bits;
    bool show_as_bin;
} SimpleNumData;

// forward decls
struct LitNode;
struct AliasNode;
struct DataNode;

VECTOR_PROTO(Node, Node)

#define COMMON_NODE Node base;

typedef struct MetaData {
    bool parsed;
    const char* name;
} MetaData;

typedef struct RootNode {
    COMMON_NODE
    Vector child_nodes;
    MetaData meta;
} RootNode;

typedef struct IdentNode {
    COMMON_NODE
    // this could be a flag
    // an alias
    // or data
    Node* link;
    Token* token;
} IdentNode;

VECTOR_PROTO(IdentNode, IdentNode)

// a string literal can contains {}
//  which is an expression
// e.g. "{sibsi} + {disp32 * 2} + [{EBP}]"
typedef struct LitStringData {
    const uint16_t id;
    char* string;
    NodeVector expressions;
} LitStringData;

typedef struct LitNode {
    COMMON_NODE

    union LitData {
        LitStringData lit_string;
        SimpleNumData lit_number;
    } data;
    Token* token;
} LitNode;

struct BinNode;
struct UnaryNode;

typedef union OperandNode {
    Node* node;
    LitNode* lit;
    IdentNode* ident;
    struct UnaryNode* unary;
    struct BinNode* bin;
} OperandNode;

typedef struct BinNode {
    COMMON_NODE

    OperandNode left;
    OperandNode right;

    BinaryOperator op;
} BinNode;

typedef struct UnaryNode {
    COMMON_NODE

    OperandNode operand;

    UnaryOperator op;
} UnaryNode;

typedef struct FieldNode {
    COMMON_NODE
    // struct DataNode* data_node;
    union {
        struct NamedFieldInfo {
            const char* name;
            SimpleNumData default_value;
            uint8_t bits;
            bool has_default;
        } named_info;
        SimpleNumData num;
    };
    bool named;
} FieldNode;

VECTOR_PROTO(FieldNode, FieldNode)
ARRAY_PROTO(FieldNodeVector, FieldNodeVector)

typedef struct DataNode {
    COMMON_NODE
    const char* name;
    const char* capitalised;
    FieldNodeVector all_fields;
    FieldNodeVectorArray rows;
    size_t bits;
} DataNode;

typedef struct FlagNode {
    COMMON_NODE
    const char* name;
    const char* capitalised;
    Vector enum_values;
    size_t default_value;
} FlagNode;

typedef struct FlagValueNode {
    COMMON_NODE
    FlagNode* flag;
    size_t enum_pos;
} FlagValueNode;

typedef union LeftRule {
    Node* base;
    IdentNode* ident;
    LitNode* lit;
} LeftRule;

typedef enum MarkedType {
    MARKED_QUESTION,
    MARKED_STAR,
    MARKER_NONE,
} MarkedType;

typedef struct MarkedIdent {
    MarkedType type;
    Node* ident;
} MarkedIdent;

ARRAY_PROTO(MarkedIdent, MarkedIdent)

typedef struct StructureNode {
    COMMON_NODE
    MarkedIdentArray rules;
} StructureNode;

ARRAY_PROTO(LeftRule, LeftRule)

typedef LeftRuleArray LeftRules;

struct BracedRules;
struct IfBracedRules;

typedef union RightRule {
    Node* base;
    struct BracedRules* brace;
    IdentNode* ident;
    LitNode* lit;
} RightRule;

typedef union IfFlagRule {
    Node* base;
    struct FlagValueNode* flag;
    struct IfBracedRules* brace;
} IfFlagRule;

typedef struct RuleNodeIf {
    COMMON_NODE
    Node* condition;
    RightRule output;
} RuleNodeIf;

typedef struct RuleNodeWhen {
    COMMON_NODE
    Node* condition;
    RightRule output;
} RuleNodeWhen;

typedef struct RuleNodeLR {
    COMMON_NODE
    LeftRules left;
    RightRule right;
} RuleNodeLR;

typedef struct RuleNodeL {
    COMMON_NODE
    LeftRules rules;
} RuleNodeL;

typedef union RuleData {
    RuleNodeIf rule_if;
    RuleNodeWhen rule_when;
    RuleNodeLR rule_lr;
    RuleNodeL rule_l;
} RuleData;

typedef struct Rule {
    COMMON_NODE;
    RuleData data;
} Rule;

ARRAY_PROTO(Rule, Rule)

ARRAY_PROTO(IfFlagRule, IfFlagRule)

/*
 * This is the = {} and contains
 * a list of rules, separated on new lines
 */
typedef struct BracedRules {
    COMMON_NODE;
    RuleArray rules;
} BracedRules;

typedef struct IfBracedRules {
    COMMON_NODE;
    IfFlagRuleArray rules;
} IfBracedRules;

typedef struct AliasNode {
    COMMON_NODE
    const char* identifier;
    const char* capitalised;
    uint32_t bits;
    BracedRules rules;
} AliasNode;

typedef struct CalcNode {
    COMMON_NODE
    const char* identifier;
    const char* capitalised;
    IfBracedRules rules;
} CalcNode;

typedef struct ParseRet {
    bool succ;
    Node* node;
} ParseRet;

typedef struct Parsed {
    bool succ;
    RootNode root;
    uint8_t* node_buffer;
} Parsed;

Token* current();
Token* consume();

LitNode* add_lit_node(NodeType lit_type);
LitNode* add_lit_string_node(char* string);
LitNode* add_lit_number_node(struct LitNumData num, Token* tok);
IdentNode* add_ident_node(Token* tok, Node* link);
UnaryNode* add_unary_node(UnaryOperator op, OperandNode operand);
BinNode* add_binary_node(const BinaryOperator op, OperandNode left, OperandNode right);

Node* check_link(const char* identifier);

ParseRet add_to_symbol_table(const char* name, Node* link);

SimpleNumData complex_to_simple_num(struct LitNumData num, bool context_binary);

void fprint_simple_num(FILE* file, const SimpleNumData* num);

Parsed parse(TokenArray* tokens);

extern const ParseRet PARSE_RET_FAIL;
extern const ParseRet PARSE_RET_SUCC;

#endif // ANURA_PARSER_H
