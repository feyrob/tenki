#pragma once
#ifndef COLISION_H

#include "summerjam.h"
#include "game.h"
#include "vector2_summer.h"

typedef struct rect 
{
	f32 left;
	f32 right;
	f32 top;
	f32 bottom;
} rect;

typedef struct Collision 
{
	f32 time;
	Vector2 normal;
} Collision;

typedef struct Penetration 
{
	Vector2 depth;
	Vector2 normal;
} Penetration;

rect GetEntityRect(Entity ent);

rect GetExpandedRect(Entity ent, f32 halfWidth, f32 halfHeight, f32 modifier = 0.0f);

bool Is_Penetration(Entity e1, Entity e2, Penetration& pen);

int get_worst_pen_index(Penetration pens[], int pen_count);

bool Is_Collision(Entity e1, Entity e2, Collision& col, f32 dt);

void resolve_swept_collisions_with_terrain(Entity* ent, Collision cols[], int num_cols);

bool Is_Penetration_Naive(Entity e1, Entity e2);

bool is_point_collision(Entity ent, Vector2 pos);

#define COLISION_H
#endif
