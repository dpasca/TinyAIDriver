//==================================================================
/// MathBase.h
///
/// Created by Davide Pasca - 2022/05/29
/// See the file "license.txt" that comes with this project for
/// copyright info.
//==================================================================

#ifndef MATHBASE_H
#define MATHBASE_H

#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <cfloat>

inline constexpr auto FM_PI = (float)M_PI;

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>  // glm::translate, glm::rotate, glm::scale
#include <glm/ext/matrix_clip_space.hpp> // glm::perspective
#include <glm/ext/scalar_constants.hpp>  // glm::pi
#include <glm/gtx/polar_coordinates.hpp> // glm::euclidean

using Float2 = glm::vec2;
using Float3 = glm::vec3;
using Int3 = glm::ivec3;

using Matrix44 = glm::mat4;

//==================================================================
inline auto lengthSqr = []( const auto &v ) { return glm::dot( v, v ); };

#endif

