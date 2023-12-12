//***************************************************************************
// 
// 파일: game.h
// 
// 설명: game
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/09/12
// 
//***************************************************************************

#ifndef GAME_H
#define GAME_H

#include "safe99_common/defines.h"

typedef enum object
{
    OBJECT_SPACE =   0,
    OBJECT_WALL =    (1 << 0),
    OBJECT_PLAYER =  (1 << 1),
    OBJECT_BOX =     (1 << 2),
    OBJECT_GOAL =    (1 << 3),
    OBJECT_UNKNOWN = (1 << 4)
} object_t;

typedef struct game
{
    // DLL 핸들
    HINSTANCE h_renderer_dll;
    HINSTANCE h_file_system_dll;

    // DLL 인스턴스
    i_soft_renderer_t* p_renderer;
    i_file_system_t* p_file_system;

    // 메시
    i_mesh_t* p_wall_mesh;
    i_mesh_t* p_player_mesh;
    i_mesh_t* p_box_mesh;
    i_mesh_t* p_goal_mesh;
    i_mesh_t* p_player_goal_mesh;
    i_mesh_t* p_box_goal_mesh;

    // 게임 프레임
    bool b_running;
    
    timer_t frame_timer;
    float delta_time;

    // 게임
    object_t* pa_objects;

    size_t map_width;
    size_t map_height;

    int player_x;
    int player_y;

    size_t num_box;
    size_t num_max_box;

    size_t moves;

    // 트랜스폼
    transform3_t transform;

    // 카메라
    camera3_t camera;
    float min_fov;
    float max_fov;
} game_t;

game_t* gp_game;

bool init_game(void);
void shutdown_game(void);
void tick_game(void);

FORCEINLINE bool is_running_game(void)
{
    return gp_game->b_running;
}

FORCEINLINE void update_window_size_game(void)
{
    gp_game->p_renderer->vtbl->update_window_size(gp_game->p_renderer);
}

#endif // GAME_H