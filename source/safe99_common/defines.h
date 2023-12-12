﻿//***************************************************************************
// 
// 파일: defines.h
// 
// 설명: 매크로 정의
// 
// 작성자: bumpsgoodman
// 
// 작성일: 2023/09/04
// 
//***************************************************************************

#ifndef DEFINES_H
#define DEFINES_H

#include <intrin.h>

// 타입 정의
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef unsigned int uint_t;

// ASSERT
#define ASSERT(cond, msg) ((void)0)

// SAFE DELETE
#include <stdlib.h>
#define SAFE_FREE(p)         { free((p)); (p) = NULL; }
#define SAFE_RELEASE(p)      { if ((p)) { (p)->vtbl->release((p)); (p) = NULL; } }
#define SAFE_RELEASE_MS(p)   { if ((p)) { (p)->Release(); (p) = NULL; } }

// extern "C"
#ifdef __cplusplus
#define START_EXTERN_C extern "C" {
#define END_EXTERN_C }
#else
#define START_EXTERN_C
#define END_EXTERN_C
#endif // __cplusplus

// 인터페이스
#ifndef interface
#define interface struct
#endif // interface

// DLL 객체 생성
typedef void (__stdcall *create_instance_func)(void**);

// SIMD
#if (defined(__AVX2__))
#define SUPPORT_AVX2
#endif // AVX2
#if (defined(__AVX__))
#define SUPPORT_AVX
#endif // AVX
#if (defined(_M_AMD64) || defined(_M_X64) || defined(__x86_64__) || defined(_M_IX86) || defined(__i386__))
#define SUPPORT_SSE
#endif // SSE

// inline
#define INLINE inline
#define FORCEINLINE __forceinline

// dll exports
#ifdef SAFE99_DLL_EXPORTS
#define SAFE99_API __declspec(dllexport)
#else
#define SAFE99_API __declspec(dllimport)
#endif // SAFE99_DLL_EXPORTS

#define TO_STR(s) #s

// alignment
#define ALIGN8 _declspec(align(8))
#define ALIGN16 _declspec(align(16))
#define ALIGN32 _declspec(align(32))

typedef struct rect
{
    int left;
    int top;
    int right;
    int bottom;
} rect_t;

typedef struct timer
{
    uint64_t frequency;
    uint64_t prev_counter;
} timer_t;

ALIGN16 typedef struct color
{
    union
    {
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };

        float rgba[4];
    };
} color_t;

typedef __m128 vector_t;

ALIGN16 typedef struct vector2
{
    float x;
    float y;
} vector2_t;

ALIGN16 typedef struct vector3
{
    float x;
    float y;
    float z;
} vector3_t;

ALIGN16 typedef struct vector4
{
    float x;
    float y;
    float z;
    float w;
} vector4_t;

ALIGN16 typedef struct matrix
{
    union
    {
        struct
        {
            vector_t r0;
            vector_t r1;
            vector_t r2;
            vector_t r3;
        };

        vector_t r[4];
    };
} matrix_t;

typedef __m128i vector_int_t;

ALIGN16 typedef struct vector2_int
{
    int x;
    int y;
} vector2_int_t;

ALIGN16 typedef struct vector3_int
{
    int x;
    int y;
    int z;
} vector3_int_t;

ALIGN16 typedef struct vector4_int
{
    int x;
    int y;
    int z;
    int w;
} vector4_int_t;

typedef interface i_safe3d i_safe3d_t;
typedef interface i_safe3d_vtbl
{
    size_t              (__stdcall*     add_ref)(i_safe3d_t* p_this);
    size_t              (__stdcall*     release)(i_safe3d_t* p_this);
    size_t              (__stdcall*     get_ref_count)(const i_safe3d_t* p_this);

    const vector3_t*    (__stdcall*     get_vertices)(const i_safe3d_t* p_this);
    const uint_t*       (__stdcall*     get_indices)(const i_safe3d_t* p_this);

    size_t              (__stdcall*     get_num_vertices)(const i_safe3d_t* p_this);
    size_t              (__stdcall*     get_num_indices)(const i_safe3d_t* p_this);
    color_t             (__stdcall*     get_wireframe_color)(const i_safe3d_t* p_this);
} i_safe3d_vtbl_t;

typedef interface i_safe3d
{
    i_safe3d_vtbl_t* vtbl;
} i_safe3d_t;

typedef interface i_texture i_texture_t;
typedef interface i_texture_vtbl
{
    size_t          (__stdcall*     add_ref)(i_texture_t* p_this);
    size_t          (__stdcall*     release)(i_texture_t* p_this);
    size_t          (__stdcall*     get_ref_count)(const i_texture_t* p_this);

    size_t          (__stdcall*     get_width)(const i_texture_t* p_this);
    size_t          (__stdcall*     get_height)(const i_texture_t* p_this);
    const char*     (__stdcall*     get_bitmap)(const i_texture_t* p_this);
} i_texture_vtbl_t;

typedef interface i_texture
{
    i_texture_vtbl_t* vtbl;
} i_texture_t;

typedef interface i_vertex_buffer i_vertex_buffer_t;
typedef interface i_vertex_buffer_vtbl
{
    size_t              (__stdcall*     add_ref)(i_vertex_buffer_t* p_this);
    size_t              (__stdcall*     release)(i_vertex_buffer_t* p_this);
    size_t              (__stdcall*     get_ref_count)(const i_vertex_buffer_t* p_this);

    const vector3_t*    (__stdcall*     get_positions)(const i_vertex_buffer_t* p_this);
    const color_t*      (__stdcall*     get_colors_or_null)(const i_vertex_buffer_t* p_this);
    const vector2_t*    (__stdcall*     get_tex_coord_or_null)(const i_vertex_buffer_t* p_this);

    size_t              (__stdcall*     get_num_vertices)(const i_vertex_buffer_t* p_this);
} i_vertex_buffer_vtbl_t;

typedef interface i_vertex_buffer
{
    i_vertex_buffer_vtbl_t* vtbl;
} i_vertex_buffer_t;

typedef interface i_index_buffer i_index_buffer_t;
typedef interface i_index_buffer_vtbl
{
    size_t          (__stdcall*     add_ref)(i_index_buffer_t* p_this);
    size_t          (__stdcall*     release)(i_index_buffer_t* p_this);
    size_t          (__stdcall*     get_ref_count)(const i_index_buffer_t* p_this);

    const uint_t*   (__stdcall*     get_indices)(const i_index_buffer_t* p_this);
    size_t          (__stdcall*     get_num_indices)(const i_index_buffer_t* p_this);
} i_index_buffer_vtbl_t;

typedef interface i_index_buffer
{
    i_index_buffer_vtbl_t* vtbl;
} i_index_buffer_t;

typedef interface i_mesh i_mesh_t;
typedef interface i_mesh_vtbl
{
    size_t                  (__stdcall*     add_ref)(i_mesh_t* p_this);
    size_t                  (__stdcall*     release)(i_mesh_t* p_this);
    size_t                  (__stdcall*     get_ref_count)(const i_mesh_t* p_this);

    i_vertex_buffer_t*      (__stdcall*     get_vertex_buffer)(const i_mesh_t* p_this);
    i_index_buffer_t*       (__stdcall*     get_index_buffer)(const i_mesh_t* p_this);
    i_texture_t*            (__stdcall*     get_texture)(const i_mesh_t* p_this);
    color_t                 (__stdcall*     get_wireframe_color)(const i_mesh_t* p_this);
} i_mesh_vtbl_t;

typedef interface i_mesh
{
    i_mesh_vtbl_t* vtbl;
} i_mesh_t;

// Note:
// - 멤버 읽기/쓰기 모두 도우미 함수를 사용해야 함
// ex) const vector2_t position = transform2_get_position(&transform);
typedef struct transform2
{
    vector2_t position;
    float rotation;
    vector2_t scale;

    vector2_t right_vector;
    vector2_t up_vector;

    matrix_t model_matrix;
} transform2_t;

// Note:
// - 멤버 읽기/쓰기 모두 도우미 함수를 사용해야 함
// ex) const vector3_t position = transform3_get_position(&transform);
typedef struct transform3
{
    vector3_t position;
    vector3_t rotation;
    vector3_t scale;

    vector3_t right_vector;
    vector3_t up_vector;
    vector3_t forward_vector;

    matrix_t model_matrix;
} transform3_t;

typedef struct view_port
{
    float top_left_x;
    float top_left_y;
    float width;
    float height;
} view_port_t;

typedef struct camera2
{
    transform2_t transform;
    matrix_t view_matrix;
} camera2_t;

typedef struct camera3
{
    transform3_t transform;
    view_port_t view_port;
    float fov;
    float near_z;
    float far_z;
} camera3_t;

#endif // DEFINES_H