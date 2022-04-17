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

			res.LoadSceneFile(RES "/scenes/sponza/sponza.obj","Sponza");
			
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
			cam.SetEyePos({ 1,3,0 });
			cam.LookAt({ 1,1,1 });
			renderer.Init(scene, probesXYZ[0], probesXYZ[1], probesXYZ[2], raysPerProbe);
		}

		virtual void Update(Time time) override
		{
			handleUI();
			renderer.Draw(scene, cam);
			handle_movement(time);

		}

		void handleUI()
		{
			ImGui::Begin("Lighting");
			ImGui::DragFloat("Light direction Theta", &scene.GetSun().Direction.x, 0.55f, 0, 180, "%.04f", 1);
			ImGui::DragFloat("Light direction Phi", &scene.GetSun().Direction.y, 0.55f, -180, 180, "%.04f", 1);
			bool isChangeECons = ImGui::DragFloat("Energy Conservation", &econservation,0.01f,0,1,"%.04f");
			if (isChangeECons) renderer.SetEnergyConservation(econservation);
			ImGui::DragFloat("Indirect Intensity", &indirectInt, 0.05f, 0, 100);
			ImGui::DragFloat("Direct Intensity", &directInt, 0.05f, 0, 100);
			renderer.SetIndirectIntensity(indirectInt);
			renderer.SetDirectIntensity(directInt);
			scene.GetSun().Color = glm::vec3(1, 1, 1);
			ImGui::Separator();
			ImGui::DragInt3("Probe amounts", probesXYZ);
			ImGui::DragInt("Rays per probe", &raysPerProbe, 1.0f, 2);
			if (ImGui::Button("Apply changes"))
			{
				renderer = Renderer();
				renderer.Init(scene, probesXYZ[0], probesXYZ[1], probesXYZ[2], raysPerProbe);
				HYM_INFO("Reloaded renderer");
			}
			
			ImGui::End();

			ImGui::Begin("Framerate etc");
			ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);
			if (ImGui::Button("Rebuild Renderer"))
			{
				renderer = Renderer();
				renderer.Init(scene, probesXYZ[0], probesXYZ[1], probesXYZ[2],raysPerProbe);
				HYM_INFO("Reloaded renderer");
			}
			ImGui::End();
			
			ImGui::Begin("Debug");
			ImGui::Checkbox("Render Debug Probes", &rdp);
			ImGui::Checkbox("Show Ray Hitlocations", &shl);
			ImGui::DragInt("Debug Probe ID", &probeID[0], 1.0f, 0, probesXYZ[0]*probesXYZ[1]*probesXYZ[2]);
			int probeID1d = probeID[0];
			//ImGui::DragInt("Debug Probe Y", &probeID[1], 1.0f, 0, probesXYZ[1]);
			//ImGui::DragInt("Debug Probe Z", &probeID[2], 1.0f, 0, probesXYZ[2]);

			//int probeID1d = (probeID[2] * probesXYZ[0] * probesXYZ[1]) + (probeID[1] * probesXYZ[0]) + probeID[0];

			renderer.DoDebugDrawProbes(rdp);
			renderer.DoShowHitLocations(shl);
			renderer.SetDebugProbeID(probeID1d);
			ImGui::Separator();
			bool c = ImGui::DragFloat("Normal Bias", &normalBias, 0.01, 0, 10,"%.06f");
			bool c1 = ImGui::DragFloat("Cheb Bias", &chebBias, 0.01, 0, 10, "%.06f");
			bool c2 = ImGui::DragFloat("Depth Sharpness", &sharpness, 1, 0, 200);
			bool c3 = ImGui::DragFloat("Min Ray Distance", &minRayDst, 0.001, 0, 100, "%.06f");
			if (c) renderer.SetNormalBias(normalBias);
			if (c1) renderer.SetChebyshevBias(chebBias);
			if (c2) renderer.SetDepthSharpness(sharpness);
			if (c3) renderer.SetRayMinDst(minRayDst);
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
		int probesXYZ[3] = {16,16,16};
		int raysPerProbe = 64;
		bool rdp = false;
		bool shl = false;
		int probeID[3] = { 0,0,0 };

		float sharpness = 50.f, normalBias = 0.25f, chebBias = 0.f, minRayDst = 0.08;
		float econservation = 0.85;
		float indirectInt = 0.4f;
		float directInt = 1;

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