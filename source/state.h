/**
  @file state.h
  @brief TODO: Describe this file
  $Revision$
  $HeadURL$
  */
#ifndef STATE_H
#define STATE_H

#include <stdbool.h>
#include "list.h"

#include "frame.h"
#include "workdir.h"

bool state_get_running(void);
void state_set_running(bool val);
bool state_get_aardvark_mode(void);
void state_set_aardvark_mode(bool val);
void state_set_focused_node(list_node_t* p_node);
list_node_t* state_get_focused_node(void);
Frame_T* state_get_focused_frame(void);
WorkDir_T* state_get_focused_workdir(void);

typedef enum {
    REFRESH_COMPLETE,
    REFRESH_CURR_WIN,
    REFRESH_ALL_WINS,
    REFRESH_AARDVARK,
} RefreshState_T;

RefreshState_T state_get_refresh_state(void);
void state_set_refresh_state(RefreshState_T state);

typedef enum {
    MODE_NORMAL,
    MODE_SEARCH
} Mode_T;

Mode_T state_get_mode(void);
void state_set_mode(Mode_T mode);

#endif /* STATE_H */
