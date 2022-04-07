#include "Sun.h"
#include <cmath>
#include "glm/trigonometric.hpp"

void Hym::Sun::Set()
{
	auto dirRad = glm::radians(Direction);
	auto sinTheta = sin(dirRad.x);
	glm::vec3 dirCart;
	dirCart.x = sinTheta * cos(dirRad.y);
	dirCart.y = sinTheta * sin(dirRad.y);
	dirCart.z = cos(dirRad.x);
	auto con = sunBuffer.Map();
	con->color = Color;
	con->direction = dirCart;
}

void Hym::Sun::RenderShadowMap()
{
}
