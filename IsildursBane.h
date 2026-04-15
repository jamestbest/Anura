//
// Created by james on 30/10/25.
//

#ifndef ISILDURSBANE_H
#define ISILDURSBANE_H

void* control_thread_create(void* data);

typedef enum CONTROL_STATE {
    STATE_NORMAL,
    STATE_STEP_INTO, // we handle SIG TRAPS and send single step until
    STATE_STEP_OVER,
    STATE_STEP_OUT,
    STATE_BREAK_CAUSE,
    STATE_BREAK_SAVE
} CONTROL_STATE;

void change_state(CONTROL_STATE state);

#endif //ISILDURSBANE_H
