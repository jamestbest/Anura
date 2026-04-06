//
// Created by jamestbest on 2/16/26.
//

typedef enum TokenType {
    TYPE_X,
    TYPE_Y,
    TYPE_Z,
} TokenType;

typedef union TokenData {
    unsigned int idx;
    const char* identifier;
} TokenData;

typedef struct Token {
    TokenType type;
    TokenData data;
} Token;

#define DUMMY_X ((Token){.type= TYPE_X, .data.idx=0})
#define DUMMY_Y ((Token){.type= TYPE_Y, .data.idx=10})
#define DUMMY_Z ((Token){.type= TYPE_Z, .data.identifier="IAmIdent"})

static int parse();
static int parse_top_level();
static int parse_x();
static int parse_y();
static int parse_z();
static Token* current();
static Token* consume();
static Token* expect(TokenType type);
static int error(const char* message);

int main() {
    return parse();
}

// X Y Y
// Y Z X X
// Z Y Z Z
Token tokens[]= {
    DUMMY_X,
    DUMMY_Y,
    DUMMY_Y,
    DUMMY_Y,
    DUMMY_Z,
    DUMMY_Y,
    DUMMY_X,
    DUMMY_Z,
    DUMMY_Y,
    DUMMY_X
};
unsigned long t_idx;
unsigned long max_t_idx;

#define SUCCESS 0
#define FAIL (-1)

int parse() {
    t_idx= 0;
    max_t_idx= sizeof(tokens)/sizeof(Token) - 1;

    while (t_idx <= max_t_idx) {
        const int res= parse_top_level();
        if (res == FAIL) return res;
    }

    return SUCCESS;
}

int parse_top_level() {
    const Token* c= current();
    if (!c) return FAIL;

    switch (c->type) {
        case TYPE_X: return parse_x();
        case TYPE_Y: return parse_y();
        case TYPE_Z: return parse_z();
    }

    return FAIL;
}

int parse_x_prime() {
    Token* y= expect(TYPE_Y);
    if (!y) return error("Where is my Y!");

    if (!expect(TYPE_Y)) return FAIL;

    return SUCCESS;
}

int parse_x() {
    consume(); // eat the `X`

    const int prime_res= parse_x_prime();
    if (prime_res != SUCCESS) return prime_res;

    return SUCCESS;
}

int parse_y() {
    Token* y= consume();

    if (!expect(TYPE_Z)) return FAIL;
    Token* x= expect(TYPE_X);
    if (!x) return error("");

    x->data.idx= y->data.idx;

    return !expect(TYPE_X);
}

int parse_z() {
    consume();

    Token* y= expect(TYPE_Y);
    if (!y) return error("What?!");

    Token* z= expect(TYPE_Z);
    if (!z) return FAIL;

    return !expect(TYPE_Z);
}

int error(const char* message) {
    return FAIL;
}

Token* expect(const TokenType type) {
    Token* c= current();
    if (!c) return 0;

    if (current()->type != type) return 0;

    return consume();
}

Token* consume() {
    if (t_idx > max_t_idx) return 0;
    return &tokens[t_idx++];
}

Token* current() {
    if (t_idx > max_t_idx) return 0;
    return &tokens[t_idx];
}
