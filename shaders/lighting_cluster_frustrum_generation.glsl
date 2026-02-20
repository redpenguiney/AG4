#version 430 core

layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct Cluster {
	vec4 p1; // wcoord is padding
	vec4 p2; // wcoord is padding
	uint nLights;
	uint lights[127];
};

layout (binding = 0, std430) buffer Clusters {
	Cluster[] clusters;
};

