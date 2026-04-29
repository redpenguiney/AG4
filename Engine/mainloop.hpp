#pragma once
#include "event.hpp"

// Singleton in charge of running the game loop. It decides when rendering/simulation/etc. occur.
// TODO: max framerate option in leiu of vsync
// TODO: adaptive timestep option
class Mainloop {
public:
	static Mainloop& Get();

	bool quit = false;
	// TODO: find a way to move pausing out of main.cpp
	bool physicsPaused = false;

	// for debugging
	bool stepPhysics = false;

	// sets quit = true if Window::Get().ShouldClose() returns true.
	bool quitIfTryingToCloseWidnow = true;

	float simulationTimestep = 1.0f / 60.0f; // number of seconds physics simulation is stepped by every frame

	Event<Mainloop, float>& preRender = Event<Mainloop, float>::New();
	Event<Mainloop, float>& postRender = Event<Mainloop, float>::New();

	Event<Mainloop, float>& prePhysics = Event<Mainloop, float>::New();
	Event<Mainloop, float>& postPhysics = Event<Mainloop, float>::New();

private:
	Mainloop() = default;
	Mainloop(const Mainloop&) = delete;
	~Mainloop() = default;

	double previousTime = 0.0;
	double physicsLag = 0.0; // how many seconds behind the simulation is. Before rendering, we check if lag is > SIMULATION_TIMESTEP and if so, simulate stuff.


	void Run();

	friend int main();
};