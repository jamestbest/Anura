//
// Created by jamestbest on 10/17/25.
//

// Palantir is the temporary TUI for Anura
// it is a seperate thread from the main processing
// adds actions to a queue for the main processor to deal with later

#include "Palantir.h"

typedef enum ACTION_GTYPE {
    ACTION_GTYPE_BREAK_POINT,
    ACTION_GTYPE_CONTROL_FLOW,
} ACTION_GTYPE;

typedef enum ACTION_TYPE {
    ACTION_BP_ADD,
    ACTION_BP_REMOVE,
    ACTION_BP_LIST,

    ACTION_CF_SINGLE_STEP,
    ACTION_CF_STEP_OVER,
    ACTION_CF_STEP_INFO,
    ACTION_CF_CONTINUE
} ACTION_TYPE;

typedef union ACTION_DATA {
    struct {
        void* addr;
    } BP_ADD;

    struct {
        void* addr;
    } BP_REMOVE;

    struct {

    } BP_LIST;
} ACTION_DATA;

typedef struct Action {
    ACTION_TYPE type;
    ACTION_DATA data;
} Action;

int setup() {

}