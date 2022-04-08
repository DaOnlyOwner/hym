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
			res.Init();

			res.LoadSceneFile(RES "/scenes/SunTemple/SunTemple.fbx","Sponza");
			
			std::vector<std::pair<Concept,std::string>> concepts;
			for (auto& m : *res.GetSceneModels("Sponza"))
			{
				Concept c;
				c.AddComponent<ModelComponent>(m);
				c.AddComponent<TransformComponent>(TransformComponent{});
				concepts.push_back({ c,"" });
			}
			auto ref = ArrayRef<std::pair<Concept,std::string>>::MakeRef(concepts);
			scene.AddConcepts(ref);

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
			renderer.Init(scene, probesXYZ[0], probesXYZ[1], probesXYZ[2], raysPerProbe);
		}

		virtual void Update(Time time) override
		{
			//Resources::Inst().Update(reg);
			handleUI();
			renderer.Draw(scene, cam);
			handle_movement(time);

		}

		void handleUI()
		{
			ImGui::Begin("Lighting");
			ImGui::DragFloat("Light direction Theta", &scene.GetSun().Direction.x, 0.55f, 0, 180, "%.04f", 1);
			ImGui::DragFloat("Light direction Phi", &scene.GetSun().Direction.y, 0.55f, -180, 180, "%.04f", 1);
			scene.GetSun().Color = glm::vec3(1, 1, 1);
			ImGui::Separator();
			ImGui::DragInt3("Probe amounts", probesXYZ);
			ImGui::DragInt("Rays per probe", &raysPerProbe, 1.0f, 2);
			if (ImGui::Button("Apply changes"))
			{
				renderer = Renderer();
				renderer.Init(scene, probesXYZ[0], probesXYZ[1], probesXYZ[2], raysPerProbe);
			}
			
			ImGui::End();

			ImGui::Begin("Framerate etc");
			ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);
			if (ImGui::Button("Rebuild Renderer"))
			{
				renderer = Renderer();
				renderer.Init(scene, probesXYZ[0], probesXYZ[1], probesXYZ[2],raysPerProbe);
			}
			ImGui::End();

		}

		void Resize(int w, int h) override
		{
			if (w < 5 || h < 5) return;
			SwapChain->Resize(w, h);
			cam.SetPerspectiveProj(87.f, w / (float)h, 0.1f, 10000.f);
			renderer.Resize(scene);
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
		int probesXYZ[3] = {32,4,32};
		int raysPerProbe = 64;
	};
}


int main()
{
	Hym::InitInfo init{};
	init.backend = Hym::InitInfo::RenderBackend::D3D12;
	init.enableDebugLayers = true;
	Hym::TestApp app(init);
	return app.Run();
}