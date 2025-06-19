#include "client.h"
#include "pathfinder_null.h"
#include "pathfinder_nav_mesh.h"
#include "../common/path_manager.h"
#include <fmt/format.h>
#include <sys/stat.h>
#include <iostream>

IPathfinder *IPathfinder::Load(const std::string &zone, const std::string &custom_navmesh_path) {
	struct stat statbuffer;
	std::string navmesh_file_path;
	
	if (!custom_navmesh_path.empty()) {
		// Use custom navmesh path
		navmesh_file_path = fmt::format("{}/{}.nav", custom_navmesh_path, zone);
		std::cout << fmt::format("[DEBUG] IPathfinder::Load: Using custom navmesh path: {}", custom_navmesh_path) << std::endl;
	} else {
		// Use default server path
		std::string server_path = path.GetServerPath();
		navmesh_file_path = fmt::format("{}/maps/nav/{}.nav", server_path, zone);
		std::cout << fmt::format("[DEBUG] IPathfinder::Load: Using server path: {}", server_path) << std::endl;
	}
	
	std::cout << fmt::format("[DEBUG] IPathfinder::Load: Looking for navmesh at: {}", navmesh_file_path) << std::endl;
	
	if (stat(navmesh_file_path.c_str(), &statbuffer) == 0) {
		std::cout << "[DEBUG] IPathfinder::Load: Found navmesh file, loading PathfinderNavmesh" << std::endl;
		return new PathfinderNavmesh(navmesh_file_path);
	}

	std::cout << "[DEBUG] IPathfinder::Load: No navmesh file found, returning PathfinderNull" << std::endl;
	return new PathfinderNull();
}
