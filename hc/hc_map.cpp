#include "hc_map.h"
#include "raycast_mesh.h"
#include <glm/glm.hpp>
#include <fmt/format.h>
#include <iostream>
#include <fstream>
#include <memory>
#include <cstdio>
#include <vector>

// Forward declaration from raycast_mesh.cpp
extern RaycastMesh* createRaycastMesh(uint32_t vcount, const float *vertices, uint32_t tcount,
                                      const uint32_t *indices, uint32_t maxDepth,
                                      uint32_t minLeafSize, float minAxisSize);

struct HCMap::impl {
	RaycastMesh* rm;
	
	impl() : rm(nullptr) {}
	~impl() { 
		if (rm) {
			rm->release();
		}
	}
};

HCMap::HCMap() : m_impl(nullptr) {
}

HCMap::~HCMap() {
	delete m_impl;
}

bool HCMap::Load(const std::string& filename) {
	FILE *f = fopen(filename.c_str(), "rb");
	if (!f) {
		std::cout << fmt::format("[ERROR] Unable to open map file: {}", filename) << std::endl;
		return false;
	}

	// Read version
	uint32_t version;
	if (fread(&version, sizeof(version), 1, f) != 1) {
		fclose(f);
		return false;
	}

	if (m_impl) {
		delete m_impl;
	}
	m_impl = new impl;

	bool loaded = false;
	
	if (version == 0x01000000) {
		// V1 format
		uint32_t face_count;
		uint16_t node_count;
		uint32_t facelist_count;

		if (fread(&face_count, sizeof(face_count), 1, f) != 1) {
			fclose(f);
			return false;
		}

		if (fread(&node_count, sizeof(node_count), 1, f) != 1) {
			fclose(f);
			return false;
		}

		if (fread(&facelist_count, sizeof(facelist_count), 1, f) != 1) {
			fclose(f);
			return false;
		}

		// Read vertices
		std::vector<glm::vec3> verts;
		verts.resize(node_count);
		if (fread(&verts[0], sizeof(glm::vec3), node_count, f) != (size_t)node_count) {
			fclose(f);
			return false;
		}

		// Read faces
		std::vector<uint32_t> indices;
		indices.resize(face_count * 3);
		
		for (uint32_t i = 0; i < face_count; ++i) {
			uint32_t a, b, c;
			uint32_t nx, ny, nz;
			if (fread(&a, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
			if (fread(&b, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
			if (fread(&c, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
			if (fread(&nx, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
			if (fread(&ny, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
			if (fread(&nz, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }

			indices[i * 3] = a;
			indices[i * 3 + 1] = b;
			indices[i * 3 + 2] = c;
		}

		// Create RaycastMesh using factory function
		m_impl->rm = createRaycastMesh(verts.size(), &verts[0].x, face_count, &indices[0]);
		if (m_impl->rm) {
			loaded = true;
		}
		
		std::cout << fmt::format("[INFO] Loaded V1 map: {} vertices, {} faces", node_count, face_count) << std::endl;
	}
	else if (version == 0x02000000) {
		// V2 format
		uint32_t face_count;
		uint32_t node_count;
		uint32_t facelist_count;

		if (fread(&face_count, sizeof(face_count), 1, f) != 1) {
			fclose(f);
			return false;
		}

		if (fread(&node_count, sizeof(node_count), 1, f) != 1) {
			fclose(f);
			return false;
		}

		if (fread(&facelist_count, sizeof(facelist_count), 1, f) != 1) {
			fclose(f);
			return false;
		}

		// Read vertices
		std::vector<glm::vec3> verts;
		verts.resize(node_count);
		if (fread(&verts[0], sizeof(glm::vec3), node_count, f) != (size_t)node_count) {
			fclose(f);
			return false;
		}

		// Read faces
		std::vector<uint32_t> indices;
		indices.resize(face_count * 3);
		
		for (uint32_t i = 0; i < face_count; ++i) {
			uint32_t a, b, c;
			uint32_t flags;
			if (fread(&a, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
			if (fread(&b, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
			if (fread(&c, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }
			if (fread(&flags, sizeof(uint32_t), 1, f) != 1) { fclose(f); return false; }

			indices[i * 3] = a;
			indices[i * 3 + 1] = b;
			indices[i * 3 + 2] = c;
		}

		// Create RaycastMesh using factory function
		m_impl->rm = createRaycastMesh(verts.size(), &verts[0].x, face_count, &indices[0]);
		if (m_impl->rm) {
			loaded = true;
		}
		
		std::cout << fmt::format("[INFO] Loaded V2 map: {} vertices, {} faces", node_count, face_count) << std::endl;
	}
	else {
		std::cout << fmt::format("[ERROR] Unknown map version: 0x{:08x}", version) << std::endl;
	}

	fclose(f);
	
	if (!loaded) {
		delete m_impl;
		m_impl = nullptr;
		return false;
	}

	return true;
}

float HCMap::FindBestZ(const glm::vec3 &start, glm::vec3 *result) const {
	if (!m_impl || !m_impl->rm) {
		return BEST_Z_INVALID;
	}

	glm::vec3 tmp;
	if (!result) {
		result = &tmp;
	}

	// Cast ray down from start position
	glm::vec3 from(start.x, start.y, start.z + 10.0f); // Start slightly above
	glm::vec3 to(start.x, start.y, BEST_Z_INVALID);
	float hit_distance;
	bool hit = false;

	hit = m_impl->rm->raycast((const RmReal*)&from, (const RmReal*)&to, (RmReal*)result, nullptr, &hit_distance);
	
	if (hit) {
		return result->z;
	}

	// Try casting ray up if nothing found below
	to.z = -BEST_Z_INVALID;
	hit = m_impl->rm->raycast((const RmReal*)&from, (const RmReal*)&to, (RmReal*)result, nullptr, &hit_distance);
	
	if (hit) {
		return result->z;
	}

	return BEST_Z_INVALID;
}

HCMap* HCMap::LoadMapFile(const std::string& zone_name, const std::string& maps_path) {
	std::string filename = fmt::format("{}/base/{}.map", maps_path, zone_name);
	
	HCMap* map = new HCMap();
	if (map->Load(filename)) {
		std::cout << fmt::format("[INFO] Successfully loaded map for zone: {}", zone_name) << std::endl;
		return map;
	}
	
	delete map;
	return nullptr;
}