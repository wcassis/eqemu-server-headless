#include "../common/event/event_loop.h"
#include "../common/eqemu_logsys.h"
#include "../common/crash.h"
#include "../common/platform.h"
#include "../common/json_config.h"
#include "../common/path_manager.h"
#include <thread>
#include <iostream>
#include <sstream>
#include <atomic>
#include <queue>
#include <mutex>
#include <unistd.h>

#include "eq.h"

EQEmuLogSys Log;
EQEmuLogSys LogSys;
PathManager path;

void ProcessCommand(const std::string& cmd, std::vector<std::unique_ptr<EverQuest>>& eq_list, std::atomic<bool>& running) {
	// Debug output for received command
	if (EverQuest::GetDebugLevel() >= 1) {
		std::cout << fmt::format("[DEBUG] Processing command: '{}'", cmd) << std::endl;
	}
	
	// Simple command parser
	std::istringstream iss(cmd);
	std::string command;
	iss >> command;
	
	if (EverQuest::GetDebugLevel() >= 2) {
		auto pos = iss.tellg();
		std::string remaining = (pos == -1) ? "" : iss.str().substr(pos);
		std::cout << fmt::format("[DEBUG] Parsed command: '{}', remaining: '{}'", 
			command, remaining) << std::endl;
	}
	
	// Commands that can be executed without being zoned in
	if (command == "help") {
		std::cout << "Available commands:\n";
		std::cout << "  say <message>              - Say message in current zone\n";
		std::cout << "  tell <player> <message>    - Send tell to player\n";
		std::cout << "  shout <message>            - Shout message (zone-wide)\n";
		std::cout << "  ooc <message>              - OOC message (cross-zone)\n";
		std::cout << "  auction <message>          - Auction message (cross-zone)\n";
		std::cout << "  move <x> <y> <z>           - Move to coordinates\n";
		std::cout << "  moveto <entity>            - Move to named entity\n";
		std::cout << "  follow <entity>            - Follow named entity\n";
		std::cout << "  stopfollow                 - Stop following\n";
		std::cout << "  walk                       - Set movement speed to walk\n";
		std::cout << "  run                        - Set movement speed to run\n";
		std::cout << "  face <x> <y> <z>           - Face coordinates\n";
		std::cout << "  face <entity>              - Face named entity\n";
		std::cout << "  turn <degrees>             - Turn to heading (0=N, 90=E, 180=S, 270=W)\n";
		std::cout << "  loc                        - Show current location\n";
		std::cout << "  list [search]              - List nearby entities (optional: filter by name)\n";
		std::cout << "  pathfinding <on|off>       - Toggle pathfinding (default: on)\n";
		std::cout << "  debug <level>              - Set debug level (0-3)\n";
		std::cout << "  quit                       - Exit program\n";
		return;
	}
	else if (command == "quit" || command == "exit") {
		running.store(false);
		return;
	}
	else if (command == "debug") {
		int level;
		if (iss >> level) {
			EverQuest::SetDebugLevel(level);
			std::cout << fmt::format("Debug level set to {}\n", level);
		}
		return;
	}
	
	// All other commands require being zoned in
	if (!eq_list.empty()) {
		// Send commands to first client for now
		auto& eq = eq_list[0];
		
		if (EverQuest::GetDebugLevel() >= 1) {
			std::cout << fmt::format("[DEBUG] Executing command '{}' on EverQuest client", command) << std::endl;
		}
		
		if (command == "say") {
			std::string message;
			std::getline(iss, message);
			if (!message.empty() && message[0] == ' ') message = message.substr(1);
			eq->SendChatMessage(message, "say");
		}
		else if (command == "tell") {
			std::string target, message;
			iss >> target;
			std::getline(iss, message);
			if (!message.empty() && message[0] == ' ') message = message.substr(1);
			eq->SendChatMessage(message, "tell", target);
		}
		else if (command == "shout") {
			std::string message;
			std::getline(iss, message);
			if (!message.empty() && message[0] == ' ') message = message.substr(1);
			eq->SendChatMessage(message, "shout");
		}
		else if (command == "ooc") {
			std::string message;
			std::getline(iss, message);
			if (!message.empty() && message[0] == ' ') message = message.substr(1);
			eq->SendChatMessage(message, "ooc");
		}
		else if (command == "auction") {
			std::string message;
			std::getline(iss, message);
			if (!message.empty() && message[0] == ' ') message = message.substr(1);
			eq->SendChatMessage(message, "auction");
		}
		else if (command == "move") {
			float x, y, z;
			if (iss >> x >> y >> z) {
				if (EverQuest::GetDebugLevel() >= 1) {
					std::cout << fmt::format("[DEBUG] Executing move command to ({}, {}, {})", x, y, z) << std::endl;
				}
				eq->Move(x, y, z);
			} else {
				std::cout << "Usage: move <x> <y> <z>\n";
			}
		}
		else if (command == "moveto") {
			std::string entity;
			std::getline(iss, entity);
			if (!entity.empty() && entity[0] == ' ') entity = entity.substr(1);
			if (!entity.empty()) {
				eq->MoveToEntity(entity);
			} else {
				std::cout << "Usage: moveto <entity_name>\n";
			}
		}
		else if (command == "follow") {
			std::string entity;
			std::getline(iss, entity);
			if (!entity.empty() && entity[0] == ' ') entity = entity.substr(1);
			if (!entity.empty()) {
				eq->Follow(entity);
			} else {
				std::cout << "Usage: follow <entity_name>\n";
			}
		}
		else if (command == "stopfollow") {
			eq->StopFollow();
		}
		else if (command == "face") {
			std::string arg;
			if (iss >> arg) {
				// Check if it's coordinates or entity name
				float x, y, z;
				std::istringstream coord_stream(arg);
				if (coord_stream >> x && iss >> y >> z) {
					eq->Face(x, y, z);
				} else {
					// It's an entity name, read the rest of the line
					std::string rest;
					std::getline(iss, rest);
					std::string full_name = arg + rest;
					eq->FaceEntity(full_name);
				}
			} else {
				std::cout << "Usage: face <x> <y> <z> or face <entity_name>\n";
			}
		}
		else if (command == "turn") {
			float degrees;
			if (iss >> degrees) {
				// Convert degrees to EQ heading (0-512 scale)
				// In EQ: 0 = North, 128 = East, 256 = South, 384 = West
				float heading = degrees * 512.0f / 360.0f;
				// Normalize to 0-512
				while (heading < 0.0f) heading += 512.0f;
				while (heading >= 512.0f) heading -= 512.0f;
				eq->SetHeading(heading);
				eq->SendPositionUpdate();
				std::cout << fmt::format("Turned to heading {:.1f} degrees (EQ heading: {:.1f})\n", degrees, heading);
			} else {
				std::cout << "Usage: turn <degrees> (0=North, 90=East, 180=South, 270=West)\n";
			}
		}
		else if (command == "loc") {
			auto pos = eq->GetPosition();
			std::cout << fmt::format("Current position: ({:.2f}, {:.2f}, {:.2f}) heading {:.1f}\n", 
				pos.x, pos.y, pos.z, eq->GetHeading());
		}
		else if (command == "list") {
			std::string search;
			std::getline(iss, search);
			if (!search.empty() && search[0] == ' ') search = search.substr(1);
			eq->ListEntities(search);
		}
		else if (command == "walk") {
			eq->SetMoveSpeed(30.0f);  // Set to walk speed
			std::cout << "Movement speed set to walk\n";
		}
		else if (command == "run") {
			eq->SetMoveSpeed(70.0f);  // Set to run speed
			std::cout << "Movement speed set to run\n";
		}
		else if (command == "pathfinding") {
			std::string state;
			if (iss >> state) {
				if (state == "on" || state == "true" || state == "1") {
					eq->SetPathfinding(true);
					std::cout << "Pathfinding enabled\n";
				} else if (state == "off" || state == "false" || state == "0") {
					eq->SetPathfinding(false);
					std::cout << "Pathfinding disabled\n";
				} else {
					std::cout << fmt::format("Current pathfinding state: {}\n", 
						eq->IsPathfindingEnabled() ? "enabled" : "disabled");
				}
			} else {
				std::cout << fmt::format("Current pathfinding state: {}\n", 
					eq->IsPathfindingEnabled() ? "enabled" : "disabled");
			}
		}
		else {
			if (EverQuest::GetDebugLevel() >= 1) {
				std::cout << fmt::format("[DEBUG] Unknown command received: '{}'", command) << std::endl;
			}
			std::cout << fmt::format("Unknown command: '{}'. Type 'help' for commands.\n", command);
		}
	}
}

int main(int argc, char *argv[]) {
	RegisterExecutablePlatform(ExePlatformHC);
	Log.LoadLogSettingsDefaults();
	set_exception_handler();

	// Initialize path manager to find server files
	path.LoadPaths();

	// Parse command line arguments
	int debug_level = 0;
	std::string config_file = "hc_test1.json";
	bool pathfinding_enabled = true;
	
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "--debug" || arg == "-d") {
			if (i + 1 < argc) {
				debug_level = std::atoi(argv[++i]);
			}
		} else if (arg == "--config" || arg == "-c") {
			if (i + 1 < argc) {
				config_file = argv[++i];
			}
		} else if (arg == "--no-pathfinding" || arg == "-np") {
			pathfinding_enabled = false;
		} else if (arg == "--help" || arg == "-h") {
			std::cout << "Usage: " << argv[0] << " [options]\n";
			std::cout << "Options:\n";
			std::cout << "  -d, --debug <level>      Set debug level (0-3)\n";
			std::cout << "  -c, --config <file>      Set config file (default: hc_test1.json)\n";
			std::cout << "  -np, --no-pathfinding    Disable navmesh pathfinding\n";
			std::cout << "  -h, --help               Show this help message\n";
			return 0;
		}
	}

	EverQuest::SetDebugLevel(debug_level);
	std::cout << fmt::format("Starting EQEmu Headless Client with debug level {}, config file: {}, pathfinding: {}\n", 
		debug_level, config_file, pathfinding_enabled ? "enabled" : "disabled");

	auto config = EQ::JsonConfigFile::Load(config_file);
	auto config_handle = config.RawHandle();

	std::vector<std::unique_ptr<EverQuest>> eq_list;

	try {
		for (int i = 0; i < config_handle.size(); ++i) {
			auto c = config_handle[i];

			auto host = c["host"].asString();
			auto port = c["port"].asInt();
			auto user = c["user"].asString();
			auto pass = c["pass"].asString();
			auto server = c["server"].asString();
			auto character = c["character"].asString();
			
			// Read optional navmesh_path from config
			std::string navmesh_path;
			if (c.isMember("navmesh_path")) {
				navmesh_path = c["navmesh_path"].asString();
			}
			
			// Read optional maps_path from config
			std::string maps_path;
			if (c.isMember("maps_path")) {
				maps_path = c["maps_path"].asString();
			}
			
			std::cout << fmt::format("Connecting to {}:{} as Account '{}' to Server '{}' under Character '{}'\n", host, port, user, server, character);
			if (!navmesh_path.empty()) {
				std::cout << fmt::format("Using navmesh path: {}\n", navmesh_path);
			}
			if (!maps_path.empty()) {
				std::cout << fmt::format("Using maps path: {}\n", maps_path);
			}

			auto eq = std::unique_ptr<EverQuest>(new EverQuest(host, port, user, pass, server, character));
			eq->SetPathfinding(pathfinding_enabled);
			if (!navmesh_path.empty()) {
				eq->SetNavmeshPath(navmesh_path);
			}
			if (!maps_path.empty()) {
				eq->SetMapsPath(maps_path);
			}
			eq_list.push_back(std::move(eq));
		}
	}
	catch (std::exception &ex) {
		std::cout << fmt::format("Error parsing config file: {}\n", ex.what());
		return 0;
	}

	// Start command input thread
	std::atomic<bool> running(true);
	std::queue<std::string> command_queue;
	std::mutex command_mutex;
	std::atomic<bool> command_processing_done(false);
	bool fully_connected_announced = false;
	
	std::thread input_thread([&]() {
		if (EverQuest::GetDebugLevel() >= 1) {
			std::cout << "[DEBUG] Input thread started, waiting for commands..." << std::endl;
		}
		std::string line;
		bool stdin_closed = false;
		while (running.load()) {
			if (!stdin_closed && std::getline(std::cin, line)) {
				if (!line.empty()) {
					if (EverQuest::GetDebugLevel() >= 2) {
						std::cout << fmt::format("[DEBUG] Input thread received: '{}'", line) << std::endl;
					}
					std::lock_guard<std::mutex> lock(command_mutex);
					command_queue.push(line);
					if (EverQuest::GetDebugLevel() >= 2) {
						std::cout << fmt::format("[DEBUG] Command queued, queue size: {}", command_queue.size()) << std::endl;
					}
				}
			} else if (!stdin_closed) {
				// stdin closed (EOF reached)
				stdin_closed = true;
				if (EverQuest::GetDebugLevel() >= 1) {
					std::cout << "[DEBUG] Input thread: stdin closed (EOF), continuing to run..." << std::endl;
				}
				// Continue running to keep the thread alive
			}
			
			// Small sleep to prevent busy waiting when stdin is closed
			if (stdin_closed) {
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		if (EverQuest::GetDebugLevel() >= 1) {
			std::cout << "[DEBUG] Input thread terminating" << std::endl;
		}
	});
	
	// Start command processing thread
	std::vector<std::string> buffered_commands;
	std::thread command_thread([&]() {
		if (EverQuest::GetDebugLevel() >= 1) {
			std::cout << "[DEBUG] Command processing thread started" << std::endl;
		}
		
		// Wait for zone connection
		while (running.load() && (eq_list.empty() || !eq_list[0]->IsFullyZonedIn())) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		
		if (!running.load()) {
			command_processing_done.store(true);
			return;
		}
		
		if (EverQuest::GetDebugLevel() >= 1) {
			std::cout << "[DEBUG] Zone connection established, processing commands" << std::endl;
		}
		
		// Process all buffered commands and new commands sequentially
		while (running.load()) {
			std::string cmd;
			{
				std::lock_guard<std::mutex> lock(command_mutex);
				if (!command_queue.empty()) {
					cmd = command_queue.front();
					command_queue.pop();
				}
			}
			
			if (!cmd.empty()) {
				if (EverQuest::GetDebugLevel() >= 1) {
					std::cout << fmt::format("[DEBUG] Command thread processing: '{}'", cmd) << std::endl;
				}
				
				// Process command (this will block for movement commands)
				ProcessCommand(cmd, eq_list, running);
				
				// If quit was called, exit
				if (!running.load()) {
					break;
				}
			} else {
				// No commands to process, wait a bit
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
		
		command_processing_done.store(true);
		if (EverQuest::GetDebugLevel() >= 1) {
			std::cout << "[DEBUG] Command processing thread terminating" << std::endl;
		}
	});
	
	std::cout << "\nHeadless client ready. Type 'help' for commands.\n";
	std::cout << "Waiting for zone connection before processing commands...\n";
	
	// Debug info about input source
	if (EverQuest::GetDebugLevel() >= 1) {
		if (isatty(fileno(stdin))) {
			std::cout << "[DEBUG] Input source: Interactive terminal (TTY)" << std::endl;
		} else {
			std::cout << "[DEBUG] Input source: Pipe or file redirection" << std::endl;
		}
	}
	
	int loop_count = 0;
	auto last_update = std::chrono::steady_clock::now();
	
	for (;;) {
		EQ::EventLoop::Get().Process();
		
		// Check if any client is fully connected
		bool any_connected = false;
		if (!eq_list.empty()) {
			any_connected = eq_list[0]->IsFullyZonedIn();
		}
		
		// Announce when fully connected
		if (any_connected && !fully_connected_announced) {
			std::cout << "Fully connected to zone! Processing commands now." << std::endl;
			fully_connected_announced = true;
		}
		
		// Command processing is now handled by the command thread
		
		// Update movement for all clients
		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count() >= 16) { // ~60 FPS
			for (auto& eq : eq_list) {
				eq->UpdateMovement();
			}
			last_update = now;
		}
		
		if (EverQuest::GetDebugLevel() >= 3 && ++loop_count % 1000 == 0) {
			std::cout << "." << std::flush;
		}
		
		if (!running.load()) {
			break;
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	
	// Wait for threads to complete
	input_thread.join();
	command_thread.join();
	
	return 0;
}
