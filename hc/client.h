#pragma once

// Stub client.h for headless client
// This provides minimal definitions needed by pathfinding code

#include <glm/glm.hpp>
#include <string>
#include <vector>

// Minimal Mob class stub
class Mob {
public:
    float GetX() const { return 0.0f; }
    float GetY() const { return 0.0f; }
    float GetZ() const { return 0.0f; }
};

// Chat channels
namespace Chat {
    enum Type {
        White = 0
    };
}

// FindPerson point structure
struct FindPerson_Point {
    float x, y, z;
};

// Minimal Client class stub
class Client {
public:
    const char* GetCleanName() const { return "HeadlessClient"; }
    float GetX() const { return 0.0f; }
    float GetY() const { return 0.0f; }
    float GetZ() const { return 0.0f; }
    glm::vec3 GetPosition() const { return glm::vec3(0.0f, 0.0f, 0.0f); }
    Mob* GetTarget() const { return nullptr; }
    void Message(int channel, const char* fmt, ...) {}
    void SendPathPacket(const std::vector<FindPerson_Point>& points) {}
};

// Separator for command parsing
class Seperator {
public:
    char arg[10][256];
};

// Logging stubs
#define LogDebug(...) ((void)0)
#define LogError(...) ((void)0)
#define LogPositionUpdate(...) ((void)0)
#define LogInfo(...) ((void)0)