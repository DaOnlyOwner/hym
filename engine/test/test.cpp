#include "App.h"
#include "HymScene.h"
#include "ResourceManager.h"
#include "GenericCamera.h"
#include "Renderer.h"
#include "Debug.h"

namespace Hym
{
	class TestApp : public App
	{
	public:
		TestApp(const InitInfo& init)
			:App(init),scene(res)
		{
			renderer.Init();
			res.Init();

			res.LoadSceneFile(RES "/scenes/sponza/Sponza.obj","Sponza");
			for (auto& m : *res.GetSceneModels("Sponza"))
			{
				Concept c;
				c.AddComponent<ModelComponent>(m);
				c.AddComponent<TransformComponent>(TransformComponent{});
				scene.AddConcept(c);
			}

			cam.SetPerspectiveProj(87.f, init.width / (float)init.height, 0.1f, 100000.f);
			cam.SetEyePos({ 1,0,0 });
			cam.LookAt({ 0,0,0 });
			//auto min = Resources::Inst().GetMinScene();
			//auto max = Resources::Inst().GetMaxScene();
			//Renderer::Inst().InitTLAS();
			//Renderer::Inst().InitIrrField(32, 4, 32, 64, min, max);
			//s.direction = glm::vec3(0, -1, 0.3f);
			//s.color = glm::vec3(1, 1, 1);
			//Renderer::Inst().SetSun(s);
		}

		virtual void Update(Time time) override
		{
			//Resources::Inst().Update(reg);
			renderer.Draw(scene, cam);
			handle_movement(time);
		}

		void Resize(int w, int h) override
		{
			if (w < 5 || h < 5) return;
			SwapChain->Resize(w, h);
			cam.SetPerspectiveProj(87.f, w / (float)h, 0.1f, 10000.f);
			renderer.Resize();
		}

		void handle_movement(Time time)
		{
			auto deltaTime = time.Secs();
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cam.Move({ 0,0,SPEED * deltaTime });
			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cam.Move({ 0,0,-SPEED * deltaTime });
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam.Move({ -SPEED * deltaTime,0,0 });
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam.Move({ SPEED * deltaTime,0,0 });
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam.Move({ 0,-SPEED * deltaTime,0 });
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam.Move({ 0,SPEED * deltaTime,0 });

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
				int deltaX = oldX - GetMousePosX();
				int deltaY = oldY - GetMousePosY();

				cam.RotateEye(deltaX * deltaTime * ROTATION_SPEED, deltaY * deltaTime * ROTATION_SPEED);
			}

			oldX = GetMousePosX();
			oldY = GetMousePosY();

			/*int sign = 1;
			if (glfwGetKey(window, GLFW_KEY_R))
				sign = -1;
			if (glfwGetKey(window, GLFW_KEY_J))
			{

				s.direction += glm::vec3(sign * SPEED * deltaTime, 0, 0);
			}

			if (glfwGetKey(window, GLFW_KEY_K))
			{
				s.direction += glm::vec3(0, sign * SPEED * deltaTime, 0);
			}

			if (glfwGetKey(window, GLFW_KEY_L))
			{
				s.direction += glm::vec3(0, 0, sign * SPEED * deltaTime);
			}*/


		}

	private:
		Scene scene;
		Camera cam;
		Renderer renderer;
		ResourceManager res;
		//Sun s;
		static constexpr float SPEED = 5;
		static constexpr float ROTATION_SPEED = 5;
		float oldX = 0, oldY = 0;
	};
}


int main()
{
	Hym::InitInfo init{};
	//init.backend = Hym::InitInfo::RenderBackend::D3D12;
	init.enableDebugLayers = true;
	Hym::TestApp app(init);
	return app.Run();
}