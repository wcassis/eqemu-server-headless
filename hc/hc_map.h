#pragma once

#include <string>
#include <glm/glm.hpp>

#define BEST_Z_INVALID -99999

// Simplified Map class for headless client
// Compatible with EQEmu zone server map files
class HCMap
{
public:
	HCMap();
	~HCMap();

	// Load a map file from the specified path
	bool Load(const std::string& filename);
	
	// Find the best Z coordinate at the given X,Y position
	// Returns BEST_Z_INVALID if no valid Z found
	float FindBestZ(const glm::vec3 &start, glm::vec3 *result = nullptr) const;
	
	// Check if map is loaded
	bool IsLoaded() const { return m_impl != nullptr; }
	
	// Static helper to load a map file with proper path
	static HCMap* LoadMapFile(const std::string& zone_name, const std::string& maps_path);

private:
	struct impl;
	impl *m_impl;
};