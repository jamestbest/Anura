//
// Created by jamescoward on 11/11/2023.
//

#include "Helper_String.h"

#include <ctype.h>
#include <malloc.h>
#include <string.h>

LenSize len_basic(const char *string, const int from, const int offset, const char delimiter) {
    int i = from;
    int length = offset;
    while (string[i] != '\0' && string[i] != delimiter) {
        if ((string[i] & 0xC0) != 0x80) length++;
        i++;
    }

    return (LenSize){length, i};
}

uint len(const char* string) {
    return len_basic(string, 0, 0, '\0').len;
}

uint len_with(const char* string, const uint offset) {
    return len_basic(string, 0, offset, '\0').len;
}

uint len_from(const char* string, const uint offset) {
    return len_basic(string, offset, 0, '\0').len;
}

uint len_from_to(const char* string, const uint offset, const char to) {
    return len_basic(string, offset, 0, to).len;
}

LenSize len_size(const char* string) {
    return len_basic(string, 0, 0, '\0');
}

bool str_eq(const char* stra, const char* strb) {
    int i = 0;
    while (stra[i] != '\0' && strb[i] != '\0') {
        if (stra[i] != strb[i]) return false;
        i++;
    }

    return stra[i] == strb[i]; //have both strings ended?
}

char char_lower_unsafe(const char a) {
    return (char)(a | 32);
}

char char_lower(const char a) {
    if (is_alph(a)) {
        return char_lower_unsafe(a);
    }
    return a;
}

//returns -1 for false, otherwise it returns the position in the string that it ended at (len(pattern))
int starts_with(const char* string, const char* pattern) {
    //does the string start with pattern
    int i = 0;

    while (string[i] != '\0' && pattern[i] != '\0') {
        if (string[i] != pattern[i]) return -1;

        i++;
    }

    if (pattern[i] == '\0') return i;

    return -1;
}

int word_match_alphnumeric(const char* word, const char* match) {
    int i = 0;
    while (word[i] != '\0' && match[i] != '\0') {
        if (word[i] != match[i]) return -1;

        i++;
    }

    if (match[i] == '\0' && word[i] != '\0' && is_alph_numeric(word[i])) return -1;

    return i;
}

//ignore case
int starts_with_ic(const char* string, const char* pattern) {
    int i = 0;

    while (string[i] != '\0' && pattern[i] != '\0') {
        if (char_lower(string[i]) != char_lower(pattern[i])) return -1;

        i++;
    }

    if (pattern[i] == '\0') return i;

    return -1;
}

//ignore preceding space
/*
 * RETURNS -1 for string doesn't start with pattern
 * RETURNS position in string where the pattern ends
 */
int starts_with_ips(const char* string, const char* pattern) {
    bool is = true; //ignore space

    uint lp = len(pattern);
    uint ls = len(string);

    if (lp > ls) return -1;

    uint string_p = 0; //string pointer
    for (uint i = 0; i < lp;) {
        if (string_p > ls) return -1;
        if (is) {
            if (string[string_p] == SPACE_C) {
                string_p++;
            } else {
                is = false;
            }
        } else {
            if (string[string_p] != pattern[i]) return -1;
            else string_p++;
            i++;
        }
    }
    return string_p;
}

bool str_contains(const char* str, uint from, uint to, char c) {
    for (uint i = from; i < to; i++) {
        if (str[i] == '\0') return false;
        if (str[i] == c) return true;
    }
    return false;
}

int find_last(const char* string, const char pattern) {
    int out = -1;

    uint ls = len(string);

    for (uint i = 0; i < ls; i++) {
        if (string[i] == pattern) out = i;
    }

    return out;
}

char* str_cpy(const char* string) {
    uint length = len(string);

    char* out = malloc((length + 1) * sizeof(char));

    if (out == NULL) return NULL;

    memcpy(out, string, length);

    out[length] = '\0';

    return out;
}

char* str_cpy_replace(const char* string, char find, char replace) {
    uint length = len(string);

    char* out = malloc((length + 1) * sizeof(char));

    if (out == NULL) return NULL;

    for (uint i = 0; i < length; i++) {
        if (string[i] == find) out[i] = replace;
        else out[i] = string[i];
    }

    out[length] = '\0';

    return out;
}

bool is_digit(const uint32_t a) {
    return ((uint32_t)(a - ASCII_DIGIT_MIN)) < NUM_DIGITS;
}

bool is_digit_base(const uint32_t a, const uint base) {
    if (base == 0) return false;
    if (base <= 10) {
        return ((uint32_t)(a - ASCII_DIGIT_MIN)) < base;
    }

    if (is_digit(a)) return true;

    uint alph_count = base - 10;
    return ((uint32_t)(a - ASCII_ALPH_CAP_MIN)) < alph_count ||
            ((uint32_t)(a - ASCII_ALPH_LOW_MIN)) < alph_count;
}

bool is_alph_cap(uint32_t a) {
    return ((uint32_t)(a - ASCII_ALPH_CAP_MIN)) < NUM_ALPH;
}

bool is_alph_low(uint32_t a) {
    return ((uint32_t)(a - ASCII_ALPH_LOW_MIN)) < NUM_ALPH;
}

bool is_alph(const uint32_t a) {
    return is_alph_low(a) || is_alph_cap(a);
}

bool is_alph_numeric(const uint32_t a) {
    return is_alph(a) || is_digit(a);
}

bool is_whitespace(const uint32_t a) {
    return a == ' ' || a == '\t';
}

bool is_newline(const uint32_t a) {
    return a == '\n';
}

void fputz(FILE* file, const char* string) {
    fputs(string, file);
}

void putz(const char* string) {
    fputs(string, stdout);
}

UTF8Pos getutf8(const char* string) {
    const unsigned char fbyte = *string;
    uint32_t ret = fbyte;

    uint inc = 1;

    if (fbyte < 0x80) {
        //1 byte character
    }
    else if ((fbyte & 0xE0) == 0xC0) {
        ret = ((fbyte & 0x1F) << 6)             |
                (string[1] & 0x3f);
        inc = 2;
    }
    else if ((fbyte & 0xf0) == 0xe0) {
        ret = ((fbyte & 0x0f) << 12)            |
                ((string[1] & 0x3f) << 6)        |
                (string[2] & 0x3f);
        inc = 3;
    }
    else if ((fbyte & 0xf8) == 0xf0 && (fbyte <= 0xf4)) {
        ret = ((fbyte & 0x07) << 18)            |
                ((string[1] & 0x3f) << 12)       |
                ((string[2] & 0x3f) << 6)        |
                ((string[3] & 0x3f));
        inc = 4;
    }
    else {
        ret = -1; //allow it to skip the byte if consume();
    }

    return (UTF8Pos){ret, inc};
}

uint pututf8(const char* string) {
    const UTF8Pos info = getutf8(string);

    if (info.value != (uint)-1) printf("%lc", info.value);

    return info.bytes;
}

// remove those pesky `\`
void putz_santitize(char* string) {
    if (!string) {
        putz("<<NULL>>");
        return;
    }

    uint pos = 0;
    const char* start_point = string;
    const char* buff;

    while (string[pos] != '\0') {
        switch (string[pos]) {
            case '\n':
                buff = "\\n";
                break;
            case '\r':
                buff = "\\r";
                break;
            case '\t':
                buff = "\\t";
                break;
            default:
                pos++;
                continue;
        }
        const char save = string[pos];
        string[pos] = '\0';
        printf("%s%s", start_point, buff);
        string[pos] = save;

        start_point = &string[++pos];
    }

    printf("%s", start_point);
}

void newline() {
    putchar('\n');
}

void fnewline(FILE* f) {
    fputc('\n', f);
}

char* remove_ws_prefix(char* string) {
    uint pos = 0;
    while (is_whitespace(string[pos++])){}
    return &string[pos];
}

char* str_cat_dyn(const char* stra, const char* strb) {
    const size_t l1 = strlen(stra);
    const size_t l2 = strlen(strb);

    char* res = malloc((l1 + l2 + 1) * sizeof (char));

    if (!res) {
        return NULL;
    }

    memcpy(res, stra, l1);
    memcpy(res + l1, strb, l2);
    res[l1 + l2] = '\0';

    return res;
}

size_t fputs_upper(FILE* stream, const char* string) {
    char* s= string;
    while (*s) {
        putc(toupper(*s), stream);
        s++;
    }

    return s - string;
}
