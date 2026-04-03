//
// Created by James Coward on 11/24/25.
//

#ifndef ANURA_PARSER_H
#define ANURA_PARSER_H

#include "shared.h"
#include "shared/Vector.h"
#include "MtDoom/lexer/Lexer.h"

typedef enum NodeType {
    NT_ROOT,
    NT_META,
    NT_ALIAS,
    NT_DATA,
    NT_DATA_FIELD,
    NT_STRUCTURE,
    NT_RULE_RIGHT_STMT, // BAD NAME WHY NAME IT RULE WHEN RULES EXIST?!
    NT_STATEMENTS,
    NT_FLAG,
    NT_FLAG_VALUE,
    NT_CALCULATE,

    NT_BRACED_RULES,
    NT_IF_BRACED_RULES,

    NT_MULTI,

    NT_MAP,
    NT_VAR,

    NT_RULE_WITH,
    NT_ASSIGN,

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
    ENDIANNESS endianness;
} MetaData;

struct StructureNode;

typedef struct MetaNode {
    COMMON_NODE
    MetaData meta;
} MetaNode;

typedef struct RootNode {
    COMMON_NODE
    Vector child_nodes;
    MetaNode* meta;
    struct StructureNode* structure;
    Vector strings;
    Vector withs;
} RootNode;

typedef struct IdentNode {
    COMMON_NODE
    // this could be a flag
    // an alias
    // or data
    Node* link;
    const char* name;
    bool inverted;
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

typedef struct DataFieldNode {
    COMMON_NODE
    struct DataNode* data;
    size_t pos;
} DataFieldNode;

VECTOR_PROTO(FieldNode, FieldNode)
ARRAY_PROTO(FieldNodeVector, FieldNodeVector)

typedef struct DataNode {
    COMMON_NODE
    const char* name;
    const char* capitalised;
    FieldNodeVector all_fields;
    FieldNodeVectorArray rows;
    size_t bits;
    bool non_fielded;
} DataNode;

typedef struct FlagNode {
    COMMON_NODE
    const char* name;
    const char* capitalised;
    Vector enum_values;
    size_t default_value;
    struct CalcNode* linked_calc;
} FlagNode;

typedef struct FlagLinkPos {
    FlagNode* flag;
    size_t enum_pos;
} FlagLinkPos;

typedef enum TYPE {
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_ENUM,
    TYPE_ALIAS,
    TYPE_ERROR
} TYPE;

typedef struct TypeInfo {
    TYPE base;
    uint16_t size;
} TypeInfo;

typedef struct ExprNode {
    COMMON_NODE
    Node* expr;
    TypeInfo type;
} ExprNode;

ARRAY_PROTO(FlagLinkPos, FlagLinkPos)

typedef struct FlagValueNode {
    COMMON_NODE
    FlagLinkPosArray links;
    const char* name;
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

ARRAY_PROTO(LeftRule, LeftRule)

typedef LeftRuleArray LeftRules;

struct BracedRules;
struct IfBracedRules;

typedef struct MultiNode {
    COMMON_NODE
    NodeVector multis;
} MultiNode;

typedef struct MapNode {
    COMMON_NODE
    struct AliasNode* destination;
    NodeVector stream;
} MapNode;

typedef union RightRule {
    Node* base;
    struct BracedRules* brace;
    MultiNode* multi_out;
    Node* single_out;
    MapNode* map;
    struct IfBracedRules* ifbrace;
} RightRule;

typedef struct StructureNode {
    COMMON_NODE
    MarkedIdentArray rules;
    RightRule output;
} StructureNode;

typedef struct VarNode {
    COMMON_NODE
    const char* identifier;
    Node* link;
    ExprNode* value;
} VarNode;

typedef struct AssignNode {
    COMMON_NODE
    IdentNode* left; // either var node or var node . field
    ExprNode* right; // basic expression
} AssignNode;

typedef struct RuleNodeIf {
    COMMON_NODE
    Node* condition;
    RightRule output;
} RuleNodeIf;

typedef union IfFlagRule {
    Node* base;
    FlagValueNode* flag;
    RuleNodeIf* if_rule;
    struct IfBracedRules* brace;
} IfFlagRule;

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

typedef struct RuleNodeWith {
    COMMON_NODE
    Vector assignNodes;
    struct BracedRules* brace;
    size_t id;
    struct AliasNode* alias;
} RuleNodeWith;

typedef union RuleData {
    RuleNodeIf rule_if;
    RuleNodeWhen rule_when;
    RuleNodeLR rule_lr;
    RuleNodeL rule_l;
    RuleNodeWith rule_with;
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

typedef struct RuleRightNode {
    COMMON_NODE
    uint16_t id;
    NodeVector expressions;
} RuleRightNode;

typedef struct AliasNode {
    COMMON_NODE
    const char* identifier;
    const char* capitalised;
    uint32_t bits;
    BracedRules rules; // if when parsing the braced rules it sets something in the alias for the number of outputs and the rightRule should become RightRules where the comma adds more
    size_t right_output_count;
    RuleRightNode* linked_rule;
    bool is_flat;
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

LitNode* add_lit_node(NodeType lit_type);
LitNode* add_lit_string_node(char* string);
LitNode* add_lit_number_node(LitNumData num);
IdentNode* add_ident_node(const char* name, Node* link);
UnaryNode* add_unary_node(UnaryOperator op, OperandNode operand);
BinNode* add_binary_node(const BinaryOperator op, OperandNode left, OperandNode right);
DataFieldNode* add_data_field_node(DataNode* data, size_t pos);
ExprNode* add_expr_node();

Node* check_link(const char* identifier);
DataFieldNode* data_has_field(DataNode* data, const char* target_name);

ParseRet add_to_symbol_table(const char* name, Node* link);

SimpleNumData complex_to_simple_num(struct LitNumData num, bool context_binary);

void print_simple_num(const SimpleNumData* num);
void fprint_simple_num(FILE* file, const SimpleNumData* num);

const char* types_to_string(NodeType type);
const char* link_name(Node* link);
Node* base_link(const IdentNode* node);

Parsed parse(TokenArray* tokens);
void print_root(RootNode* root);

extern const ParseRet PARSE_RET_FAIL;
extern const ParseRet PARSE_RET_SUCC;

#endif // ANURA_PARSER_H
