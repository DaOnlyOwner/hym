#include "Sun.h"
#include <cmath>
#include "glm/trigonometric.hpp"
#include "glm/gtx/norm.hpp"

void Hym::Sun::Set()
{
	auto dirRad = glm::radians(Direction);
	auto dirCart = ToCartesian(dirRad);
	auto con = sunBuffer.Map();
	con->color = Color;
	con->direction = glm::normalize(dirCart);
}

void Hym::Sun::RenderShadowMap()
{
}

glm::vec3 Hym::Sun::ToCartesian(const glm::vec2& spherical)
{
	auto sinTheta = sin(spherical.x);
	glm::vec3 dirCart;
	dirCart.x = sinTheta * cos(spherical.y);
	dirCart.y = sinTheta * sin(spherical.y);
	dirCart.z = cos(spherical.x);
	return dirCart;
}
