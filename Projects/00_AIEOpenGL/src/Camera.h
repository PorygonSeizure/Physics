#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

class Camera
{
public:
	Camera(float fovY, float aspectRatio, float near, float far);
	virtual ~Camera() {}

	virtual void Update(float deltaTime) = 0;

	void SetPerspective(float fovY, float aspectRatio, float near, float far);
	void SetLookAtFrom(const glm::vec3& from, const glm::vec3& to);
	void SetPosition(glm::vec3 pos);

	const glm::mat4& GetTransform() const { return m_transform; }
	const glm::mat4& GetProjection() const { return m_projection; }
	const glm::mat4& GetView() const { return m_view; }
	const glm::mat4& GetProjectionView() const { return m_projectionView; }

	//returns a world-space normalized vector pointing away from the camera's world-space position
	glm::vec3 ScreenPositionToDirection(float x, float y) const;

	//returns the point of intersection of a camera ray and a world-space plane
	//the plane has a normal of XYZ and is offset from (0,0,0) by -W in the direction of the normal
	glm::vec3 PickAgainstPlane(float x, float y, const glm::vec4& plane) const;

protected:
	glm::vec3 m_up;
	glm::mat4 m_transform;
	glm::mat4 m_projection;
	glm::mat4 m_view;
	glm::mat4 m_projectionView;
};



class FlyCamera : public Camera
{
public:
	FlyCamera(float fovY, float aspectRatio, float near, float far);
	virtual ~FlyCamera() {}

	virtual void Update(float deltaTime);
	void SetSpeed(float speed) { m_speed = speed; }

private:
	float m_speed;
};

#endif