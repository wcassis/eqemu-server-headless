#pragma once

// Stub zone.h for headless client
// This provides minimal definitions needed by pathfinding code

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>

class IPathfinder;
class WaterMap;
class Map;
class NPC;

// Random number generator stub
#include <random>
class Random {
public:
    int Int(int min, int max) { 
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }
    
    double Real(double min, double max) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(min, max);
        return dis(gen);
    }
};

// Newzone data structure
struct NewZone_Struct {
    float underworld = -10000.0f;
    float max_z = 10000.0f;
};

// Minimal Zone class stub
class Zone {
public:
    // Pathfinding support
    std::unique_ptr<IPathfinder> pathing;
    WaterMap* watermap = nullptr;
    Map* zonemap = nullptr;
    Random random;
    NewZone_Struct newzone_data;
    
    bool HasMap() const { return zonemap != nullptr; }
    bool HasWaterMap() const { return watermap != nullptr; }
    
    const char* GetShortName() const { return "unknown"; }
    const char* GetLongName() const { return "Unknown Zone"; }
};

// Global zone pointer (will be nullptr in hc)
extern Zone* zone;

// Entity list stub
class EntityList {
public:
    const std::map<uint16_t, NPC*>& GetNPCList() { 
        static std::map<uint16_t, NPC*> empty_list;
        return empty_list; 
    }
    void AddNPC(NPC* npc, bool send_spawn_packet, bool dont_queue) {}
};
extern EntityList entity_list;

// NPC related enums and types
namespace Gender {
    enum Type {
        Neuter = 0
    };
}

enum class GravityBehavior {
    Flying = 0
};

struct NPCType {
    char name[64];
    char lastname[32];
    int current_hp;
    int max_hp;
    int race;
    int class_;
    int gender;
    int level;
    int bodytype;
    int hidemelee;
    int luclinface;
    int helmtexture;
    int size;
    int runspeed;
    int ignore_despawn;
    int skip_auto_scale;
    int STR, STA, DEX, AGI, INT, WIS, CHA;
    int d_melee_texture1;
    int d_melee_texture2;
    int merchanttype;
    bool show_name;
    int findable;
    int loottable_id;
    int texture;
    int light;
    int deity;
    int npc_id;
};

// NPC stub
class NPC {
public:
    NPC(NPCType* type, void* spawn, const glm::vec4& pos, GravityBehavior grav) {}
    static void SpawnNPC(const char* name, const glm::vec4& pos) {}
    const char* GetName() const { return "NPC"; }
    void Depop() {}
    void GiveNPCTypeData(NPCType* type) {}
};

// Rules system stub
#define RuleI(category, rule) 0
#define RuleR(category, rule) 0.0f
#define RuleB(category, rule) false