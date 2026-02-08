//
// Created by james on 26/12/25.
//

#include "ShuntingYard.h"

#include <string.h>

#include "ShuntingYardInternal.h"

VECTOR_PROTO(Token, Token)
VECTOR_ADD(Token, Token)

NodeVector output_queue;
TokenVector operator_stack;

typedef enum TYPE {
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_ENUM,
    TYPE_ALIAS,
    TYPE_ERROR
} TYPE;

typedef struct TYPE_INFO {
    TYPE type;
    union {
        FlagNode* flag_link;
    };
} TYPE_INFO;

const TYPE result_of_op[BINARY_OP_COUNT]= {
    [EQUALITY]= TYPE_NUMBER,
    [NEQUALITY]= TYPE_NUMBER,
    [DOT]= TYPE_NUMBER,
    [STAR]= TYPE_NUMBER,
    [POW]= TYPE_NUMBER,
    [PIPE]= TYPE_NUMBER,
};

const TYPE result_of_un_op[UNARY_OP_COUNT]= {
    [NOT]= TYPE_NUMBER,
    [EXISTS]= TYPE_NUMBER
};

#define BASE_TYPE(t) ((TYPE_INFO){.type= t})

TYPE_INFO type_of(const OperandNode op) {
    switch (op.node->type) {
        case NT_LIT_NUM: return BASE_TYPE(TYPE_NUMBER);
        case NT_LIT_STRING: return BASE_TYPE(TYPE_STRING);
        case NT_IDENT: {
            const IdentNode* ident= op.ident;
            switch (ident->link->type) {
                case NT_FLAG:
                case NT_FLAG_VALUE:
                    return (TYPE_INFO){.type= TYPE_ENUM, .flag_link= (FlagNode*)ident->link};
                case NT_ALIAS: return (TYPE_INFO){.type= TYPE_ALIAS};
                case NT_DATA: return (TYPE_INFO){.type= TYPE_NUMBER};
                default: assert(false);
            }
        }
        case NT_BIN_EXPR: return BASE_TYPE(result_of_op[op.bin->op]);
        case NT_UNARY_EXPR: return BASE_TYPE(result_of_un_op[op.unary->op]);
        default: assert(false);
    }
}

static bool check_binary_expression(const BinNode* binary);
static bool check_unary_expression(const UnaryNode* unary);

bool check_operand(OperandNode operand) {
    if (operand.node->type == NT_BIN_EXPR) return check_binary_expression(operand.bin);
    if (operand.node->type == NT_UNARY_EXPR) return check_unary_expression(operand.unary);

    if (operand.node->type == NT_IDENT) {
        IdentNode* ident= operand.ident;
        Node* link= check_link(ident->name);
        if (!link) {
            printf("<<ERROR>> Unable to find identifier `%s` in scope at expression\n", ident->name);
            return false;
        }
        ident->link= link;
    }

    return true;
}

bool check_unary_expression(const UnaryNode* unary) {
    if (!check_operand(unary->operand)) return false;

    const TYPE_INFO op_type= type_of(unary->operand);

    switch (unary->op) {
        case NOT:
            // !
            // applied to literal numbers
            return op_type.type == TYPE_NUMBER;

        case EXISTS:
            return op_type.type == TYPE_ALIAS;

        default: assert(false);
    }
}

bool check_binary_expression(const BinNode* binary) {
    if (!check_operand(binary->left)) return false;

    if (binary->op == DOT) {
        if (binary->left.node->type != NT_IDENT || binary->left.ident->link->type != NT_DATA) return false;
        if (binary->right.node->type != NT_IDENT) return false;

        DataNode* data= (DataNode*)binary->left.ident->link;
        IdentNode* ident= binary->right.ident;

        for (int i = 0; i < data->all_fields.pos; ++i) {
            const FieldNode* field= FieldNode_vec_get_unsafe(&data->all_fields, i);

            if (strcmp(field->named_info.name, ident->name) == 0) {
                ident->link= (Node*)add_data_field_node(data, i);
                return true;
            }
        }
        return false;
    }

    if (!check_operand(binary->right)) return false;

    const TYPE_INFO op1= type_of(binary->left);
    const TYPE_INFO op2= type_of(binary->right);

    switch (binary->op) {
        case EQUALITY:
        case NEQUALITY: {
            if (op1.type != op2.type) return false;

            switch (op1.type) {
                case TYPE_NUMBER:
                case TYPE_ENUM:
                    return true;
                case TYPE_ALIAS:
                case TYPE_STRING:
                case TYPE_ERROR:
                    return false;
                default: assert(false);
            }
        }
        case STAR:
        case AND:
        case POW: {
            if (op1.type != op2.type) return false;

            return (op1.type == TYPE_NUMBER);
        }
        case PIPE:
            return false;

        default: assert(false);
    }
}

ParseRet make_unary_op(const Token* tok) {
    OperandNode operand;
    Node* node= Node_vec_pop(&output_queue);
    operand.node= node;

    const UnaryNode* unary= add_unary_node(tok->data.unary_op, operand);

    if (!check_unary_expression(unary)) return PARSE_RET_FAIL;

    return (ParseRet) {
        .succ= true,
        .node= (Node*)unary,
    };
}

ParseRet make_binary_op(const Token* tok) {
    OperandNode operand1, operand2;

    Node* op2= Node_vec_pop(&output_queue);
    Node* op1= Node_vec_pop(&output_queue);

    operand1.node= op1;
    operand2.node= op2;

    const BinNode* bin= add_binary_node(tok->data.bin_op, operand1, operand2);

    if (!check_binary_expression(bin)) return PARSE_RET_FAIL;

    return (ParseRet) {
        .succ= true,
        .node= (Node*)bin,
    };
}

ParseRet make_operator() {
    const Token* op= Token_vec_pop(&operator_stack);

    if (op->type == UNARY_OP) return make_unary_op(op);
    if (op->type == BINARY_OP) return make_binary_op(op);

    assert(false);
}

uint8_t precedence(const Token* token) {
    if (token->type == UNARY_OP) return PRECEDENCE_UN[token->data.unary_op];
    return PRECEDENCE[token->data.bin_op];
}

uint8_t assoc(const Token* token) {
    if (token->type == UNARY_OP) return PRECEDENCE_UN[token->data.unary_op];
    return PRECEDENCE[token->data.bin_op];
}

static TokenArray* tokens;
static size_t idx;

Token* current() {
    if (idx == tokens->pos) return NULL;
    return Token_arr_ptr(tokens, idx);
}

Token* consume() {
    if (idx == tokens->pos) return NULL;
    return Token_arr_ptr(tokens, idx++);
}

ShuntRet shunt(TokenArray* toks, size_t idx_) {
    output_queue= Node_vec_create();
    operator_stack= Token_vec_create();

    tokens= toks;
    idx= idx_;

    bool in_expr= true;
    while (in_expr) {
        // [[todo]]: change to peek
        Token* t= current();

        switch (t->type) {
            case LIT_NUM: {
                consume();

                LitNode* lit= add_lit_number_node(t->data.lit_num);

                lit->data.lit_number= complex_to_simple_num(t->data.lit_num, true);

                Node_vec_add(&output_queue, (Node*)lit);
                break;
            }

            case LIT_STRING: {
                consume();

                LitNode* lit= add_lit_string_node(t->data.lit_string);
                Node_vec_add(&output_queue, (Node*)lit);
                break;
            }

            case IDENTIFIER: {
                consume();

                IdentNode* ident= add_ident_node(t->data.identifier, NULL);
                Node_vec_add(&output_queue, (Node*)ident);
                break;
            }

            case BINARY_OP:
            case UNARY_OP: {
                consume();

                Token* t2;
                while (t2= Token_vec_peek(&operator_stack), t2 != NULL) {
                    if (t2->type == LPAREN) break;

                    const int o1_p= precedence(t);
                    const int o2_p= precedence(t2);
                    if (!(
                        (o2_p > o1_p) ||
                        ((o2_p == o1_p) && assoc(t) == ASSOC_LEFT)
                    )) break;

                    const ParseRet res= make_operator();
                    if (!res.succ) return SHUNT_RET_FAIL;
                    Node_vec_add(&output_queue, res.node);
                }
                Token_vec_add(&operator_stack, t);

                break;
            }

            case LPAREN: {
                consume();

                Token_vec_add(&operator_stack, t);
                break;
            }

            case RPAREN: {
                consume();

                Token* t2;
                while (t2= Token_vec_peek(&operator_stack), t2 != NULL && t2->type != LPAREN) {
                    assert(operator_stack.pos != 0);

                    const ParseRet res= make_operator();
                    if (!res.succ) return SHUNT_RET_FAIL;
                    Node_vec_add(&output_queue, res.node);
                }

                Token_vec_pop(&operator_stack);
                break;
            }

            default:
                in_expr= false;
                break;
        }
    }

    while (operator_stack.pos) {
        const Token* op= Token_vec_peek(&operator_stack);

        assert(op && op->type != LPAREN);

        const ParseRet res= make_operator();
        if (!res.succ) SHUNT_RET_FAIL;
        Node_vec_add(&output_queue, res.node);
    }

    Node* res= Node_vec_pop(&output_queue);
    if (res && res->type == NT_IDENT) {
        if (!check_operand((OperandNode){.ident= (IdentNode*)res})) return SHUNT_RET_FAIL;
    }

    return (ShuntRet) {
        .succ= res != NULL,
        .node= res,
        .idx= idx
    };
}
