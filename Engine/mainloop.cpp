#include "mainloop.hpp"
#include "utility.hpp"
#include "log.hpp"
#include "graphics_engine.hpp"
#include "window.hpp"
#include "physics_engine.hpp"
#include "aabb_tree.hpp"

Mainloop& Mainloop::Get() {
	static Mainloop m;
	return m;
}

void Mainloop::Run() {
	previousTime = Time();

	while (!quit) {
		double currentTime = Time();
		double elapsedTime = currentTime - previousTime;
		previousTime = currentTime;
		
		if (!physicsPaused) physicsLag += elapsedTime; // time has passed and thus the simulation is behind
		unsigned tries = 0;
		while (physicsLag > simulationTimestep && tries++ < 2) {
			prePhysics->Fire(simulationTimestep);
			BaseEvent::FlushEventQueue();
			PhysicsEngine::Get().StepSimulation(simulationTimestep);
			postPhysics->Fire(simulationTimestep);
			BaseEvent::FlushEventQueue();
			physicsLag -= simulationTimestep;
		}
		if (physicsLag > simulationTimestep) {
			DebugLogInfo("Simulation is ", physicsLag, " seconds behind (", elapsedTime, "s since last frame).");
		}

		GameobjectSAS().OptimizeTree();

		preRender->Fire(elapsedTime);
		BaseEvent::FlushEventQueue();
		GraphicsEngine::Get().RenderScene(elapsedTime);
		postRender->Fire(elapsedTime);
		BaseEvent::FlushEventQueue();
		// TODO: unsure about placement of flip buffers? 
		// i think this yields until GPU done drawing and image on screen
		// could/should we do something to try and do physics or something while GPU working? or are we already? 
		// printf("Flipping buffers.\n");
		Window::Get().FlipBuffers();
		if (!Window::Get().vsync || !Window::Get().doubleBuf) {
			while (Time() - currentTime < 1.0 / 60.0) {}
		}

		Window::Get().Update();
		if (quitIfTryingToCloseWidnow && Window::Get().ShouldClose()) quit = true;
	}
}
