/*
 * ATOMS OS — Userspace BGL Platform API Adapter
 * Copyright (C) 2026 ATOMS OS Project / Saumya Chaudhari
 */

#include "kernel/graphics/bgl/bgl.h"
#include "userspace/runtime/c/include/stdlib.h"

static BGLContext* s_current_context = NULL;
static BGLDrawable* s_current_drawable = NULL;
static BGLError s_last_error = BGL_SUCCESS;
static uint32_t s_next_id = 1;

BGLDrawable* bglCreateDrawableForWindow(uint32_t window_id) {
    BGLDrawable* d = (BGLDrawable*)malloc(sizeof(BGLDrawable));
    if (!d) return NULL;
    d->drawable_id = s_next_id++;
    d->window_id = window_id;
    d->owner_pid = 1;
    d->width = 1920;
    d->height = 1080;
    d->pitch = 1920 * 4;
    d->pixel_format = BGL_FORMAT_ARGB8888;
    d->color_buffer = (uint32_t*)malloc(1920 * 1080 * 4);
    d->depth_buffer = (float*)malloc(1920 * 1080 * sizeof(float));
    d->stencil_buffer = (uint8_t*)malloc(1920 * 1080);
    d->generation = 1;
    d->active = true;
    d->is_dirty = false;
    return d;
}

bool bglDestroyDrawable(BGLDrawable* drawable) {
    if (!drawable) return false;
    if (drawable->color_buffer) free(drawable->color_buffer);
    if (drawable->depth_buffer) free(drawable->depth_buffer);
    if (drawable->stencil_buffer) free(drawable->stencil_buffer);
    free(drawable);
    return true;
}

bool bglResizeDrawable(BGLDrawable* drawable, uint32_t new_w, uint32_t new_h) {
    if (!drawable) return false;
    drawable->width = new_w;
    drawable->height = new_h;
    drawable->pitch = new_w * 4;
    drawable->generation++;
    return true;
}

BGLDrawable* bglGetDrawable(uint32_t drawable_id) {
    (void)drawable_id;
    return s_current_drawable;
}

BGLContext* bglCreateContext(BGLDrawable* drawable) {
    BGLContext* ctx = (BGLContext*)malloc(sizeof(BGLContext));
    if (!ctx) return NULL;
    ctx->context_id = s_next_id++;
    ctx->owner_pid = 1;
    ctx->owner_task_id = 1;
    ctx->bound_drawable = drawable;
    ctx->state = BGL_STATE_CREATED;
    ctx->clear_color = 0xFF000000;
    ctx->drawable_generation = drawable ? drawable->generation : 0;
    ctx->swap_count = 0;
    ctx->clear_count = 0;
    ctx->private_data = NULL;
    ctx->active = true;
    return ctx;
}

bool bglDestroyContext(BGLContext* ctx) {
    if (!ctx) return false;
    if (ctx == s_current_context) {
        s_current_context = NULL;
        s_current_drawable = NULL;
    }
    free(ctx);
    return true;
}

void bglDiagnosticClear(BGLContext* ctx, uint32_t color) {
    if (!ctx || !ctx->bound_drawable) return;
    ctx->clear_color = color;
    ctx->clear_count++;
}

BGLContext* bglGetContext(uint32_t context_id) {
    (void)context_id;
    return s_current_context;
}

void bglDetachDrawableFromContexts(BGLDrawable* drawable) {
    if (s_current_drawable == drawable) {
        s_current_drawable = NULL;
    }
}

bool bglMakeCurrent(BGLContext* ctx, BGLDrawable* drawable) {
    s_current_context = ctx;
    s_current_drawable = drawable;
    if (ctx) {
        ctx->bound_drawable = drawable;
    }
    return true;
}

void bglReleaseCurrent(void) {
    s_current_context = NULL;
    s_current_drawable = NULL;
}

BGLContext* bglGetCurrentContext(void) {
    return s_current_context;
}

bool bglSwapBuffers(BGLContext* ctx) {
    if (!ctx) return false;
    ctx->swap_count++;
    return true;
}

BGLError bglGetLastError(void) {
    return s_last_error;
}

void bglSetLastError(BGLError err) {
    s_last_error = err;
}
