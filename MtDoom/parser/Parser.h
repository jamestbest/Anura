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
    NT_STRUCTURE_STMT,
    NT_STATEMENTS,
    NT_FLAG,

    NT_BRACED_RULES,

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
VECTOR_ADD(Node, Node)

#define COMMON_NODE Node base;

typedef struct RootNode {
    COMMON_NODE
    Vector child_nodes;
} RootNode;

typedef struct IdentNode {
    COMMON_NODE
    // this could be a flag
    // an alias
    // or data
    Node* link;
    Token* token;
} IdentNode;

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

typedef union OperandNode {
    LitNode lit;
    IdentNode ident;
} OperandNode;

typedef struct BinNode {
    COMMON_NODE

    OperandNode* left;
    OperandNode* right;

    BinaryOperator op;
} BinNode;

typedef struct UnaryNode {
    COMMON_NODE

    OperandNode* operand;

    UnaryOperator op;
} UnaryNode;

typedef union ExprNode {
    IdentNode ident;
    LitNode lit;
    BinNode binary;
    UnaryNode unary;
} ExprNode;

typedef struct FieldNode {
    COMMON_NODE
    struct DataNode* data_node;
    union {
        struct NamedFieldInfo {
            const char* name;
            const char* default_value;
            uint8_t bits;
            bool has_default;
        } named_info;
        SimpleNumData num;
    };
    bool named;
} FieldNode;

VECTOR_PROTO(FieldNode, FieldNode)

typedef struct DataNode {
    COMMON_NODE
    const char* name;
    const char* capitalised;
    FieldNodeVector fields;
    size_t bits;
} DataNode;

typedef struct FlagNode {
    COMMON_NODE
    const char* name;
    const char* capitalised;
    Vector enum_values;
    char* default_value;
} FlagNode;

typedef union LeftRule {
    Node* base;
    struct AliasNode* alias;
    struct DataNode* data;
    struct LitNode* lit;
} LeftRule;

ARRAY_PROTO(LeftRule, LeftRule)
ARRAY_ADD(LeftRule, LeftRule)

typedef LeftRuleArray LeftRules;

struct Rule;

VECTOR_PROTO(struct Rule, Rule)
VECTOR_ADD(struct Rule, Rule)

/*
 * This is the = {} and contains
 * a list of rules, separated on new lines
 */
typedef struct BracedRules {
    COMMON_NODE;
    RuleVector rules;
} BracedRules;

typedef union RightRule {
    Node* base;
    BracedRules* brace;
    struct AliasNode* alias;
    struct DataNode* data;
    struct LitNode* lit;
} RightRule;

typedef struct RuleNodeIf {
    COMMON_NODE
    ExprNode* condition;
    RightRule* output;
} RuleNodeIf;

typedef struct RuleNodeWhen {
    COMMON_NODE
    ExprNode* condition;
    RightRule* output;
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

typedef struct AliasNode {
    COMMON_NODE
    const char* identifier;
    const char* capitalised;
    uint32_t bits;
    BracedRules rules;
} AliasNode;

typedef struct ParseRet {
    bool succ;
    RootNode root;
} ParseRet;

Token* current();
Token* consume();

LitNode* add_lit_node(NodeType lit_type);

void fprint_simple_num(FILE* file, const SimpleNumData* num);

ParseRet parse(TokenArray* tokens);

#endif // ANURA_PARSER_H
