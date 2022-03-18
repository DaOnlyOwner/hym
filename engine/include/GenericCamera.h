#pragma once
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace Hym
{
    // Unit = m
    class Camera
    {
    public:
        Camera() {}
        Camera(float aspect);
        glm::mat4 GetViewMatrix();
        glm::mat4 GetProjMatrix() const;
        void Move(const glm::vec3& delta);
        void RotateEye(float phi, float theta);
        void LookAt(const glm::vec3& center);
        void SetEyePos(const glm::vec3& eye_pos);
        void SetOrthoProj(float left, float right, float bot, float top, float near_, float far_) { proj = glm::ortho(left, right, bot, top, near_, far_); } // "near" is a macro the compiler defines somehow
        void SetPerspectiveProj(float fov, float aspect, float near_, float far_) { proj = glm::perspective(glm::radians(fov), aspect, near_, far_); }
        glm::vec3 GetEyePos() const;
        glm::vec3 GetForward() const;
        glm::vec3 WorldUp{ 0,1,0 };

    private:
        glm::vec3 eye;
        glm::vec3 forward{ 0,0,1 };
        glm::mat4 proj;
        glm::mat4 view;
        bool view_dirty = true;
    };
}