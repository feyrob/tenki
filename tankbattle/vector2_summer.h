#pragma once

#ifndef VECTOR2_H

#include "summerjam.h"

typedef struct Vector2 {
	f32 x;
	f32 y;
} Vector2;

Vector2 operator+(Vector2 const& lhs, Vector2 const& rhs);

Vector2& operator+=(Vector2& lhs, Vector2 const& rhs);

Vector2 operator-(Vector2 const& lhs, Vector2 const& rhs);

Vector2 operator*(Vector2 const& lhs, Vector2 const& rhs);

Vector2 operator*(Vector2 const& lhs, float const& rhs);

Vector2 operator*(float const& lhs, Vector2 const& rhs);

Vector2 Rotate(Vector2 const& vec, float rotate);

#define VECTOR2_H
#endif
