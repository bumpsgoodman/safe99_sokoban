//***************************************************************************
// 
// 파일: key_event.c
// 
// 설명: 키보드/마우스 이벤트
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/10/18
// 
//***************************************************************************

#include "precompiled.h"

#include "key_event.h"

// 키보드
static keyboard_state_t s_key_states[256];

// 마우스
static mouse_state_t s_left_state;
static mouse_state_t s_right_state;

static int s_x;
static int s_y;

void key_down(const uint8_t key)
{
    if (s_key_states[key] == KEYBOARD_STATE_DOWN || s_key_states[key] == KEYBOARD_STATE_PRESSED)
    {
        s_key_states[key] = KEYBOARD_STATE_PRESSED;
    }
    else
    {
        s_key_states[key] = KEYBOARD_STATE_DOWN;
    }
}

void key_up(const uint8_t key)
{
    s_key_states[key] = KEYBOARD_STATE_UP;
}

void reset_keyboard(void)
{
    memset(s_key_states, KEYBOARD_STATE_UP, sizeof(keyboard_state_t) * 256);
}

keyboard_state_t get_key_state(const uint8_t key)
{
    return s_key_states[key];
}

void move_mouse(const int x, const int y)
{
    s_x = x;
    s_y = y;
}

void down_left_mouse(void)
{
    if (s_left_state == MOUSE_STATE_DOWN || s_left_state == MOUSE_STATE_PRESSED)
    {
        s_left_state = MOUSE_STATE_PRESSED;
    }
    else
    {
        s_left_state = MOUSE_STATE_DOWN;
    }
}

void up_left_mouse(void)
{
    s_left_state = MOUSE_STATE_UP;
}

void down_right_mouse(void)
{
    if (s_right_state == MOUSE_STATE_DOWN || s_right_state == MOUSE_STATE_PRESSED)
    {
        s_right_state = MOUSE_STATE_PRESSED;
    }
    else
    {
        s_right_state = MOUSE_STATE_DOWN;
    }
}

void up_right_mouse(void)
{
    s_right_state = MOUSE_STATE_UP;
}

void release_mouse(void)
{
    s_left_state = MOUSE_STATE_UP;
    s_right_state = MOUSE_STATE_UP;
}

int get_mouse_x(void)
{
    return s_x;
}

int get_mouse_y(void)
{
    return s_y;
}

mouse_state_t get_left_mouse_state(void)
{
    return s_left_state;
}

mouse_state_t get_right_mouse_state(void)
{
    return s_right_state;
}