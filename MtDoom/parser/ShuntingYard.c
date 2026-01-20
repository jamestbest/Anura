//
// Created by james on 26/12/25.
//

#include "ShuntingYard.h"

VECTOR_PROTO(Token, Token)
VECTOR_ADD(Token, Token)

NodeVector output_queue;
TokenVector operator_stack;

extern size_t t_i;
extern TokenArray* tokens;

Node* shunt() {
    output_queue= Node_vec_create();
    operator_stack= Token_vec_create();

    while (true) {
        Token* t= consume();

        switch (t->type) {
            case LIT_NUM: {
                LitNode* lit= add_lit_node(NT_LIT_NUM);

                lit->data.lit_number= t->data.lit_num;
                lit->token= t;

                Node_vec_add(&output_queue, (Node*)lit);
                break;
            }

            case LIT_STRING: {
                LitNode* lit= add_lit_node(NT_LIT_STRING);

                lit->data.lit_string= (LitStringData){
                    .string= t->data.lit_string,
                    .expressions= Node_vec_create()
                };
                lit->token= t;

                Node_vec_add(&output_queue, (Node*)lit);
                break;
            }

            case BINARY_OP: {
                Token* t2;
                while (t2= Token_vec_peek(&operator_stack), t2 != NULL) {
                    if (t2->type == LPAREN) break;

                    int o1_p= PRECEDENCE[t->data.bin_op];
                    int o2_p= PRECEDENCE[t2->data.bin_op];
                    if (!(
                        (o2_p > o1_p) ||
                        ((o2_p == o1_p) && ASSOC[t->data.bin_op] == ASSOC_LEFT)
                    )) break;

                    make_operator();
                }
                Token_vec_add(&operator_stack, t);

                break;
            }

            case LPAREN: {
                Token_vec_add(&operator_stack, t);
                break;
            }

            case RPAREN: {
                Token* t2;
                while (t2= Token_vec_peek(&operator_stack), t2 != NULL && t2->type != LPAREN) {
                    assert(operator_stack.pos != 0);

                    make_operator();
                }

                Token_vec_pop(&operator_stack);
            }
        }
    }

    while (operator_stack.pos) {
        Token* op= Token_vec_pop(&operator_stack);

        assert(op && op->type != LPAREN);

        make_operator();
    }
}
