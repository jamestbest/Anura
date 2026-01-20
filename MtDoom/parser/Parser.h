//
// Created by jamestbest on 11/24/25.
//

#ifndef ANURA_PARSER_H
#define ANURA_PARSER_H

#include "shared/Vector.h"
#include "MtDoom/lexer/Lexer.h"

typedef enum NodeType {
    NT_ROOT,
    NT_ALIAS_OR_STMT,
    NT_ALIAS_DESC_STMT,
    NT_ALIAS_RULES_STMT,
    NT_STRUCTURE_STMT,
    NT_STATEMENTS,
    NT_FLAG_STATEMENT,

    NT_IDENT,
    NT_FIELD,

    NT_LIT_STRING,
    NT_LIT_NUM,
} NodeType;

typedef struct Node {
    NodeType type;
} Node;

VECTOR_PROTO(Node, Node)
VECTOR_ADD(Node, Node)

#define COMMON_NODE Node base;

typedef struct RootNode {
    COMMON_NODE
    Vector child_nodes;
} RootNode;

typedef struct IdentNode {
    COMMON_NODE
    // something here for scope linking
    Node* alias_link;
    Token* token;
} IdentNode;

VECTOR_PROTO(IdentNode, IdentNode)

typedef struct Alias {
    COMMON_NODE
    const char* identifier;
    uint32_t bits;
} Alias;

#define COMMON_ALIAS Alias a_base;

// An alias that just represetents some literal data
typedef struct AliasDataNode {
    COMMON_ALIAS
} AliasDataNode;

typedef struct AliasMirrorNode {
    COMMON_ALIAS
    struct {
        const char* mirror_name;
        Node* mirror_node;
    };
};

typedef struct FieldNode {
    COMMON_NODE
    bool named;
    union {
        struct NamedFieldInfo {
            const char* name;
            uint8_t bits;
        } named_info;
        struct LitNumData num;
    };
} FieldNode;

VECTOR_PROTO(FieldNode, FieldNode)

typedef struct AliasDescNode {
    COMMON_NODE
    FieldNodeVector fields;
} AliasDescNode;

typedef struct RuleNodeIf {
    COMMON_NODE
    Node* condition;
    Node* output;
} RuleNodeIf;

typedef struct RuleNodeWhen {
    COMMON_NODE
    Node* condition;
    Node* output;
} RuleNodeWhen;

typedef struct LeftRule {

} LeftRule;

typedef struct LeftRules {
    // left rules are always a list of literals or aliases

} LeftRule;

typedef struct RuleNodeLR {
    COMMON_NODE
    LeftRule left;
    RightRule right;
} RuleNodeLR;

typedef struct RuleNodeL {
    COMMON_NODE
    LeftRule left;
} RuleNodeL;

typedef union Rule {
    RuleNodeIf rule_if;
    RuleNodeWhen rule_when;
    RuleNodeLR rule_lr;
    RuleNodeL rule_l;
} Rule;

VECTOR_PROTO(Rule, Rule)
VECTOR_ADD(Rule, Rule)

/*
 * This is the = {} and contains
 * a list of rules, seperated on new lines
 */
typedef struct BracedRules {
    RuleVector rules;
} BracedRules;

typedef struct AliasOrNode {
    COMMON_ALIAS
    BracedRules rules;
} AliasOrNode;

typedef struct ParseRet {
    bool succ;
    RootNode root;
} ParseRet;

// a string literal can contains {}
//  which is an expression
// e.g. "{sibsi} + {disp32 * 2} + [{EBP}]"
typedef struct LitStringData {
    const char* string;
    NodeVector expressions;
} LitStringData;

typedef struct LitNode {
    COMMON_NODE

    union LitData {
        const LitStringData lit_string;
        struct LitNumData lit_number;
    } data;
    Token* token;
} LitNode;

typedef struct BinNode {
    COMMON_NODE

    Node* left;
    Node* right;

    BinaryOperator op;
} BinNode;

typedef union ExprNode {
    IdentNode ident;
    LitNode lit;
} ExprNode;

Token* current();
Token* consume();

LitNode* add_lit_node(NodeType lit_type);

ParseRet parse(TokenArray* tokens);

#endif // ANURA_PARSER_H
