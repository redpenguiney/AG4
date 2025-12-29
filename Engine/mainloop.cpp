#include "mainloop.hpp"
#include "utility.hpp"
#include "log.hpp"
#include "graphics_engine.hpp"
#include "window.hpp"

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
			// TODO PHYSICS
			physicsLag -= simulationTimestep;
		}
		if (physicsLag > simulationTimestep) {
			DebugLogInfo("Simulation is ", physicsLag, " seconds behind (", elapsedTime, "s since last frame).");
		}

		GraphicsEngine::Get().RenderScene(elapsedTime);
		// TODO: unsure about placement of flip buffers? 
		// i think this yields until GPU done drawing and image on screen
		// could/should we do something to try and do physics or something while GPU working? or are we already? 
		// printf("Flipping buffers.\n");
		Window::Get().FlipBuffers();
		if (!Window::Get().vsync || !Window::Get().doubleBuf) {
			while (Time() - currentTime < 1.0 / 60.0) {}
		}
	}
}
