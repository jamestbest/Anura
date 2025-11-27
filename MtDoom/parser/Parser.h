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

    NT_STRING
} NodeType;

typedef struct Node {
    NodeType type;
} Node;

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

typedef struct AliasOrNode {
    COMMON_NODE
    IdentNodeVector choices;
} AliasOrNode;

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

typedef struct LeftRule {
    
} LeftRule;

typedef struct RuleNode {
    COMMON_NODE
    LeftRule left;
//    RightRule right;
} RuleNode;

VECTOR_PROTO(RuleNode, RuleNode)

typedef struct AliasRulesNode {
    COMMON_NODE
    RuleNodeVector rules;
};

typedef struct ParseRet {
    bool succ;
    RootNode root;
} ParseRet;

ParseRet parse(TokenArray* tokens);

#endif // ANURA_PARSER_H
