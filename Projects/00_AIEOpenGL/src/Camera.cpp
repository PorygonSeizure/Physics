#define GLM_SWIZZLE
#include "Camera.h"
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

using glm::vec3;
using glm::vec4;
using glm::inverse;

Camera::Camera(float fovY, float aspectRatio, float near, float far) : 
	m_up(0, 1, 0)
{
	SetLookAtFrom(vec3(0, 0, 0), vec3(0, 0, 1));
	SetPerspective(fovY, aspectRatio, near, far);
}

void Camera::SetPerspective(float fovY, float aspectRatio, float near, float far)
{
	m_projection = glm::perspective(fovY, aspectRatio, near, far);
	m_projectionView = m_projection * m_view;
}

void Camera::SetLookAtFrom(const vec3& from, const vec3& to)
{
	m_view = glm::lookAt(from, to, m_up);
	m_transform = inverse(m_view);
	m_projectionView = m_projection * m_view;
}

void Camera::SetPosition(vec3 pos)
{
	m_transform[3] = vec4(pos, 1);
	m_view = inverse(m_transform);
	m_projectionView = m_projection * m_view;
}

vec3 Camera::ScreenPositionToDirection(float x, float y) const
{
	int width = 0;
	int height = 0;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);

	vec3 screenPos(x / width * 2 - 1, (y / height * 2 - 1) * -1, -1);

	screenPos.x /= m_projection[0][0];
	screenPos.y /= m_projection[1][1];

	return glm::normalize(m_transform * vec4(screenPos, 0)).xyz();
}

vec3 Camera::PickAgainstPlane(float x, float y, const vec4& plane) const
{
	int width = 0;
	int height = 0;
	glfwGetWindowSize(glfwGetCurrentContext(), &width, &height);
	
	vec4 screenPos(x / width - 0.5f * 2, y / height -  0.5f * -2, 1, 1);
	
	glm::mat4 inverseTemp = inverse(m_projectionView);
	
	screenPos = inverseTemp * screenPos;
	
	screenPos /= screenPos.w;
	
	vec3 dir = screenPos.xyz() - m_transform[3].xyz();
	
	float t = -(glm::dot(m_transform[3].xyz(), plane.xyz()) + plane.w) / (glm::dot(dir, plane.xyz()));
	
	return (m_transform[3].xyz() + dir * t);
}



FlyCamera::FlyCamera(float fovY, float aspectRatio, float near, float far) : Camera(fovY, aspectRatio, near, far)
{
	this->m_speed = 10.f;
}

void FlyCamera::Update(float deltaTime)
{
	GLFWwindow* window = glfwGetCurrentContext();

	float frameSpeed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? deltaTime * m_speed * 2 : deltaTime * m_speed;

	//translate
	if (glfwGetKey(window, 'W') == GLFW_PRESS)
		m_transform[3] -= m_transform[2] * frameSpeed;
	if (glfwGetKey(window, 'S') == GLFW_PRESS)
		m_transform[3] += m_transform[2] * frameSpeed;
	if (glfwGetKey(window, 'D') == GLFW_PRESS)
		m_transform[3] += m_transform[0] * frameSpeed;
	if (glfwGetKey(window, 'A') == GLFW_PRESS)
		m_transform[3] -= m_transform[0] * frameSpeed;
	if (glfwGetKey(window, 'Q') == GLFW_PRESS)
		m_transform[3] += m_transform[1] * frameSpeed;
	if (glfwGetKey(window, 'E') == GLFW_PRESS)
		m_transform[3] -= m_transform[1] * frameSpeed;

	//check for rotation
	static bool mouseButtonDown = false;
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
	{
		static double prevMouseX = 0.0;
		static double prevMouseY = 0.0;

		if (!mouseButtonDown)
		{
			mouseButtonDown = true;
			glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
		}

		double mouseX = 0.0;
		double mouseY = 0.0;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		double deltaX = mouseX - prevMouseX;
		double deltaY = mouseY - prevMouseY;

		prevMouseX = mouseX;
		prevMouseY = mouseY;

		glm::mat4 mat;

		//pitch
		if (deltaY != 0.0)
		{
			mat = glm::axisAngleMatrix(m_transform[0].xyz(), (float)-deltaY / 150.f);
			m_transform[0] = mat * m_transform[0];
			m_transform[1] = mat * m_transform[1];
			m_transform[2] = mat * m_transform[2];
		}

		//yaw
		if (deltaX != 0.0)
		{
			mat = glm::axisAngleMatrix(m_up, (float)-deltaX / 150.f);
			m_transform[0] = mat * m_transform[0];
			m_transform[1] = mat * m_transform[1];
			m_transform[2] = mat * m_transform[2];
		}
	}
	else
		mouseButtonDown = false;

	m_view = inverse(m_transform);
	m_projectionView = m_projection * m_view;
}