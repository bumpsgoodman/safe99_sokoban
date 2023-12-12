//***************************************************************************
// 
// 파일: game.c
// 
// 설명: game
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/12/11
// 
//***************************************************************************

#include "precompiled.h"
#include "game.h"
#include "key_event.h"

#define PADDING 30

typedef struct vertex
{
    vector3_t positions;
    color_t color;
} vertex_t;

static char s_map1[] = "\
    XXXXX             \n\
    X   X             \n\
    Xo  X             \n\
  XXX  oXXX           \n\
  X  o  o X           \n\
XXX X XXX X     XXXXXX\n\
X   X XXX XXXXXXX  ..X\n\
X o  o             ..X\n\
XXXXX XXXX XpXXXX  ..X\n\
    X      XXX  XXXXXX\n\
    XXXXXXXX          ";

extern HWND g_hwnd;

static void update(const float delta_time);
static void draw(void);

static void print_info(void);

static void create_meshs(void);

static bool load_map(const char* map);
static object_t get_object(const int x, const int y);
static void set_object(const int x, const int y, const object_t object);

static bool is_won(void);

bool init_game(void)
{
    // 콘솔창 커서 숨기기
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = FALSE;
    SetConsoleCursorInfo(console_handle, &info);

    memset(gp_game, 0, sizeof(game_t));

    // DLL 로드
    {
    #if defined(_WIN64)
        gp_game->h_renderer_dll = LoadLibrary(L"safe99_soft_renderer_x64.dll");
        gp_game->h_file_system_dll = LoadLibrary(L"safe99_file_system_x64.dll");
    #else
        gp_game->h_renderer_dll = LoadLibrary(L"safe99_soft_renderer_x86.dll");
        gp_game->h_file_system_dll = LoadLibrary(L"safe99_file_system_x86.dll");
    #endif // _WIN64

        if (gp_game->h_renderer_dll == NULL
            || gp_game->h_file_system_dll == NULL)
        {
            ASSERT(false, "Failed load DLL");
            goto failed_init;
        }

        // 인스턴스 생성 함수
        create_instance_func pf_create_renderer = (create_instance_func)GetProcAddress(gp_game->h_renderer_dll, "create_instance");
        create_instance_func pf_create_file_system = (create_instance_func)GetProcAddress(gp_game->h_file_system_dll, "create_instance");

        // 렌더러 초기화
        pf_create_renderer(&gp_game->p_renderer);
        if (!gp_game->p_renderer->vtbl->initialize(gp_game->p_renderer, g_hwnd, false))
        {
            ASSERT(false, "Failed init renderer");
            goto failed_init;
        }

        // 파일 시스템 초기화
        pf_create_file_system(&gp_game->p_file_system);
        if (!gp_game->p_file_system->vtbl->initialize(gp_game->p_file_system))
        {
            ASSERT(false, "Failed init file system");
            goto failed_init;
        }
    }

    // 메시 초기화
    {
        create_meshs();
    }

    // 게임 프레임 초기화
    {
        gp_game->b_running = true;

        timer_init(&gp_game->frame_timer);
        gp_game->delta_time = 0.0f;
    }

    // 게임 초기화
    {
        gp_game->pa_objects = NULL;
        load_map(s_map1);

        gp_game->moves = 0;
    }

    // 카메라 초기화
    {
        transform3_set_position(&gp_game->camera.transform, (vector3_t){ (float)(PADDING * gp_game->map_width / 2), (float)(PADDING * gp_game->map_height / 2), 500.0f });
        transform3_set_rotation(&gp_game->camera.transform, (vector3_t){ 0.0f, 0.0f, 0.0f });
        transform3_set_scale(&gp_game->camera.transform, (vector3_t){ 1.0f, 1.0f, 1.0f });

        camera3_set_fov(&gp_game->camera, 80.0f);
        camera3_set_near_z(&gp_game->camera, 5.5f);
        camera3_set_far_z(&gp_game->camera, 5000.0f);

        view_port_t vp;
        vp.width = (float)gp_game->p_renderer->vtbl->get_width(gp_game->p_renderer);
        vp.height = (float)gp_game->p_renderer->vtbl->get_height(gp_game->p_renderer);
        vp.top_left_x = 0.0f;
        vp.top_left_y = 0.0f;
        camera3_set_view_port(&gp_game->camera, &vp);

        camera3_set_look_at(&gp_game->camera, (vector3_t){ (float)(PADDING * gp_game->map_width / 2), (float)(PADDING * gp_game->map_height / 2), 0.0f });

        gp_game->min_fov = 60.0f;
        gp_game->max_fov = 100.0f;
    }

    // 렌더러 세팅
    {
        gp_game->p_renderer->vtbl->toggle_backface_culling(gp_game->p_renderer);
        gp_game->p_renderer->vtbl->toggle_wireframe(gp_game->p_renderer);
    }

    return true;

failed_init:
    shutdown_game();
    return false;
}

void shutdown_game(void)
{
    SAFE_FREE(gp_game->pa_objects);

    SAFE_RELEASE(gp_game->p_renderer);
    SAFE_RELEASE(gp_game->p_file_system);
}

void tick_game(void)
{
    static float start_time;

    const float end_time = timer_get_time(&gp_game->frame_timer);
    gp_game->delta_time = end_time - start_time;
    start_time = end_time;

    update(gp_game->delta_time);
    draw();

    print_info();
}

static void update(const float delta_time)
{
    static const float FOV_SPEED = 300.0f;

    int dir_x = 0;
    int dir_y = 0;

    // 키 입력
    {
        if (get_key_state(VK_F1))
        {
            gp_game->p_renderer->vtbl->toggle_wireframe(gp_game->p_renderer);
        }

        if (get_key_state(VK_PRIOR))
        {
            const float fov = camera3_get_fov(&gp_game->camera) + -FOV_SPEED * delta_time;
            camera3_set_fov(&gp_game->camera, clamp(fov, gp_game->min_fov, gp_game->max_fov));
        }

        if (get_key_state(VK_NEXT))
        {
            const float fov = camera3_get_fov(&gp_game->camera) + FOV_SPEED * delta_time;
            camera3_set_fov(&gp_game->camera, clamp(fov, gp_game->min_fov, gp_game->max_fov));
        }

        if (get_key_state(VK_LEFT) == KEYBOARD_STATE_DOWN)
        {
            --dir_x;
        }

        if (get_key_state(VK_RIGHT) == KEYBOARD_STATE_DOWN)
        {
            ++dir_x;
        }

        if (get_key_state(VK_UP) == KEYBOARD_STATE_DOWN)
        {
            --dir_y;
        }

        if (get_key_state(VK_DOWN) == KEYBOARD_STATE_DOWN)
        {
            ++dir_y;
        }

        reset_keyboard();
    }

    const int cur_x = gp_game->player_x;
    const int cur_y = gp_game->player_y;
    const int next_x1 = cur_x + dir_x;
    const int next_y1 = cur_y + dir_y;

    const object_t cur_object = get_object(cur_x, cur_y);
    const object_t next_object1 = get_object(next_x1, next_y1);

    if (next_object1 == OBJECT_SPACE || next_object1 == OBJECT_GOAL)
    {
        set_object(next_x1, next_y1, OBJECT_PLAYER | next_object1);
        set_object(cur_x, cur_y, OBJECT_GOAL & cur_object);
    }
    else if (next_object1 == OBJECT_BOX || next_object1 == (OBJECT_BOX | OBJECT_GOAL))
    {
        const int next_x2 = cur_x + dir_x * 2;
        const int next_y2 = cur_y + dir_y * 2;
        const object_t next_object2 = get_object(next_x2, next_y2);

        if (next_object2 == OBJECT_SPACE || next_object2 == OBJECT_GOAL)
        {
            set_object(next_x2, next_y2, OBJECT_BOX | next_object2);
            set_object(next_x1, next_y1, OBJECT_PLAYER | (next_object1 & ~OBJECT_BOX));
            set_object(cur_x, cur_y, OBJECT_GOAL & cur_object);
        }
    }

    if ((get_object(cur_x, cur_y) & OBJECT_PLAYER) != OBJECT_PLAYER)
    {
        gp_game->player_x = next_x1;
        gp_game->player_y = next_y1;

        ++gp_game->moves;
    }

    // 클리어 검사
    {
        size_t num_box = 0;

        const size_t map_size = gp_game->map_width * gp_game->map_height;
        for (size_t i = 0; i < map_size; ++i)
        {
            const object_t object = gp_game->pa_objects[i];
            if (object == OBJECT_BOX)
            {
                ++num_box;
            }
        }

        gp_game->num_box = num_box;
    }
}

static void draw(void)
{
    gp_game->p_renderer->vtbl->clear(gp_game->p_renderer, color_set(0.0f, 0.0f, 0.0f, 1.0f));

    for (int y = 0; y < (int)gp_game->map_height; ++y)
    {
        for (int x = 0; x < (int)gp_game->map_width; ++x)
        {
            const object_t object = get_object(x, y);
            i_mesh_t* p_mesh;
            switch (object)
            {
            case OBJECT_WALL:
                p_mesh = gp_game->p_wall_mesh;
                break;
            case OBJECT_PLAYER:
                p_mesh = gp_game->p_player_mesh;
                break;
            case OBJECT_BOX:
                p_mesh = gp_game->p_box_mesh;
                break;
            case OBJECT_GOAL:
                p_mesh = gp_game->p_goal_mesh;
                break;
            case OBJECT_PLAYER | OBJECT_GOAL:
                p_mesh = gp_game->p_player_goal_mesh;
                break;
            case OBJECT_BOX | OBJECT_GOAL:
                p_mesh = gp_game->p_box_goal_mesh;
                break;
            default:
                p_mesh = NULL;
                break;
            }

            if (p_mesh != NULL)
            {
                transform3_t transform;
                transform3_set_position(&transform, (vector3_t){ (float)PADDING * x, (float)PADDING * y, 0.0f });
                transform3_set_rotation(&transform, (vector3_t){ 20.0f, 0.0f, 0.0f });
                transform3_set_scale(&transform, (vector3_t){ 1.0f, 1.0f, 1.0f });

                const matrix_t world_mat = transform3_get_world_matrix(&transform);
                const matrix_t view_mat_tr = matrix_get_transpose(camera3_get_view_matrix(&gp_game->camera));
                const matrix_t projection_mat_tr = matrix_get_transpose(camera3_get_perspective_matrix(&gp_game->camera));
                const matrix_t transform_mat_tr = matrix_get_transpose(matrix_mul(matrix_mul(world_mat, view_mat_tr), projection_mat_tr));

                gp_game->p_renderer->vtbl->draw_mesh3(gp_game->p_renderer, p_mesh, &transform_mat_tr);
            }
        }
    }
    
    if (is_won())
    {
        const int window_width = (int)gp_game->p_renderer->vtbl->get_width(gp_game->p_renderer);
        const int window_height = (int)gp_game->p_renderer->vtbl->get_height(gp_game->p_renderer);
        static wchar_t text[] = L"Win";
        const int len = (int)wcslen(text);
        gp_game->p_renderer->vtbl->draw_text(gp_game->p_renderer,
                                             window_width / 2 - len * 2, window_height / 8,
                                             text, len, color_set(1.0f, 1.0f, 1.0f, 1.0f));
    }


    {
        static wchar_t buff[256];

        swprintf(buff, 256, L"FPS: %zd", (size_t)(1.0f / gp_game->delta_time));
        gp_game->p_renderer->vtbl->draw_text(gp_game->p_renderer, 1, 1, buff, wcslen(buff), color_set(1.0f, 0.0f, 0.0f, 1.0f));

        swprintf(buff, 256, L"FOV(Field Of View): %.4f", camera3_get_fov(&gp_game->camera));
        gp_game->p_renderer->vtbl->draw_text(gp_game->p_renderer, 1, 16, buff, wcslen(buff), color_set(1.0f, 0.0f, 0.0f, 1.0f));

        swprintf(buff, 256, L"moves: %zd", gp_game->moves);
        gp_game->p_renderer->vtbl->draw_text(gp_game->p_renderer, 1, 32, buff, wcslen(buff), color_set(1.0f, 0.0f, 0.0f, 1.0f));

        swprintf(buff, 256, L"box: %zd / %zd", gp_game->num_box, gp_game->num_max_box);
        gp_game->p_renderer->vtbl->draw_text(gp_game->p_renderer, 1, 48, buff, wcslen(buff), color_set(1.0f, 0.0f, 0.0f, 1.0f));
    }

    gp_game->p_renderer->vtbl->on_draw(gp_game->p_renderer);
}

static void print_info(void)
{
    // (0, 0)으로 콘솔 커서 옮기기
    COORD pos = { 0,0 };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);

    printf("window info\n");
    printf("- window width = %zd\n", gp_game->p_renderer->vtbl->get_width(gp_game->p_renderer));
    printf("- window height = %zd\n\n", gp_game->p_renderer->vtbl->get_height(gp_game->p_renderer));

    printf("app info\n");
    printf("- fps = %zd                        \n", (size_t)(1.0f / gp_game->delta_time));
    printf("- delta time = %.6f                \n\n", gp_game->delta_time);

    printf("map info\n");
    for (int y = 0; y < gp_game->map_height; ++y)
    {
        for (int x = 0; x < gp_game->map_width; ++x)
        {
            const object_t object = get_object(x, y);
            char ch;
            switch (object)
            {
            case OBJECT_SPACE:
                ch = ' ';
                break;
            case OBJECT_WALL:
                ch = 'X';
                break;
            case OBJECT_PLAYER:
                ch = 'p';
                break;
            case OBJECT_PLAYER | OBJECT_GOAL:
                ch = 'P';
                break;
            case OBJECT_BOX:
                ch = 'o';
                break;
            case OBJECT_BOX | OBJECT_GOAL:
                ch = 'O';
                break;
            case OBJECT_GOAL:
                ch = '.';
                break;
            default:
                ASSERT(false, "Invalid object");
                break;
            }

            printf("%c", ch);
        }
        printf("\n");
    }
    printf("moves: %zd, box: %zd / %zd\n", gp_game->moves, gp_game->num_box, gp_game->num_max_box);
    printf("%s\n", is_won() ? "Win" : "");
}

static void create_meshs(void)
{
#define SIZE 10.0f

    i_vertex_buffer_t* p_vertex_buffer;
    i_index_buffer_t* p_index_buffer;

    static vertex_t wall_vertices[] =
    {
        { { -SIZE, SIZE, -SIZE }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { { SIZE, SIZE, -SIZE }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { { SIZE, SIZE, SIZE }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { { -SIZE, SIZE, SIZE }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { { -SIZE, -SIZE, -SIZE }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { { SIZE, -SIZE, -SIZE }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { { SIZE, -SIZE, SIZE }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { { -SIZE, -SIZE, SIZE }, { 1.0f, 1.0f, 0.0f, 1.0f } },
    };

    static vertex_t player_vertices[] =
    {
        { { -SIZE, SIZE, -SIZE }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { SIZE, SIZE, -SIZE }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { SIZE, SIZE, SIZE }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { -SIZE, SIZE, SIZE }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { -SIZE, -SIZE, -SIZE }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { SIZE, -SIZE, -SIZE }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { SIZE, -SIZE, SIZE }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { -SIZE, -SIZE, SIZE }, { 1.0f, 0.0f, 0.0f, 1.0f } },
    };

    static vertex_t box_vertices[] =
    {
        { { -SIZE, SIZE, -SIZE }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { SIZE, SIZE, -SIZE }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { SIZE, SIZE, SIZE }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -SIZE, SIZE, SIZE }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -SIZE, -SIZE, -SIZE }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { SIZE, -SIZE, -SIZE }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { SIZE, -SIZE, SIZE }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { { -SIZE, -SIZE, SIZE }, { 0.0f, 1.0f, 0.0f, 1.0f } },
    };

    static vertex_t goal_vertices[] =
    {
        { { -SIZE, SIZE, -SIZE }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { SIZE, SIZE, -SIZE }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { SIZE, SIZE, SIZE }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -SIZE, SIZE, SIZE }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -SIZE, -SIZE, -SIZE }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { SIZE, -SIZE, -SIZE }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { SIZE, -SIZE, SIZE }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { { -SIZE, -SIZE, SIZE }, { 0.0f, 0.0f, 1.0f, 1.0f } },
    };

    static vertex_t player_goal_vertices[] =
    {
        { { -SIZE, SIZE, -SIZE }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { SIZE, SIZE, -SIZE }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { SIZE, SIZE, SIZE }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { -SIZE, SIZE, SIZE }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { -SIZE, -SIZE, -SIZE }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { SIZE, -SIZE, -SIZE }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { SIZE, -SIZE, SIZE }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { { -SIZE, -SIZE, SIZE }, { 1.0f, 0.0f, 1.0f, 1.0f } },
    };

    static vertex_t box_goal_vertices[] =
    {
        { { -SIZE, SIZE, -SIZE }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { SIZE, SIZE, -SIZE }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { SIZE, SIZE, SIZE }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { -SIZE, SIZE, SIZE }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { -SIZE, -SIZE, -SIZE }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { SIZE, -SIZE, -SIZE }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { SIZE, -SIZE, SIZE }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { -SIZE, -SIZE, SIZE }, { 0.0f, 1.0f, 1.0f, 1.0f } },
    };

    static uint_t indices[] =
    {
        3,1,0,
        2,1,3,

        0,5,4,
        1,5,0,

        3,4,7,
        0,4,3,

        1,6,5,
        2,6,1,

        2,7,6,
        3,7,2,

        6,4,5,
        7,4,6,
    };
    
    gp_game->p_renderer->vtbl->create_index_buffer(gp_game->p_renderer, indices, 3 * 12, &p_index_buffer);

    const semantic_t semantics[] = { SEMANTIC_POSITION, SEMANTIC_COLOR };
    const size_t offsets[] = { 0, 16 };

    // wall mesh
    {
        gp_game->p_renderer->vtbl->create_vertex_buffer(gp_game->p_renderer, offsets, semantics, 2,
                                                        wall_vertices, 8,
                                                        &p_vertex_buffer);

        gp_game->p_renderer->vtbl->create_mesh(gp_game->p_renderer,
                                               p_vertex_buffer,
                                               p_index_buffer,
                                               NULL,
                                               color_set(1.0f, 1.0f, 0.0f, 1.0f),
                                               &gp_game->p_wall_mesh);
    }

    // player mesh
    {
        gp_game->p_renderer->vtbl->create_vertex_buffer(gp_game->p_renderer, offsets, semantics, 2,
                                                        player_vertices, 8,
                                                        &p_vertex_buffer);

        gp_game->p_renderer->vtbl->create_mesh(gp_game->p_renderer,
                                               p_vertex_buffer,
                                               p_index_buffer,
                                               NULL,
                                               color_set(1.0f, 0.0f, 0.0f, 1.0f),
                                               &gp_game->p_player_mesh);
    }

    // box mesh
    {
        gp_game->p_renderer->vtbl->create_vertex_buffer(gp_game->p_renderer, offsets, semantics, 2,
                                                        box_vertices, 8,
                                                        &p_vertex_buffer);

        gp_game->p_renderer->vtbl->create_mesh(gp_game->p_renderer,
                                               p_vertex_buffer,
                                               p_index_buffer,
                                               NULL,
                                               color_set(0.0f, 1.0f, 0.0f, 1.0f),
                                               &gp_game->p_box_mesh);
    }

    // goal mesh
    {
        gp_game->p_renderer->vtbl->create_vertex_buffer(gp_game->p_renderer, offsets, semantics, 2,
                                                        goal_vertices, 8,
                                                        &p_vertex_buffer);

        gp_game->p_renderer->vtbl->create_mesh(gp_game->p_renderer,
                                               p_vertex_buffer,
                                               p_index_buffer,
                                               NULL,
                                               color_set(0.0f, 0.0f, 1.0f, 1.0f),
                                               &gp_game->p_goal_mesh);
    }

    // player goal mesh
    {
        gp_game->p_renderer->vtbl->create_vertex_buffer(gp_game->p_renderer, offsets, semantics, 2,
                                                        player_goal_vertices, 8,
                                                        &p_vertex_buffer);

        gp_game->p_renderer->vtbl->create_mesh(gp_game->p_renderer,
                                               p_vertex_buffer,
                                               p_index_buffer,
                                               NULL,
                                               color_set(1.0f, 0.0f, 1.0f, 1.0f),
                                               &gp_game->p_player_goal_mesh);
    }

    // box mesh
    {
        gp_game->p_renderer->vtbl->create_vertex_buffer(gp_game->p_renderer, offsets, semantics, 2,
                                                        box_goal_vertices, 8,
                                                        &p_vertex_buffer);

        gp_game->p_renderer->vtbl->create_mesh(gp_game->p_renderer,
                                               p_vertex_buffer,
                                               p_index_buffer,
                                               NULL,
                                               color_set(0.0f, 1.0f, 1.0f, 1.0f),
                                               &gp_game->p_box_goal_mesh);
    }

    SAFE_RELEASE(p_vertex_buffer);
    SAFE_RELEASE(p_index_buffer);

#undef SIZE
}

static bool load_map(const char* p_map)
{
    ASSERT(p_map != NULL, "p_map == NULL");

    SAFE_FREE(gp_game->pa_objects);

    int x = 0;
    int y = 0;
    int map_width = 0;
    int map_height = 1;
    int player_x = 0;
    int player_y = 0;
    size_t num_box = 0;

    // 맵 크기 구하기
    const char* p = p_map;
    while (*p != '\0')
    {
        if (*p == '\n')
        {
            map_width = (map_width < x) ? x : map_width;
            x = 0;
            ++map_height;
        }
        else
        {
            ++x;
        }

        ++p;
    }

    object_t* pa_objects = (object_t*)malloc(sizeof(object_t) * map_width * map_height);
    if (pa_objects == NULL)
    {
        ASSERT(false, "Failed to malloc objects");
        goto failed_malloc_objects;
    }

    size_t index = 0;
    x = 0;
    y = 0;
    p = p_map;
    while (*p != '\0')
    {
        object_t object;

        switch (*p)
        {
        case ' ':
            object = OBJECT_SPACE;
            break;
        case 'X':
            object = OBJECT_WALL;
            break;
        case 'P':
            object = OBJECT_PLAYER | OBJECT_GOAL;
            player_x = x;
            player_y = y;
            break;
        case 'p':
            object = OBJECT_PLAYER;
            player_x = x;
            player_y = y;
            break;
        case 'O':
            object = OBJECT_BOX | OBJECT_GOAL;
            ++num_box;
            break;
        case 'o':
            object = OBJECT_BOX;
            ++num_box;
            break;
        case '.':
            object = OBJECT_GOAL;
            break;
        case '\n':
            x = -1;
            ++y;
            object = OBJECT_UNKNOWN;
            break;
        default:
            ASSERT(false, "Invalid object");
            goto invalid_object;
        }

        if (object != OBJECT_UNKNOWN)
        {
            ASSERT(index < map_width * map_height, "Out of index");
            pa_objects[index] = object;
            ++index;
        }

        ++x;
        ++p;
    }

    gp_game->pa_objects = pa_objects;
    gp_game->map_width = map_width;
    gp_game->map_height = map_height;
    gp_game->player_x = player_x;
    gp_game->player_y = player_y;
    gp_game->num_box = num_box;
    gp_game->num_max_box = num_box;

    return true;

invalid_object:
    SAFE_FREE(pa_objects);

failed_malloc_objects:
    return false;
}

static object_t get_object(const int x, const int y)
{
    return gp_game->pa_objects[y * gp_game->map_width + x];
}

static void set_object(const int x, const int y, const object_t object)
{
    gp_game->pa_objects[y * gp_game->map_width + x] = object;
}

static bool is_won(void)
{
    return gp_game->num_box == 0;
}