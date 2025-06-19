#pragma once

#include "../common/eqemu_logsys.h"
#include "../common/net/daybreak_connection.h"
#include "../common/net/eqstream.h"
#include "../common/event/timer.h"
#include <openssl/des.h>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <chrono>

// Forward declarations
class IPathfinder;
class HCMap;

// Titanium login opcodes
enum TitaniumLoginOpcodes {
	HC_OP_SessionReady = 0x0001,
	HC_OP_Login = 0x0002,
	HC_OP_ServerListRequest = 0x0004,
	HC_OP_PlayEverquestRequest = 0x000d,
	HC_OP_ChatMessage = 0x0016,
	HC_OP_LoginAccepted = 0x0017,
	HC_OP_ServerListResponse = 0x0018,
	HC_OP_PlayEverquestResponse = 0x0021
};

// Titanium world opcodes
enum TitaniumWorldOpcodes {
	HC_OP_SendLoginInfo = 0x4dd0,
	HC_OP_GuildsList = 0x6957,
	HC_OP_LogServer = 0x0fa6,
	HC_OP_ApproveWorld = 0x3c25,
	HC_OP_EnterWorld = 0x7cba,
	HC_OP_PostEnterWorld = 0x52a4,
	HC_OP_ExpansionInfo = 0x04ec,
	HC_OP_SendCharInfo = 0x4513,
	HC_OP_World_Client_CRC1 = 0x5072,
	HC_OP_World_Client_CRC2 = 0x5b18,
	HC_OP_AckPacket = 0x7752,
	HC_OP_WorldClientReady = 0x5e99,
	HC_OP_MOTD = 0x024d,
	HC_OP_SetChatServer = 0x00d7,
	HC_OP_SetChatServer2 = 0x6536,
	HC_OP_ZoneServerInfo = 0x61b6,
	HC_OP_WorldComplete = 0x509d
};

// Titanium zone opcodes
enum TitaniumZoneOpcodes {
	HC_OP_ZoneEntry = 0x7213,
	HC_OP_NewZone = 0x0920,
	HC_OP_ReqClientSpawn = 0x0322,
	HC_OP_ZoneSpawns = 0x2e78,
	HC_OP_SendZonepoints = 0x3eba,
	HC_OP_ReqNewZone = 0x7ac5,
	HC_OP_PlayerProfile = 0x75df,
	HC_OP_CharInventory = 0x5394,
	HC_OP_TimeOfDay = 0x1580,
	HC_OP_SpawnDoor = 0x4c24,
	HC_OP_ClientReady = 0x5e20,
	HC_OP_ZoneChange = 0x5dd8,
	HC_OP_SetServerFilter = 0x6563,
	HC_OP_GroundSpawn = 0x0f47,
	HC_OP_Weather = 0x254d,
	HC_OP_ClientUpdate = 0x14cb,
	HC_OP_SpawnAppearance = 0x7c32,
	HC_OP_NewSpawn = 0x1860,
	HC_OP_DeleteSpawn = 0x55bc,
	HC_OP_MobHealth = 0x0695,
	HC_OP_HPUpdate = 0x3bcf,
	HC_OP_TributeUpdate = 0x5639,
	HC_OP_TributeTimer = 0x4665,
	HC_OP_SendAATable = 0x367d,
	HC_OP_UpdateAA = 0x5966,
	HC_OP_RespondAA = 0x3af4,
	HC_OP_SendTributes = 0x067a,
	HC_OP_TributeInfo = 0x152d,
	HC_OP_RequestGuildTributes = 0x5e3a,
	HC_OP_SendGuildTributes = 0x5e3d,
	HC_OP_SendAAStats = 0x5996,
	HC_OP_SendExpZonein = 0x0587,
	HC_OP_WorldObjectsSent = 0x0000,  // No Titanium mapping, server sends 0x0000
	HC_OP_ExpUpdate = 0x5ecd,
	HC_OP_RaidUpdate = 0x1f21,
	HC_OP_GuildMOTD = 0x475a,
	HC_OP_ChannelMessage = 0x1004,
	HC_OP_WearChange = 0x7441,
	HC_OP_MoveDoor = 0x700d,
	HC_OP_CompletedTasks = 0x76a2,
	HC_OP_DzCompass = 0x28aa,
	HC_OP_DzExpeditionLockoutTimers = 0x7c12,
	HC_OP_BeginCast = 0x3990,
	HC_OP_ManaChange = 0x4839,
	HC_OP_FormattedMessage = 0x5a48,
	HC_OP_PlayerStateAdd = 0x63da,
	HC_OP_Death = 0x6160,
	HC_OP_PlayerStateRemove = 0x381d,
	HC_OP_Stamina = 0x7a83
};

// UCS (Universal Chat Service) opcodes - legacy 1-byte opcodes
enum UCSClientOpcodes {
	HC_OP_UCS_MailLogin = 0x00,
	HC_OP_UCS_ChatMessage = 0x01,
	HC_OP_UCS_ChatJoin = 0x02,
	HC_OP_UCS_ChatLeave = 0x03,
	HC_OP_UCS_ChatWho = 0x04,
	HC_OP_UCS_ChatInvite = 0x05,
	HC_OP_UCS_ChatModerate = 0x06,
	HC_OP_UCS_ChatGrant = 0x07,
	HC_OP_UCS_ChatVoice = 0x08,
	HC_OP_UCS_ChatKick = 0x09,
	HC_OP_UCS_ChatSetOwner = 0x0a,
	HC_OP_UCS_ChatOPList = 0x0b,
	HC_OP_UCS_ChatList = 0x0c,
	HC_OP_UCS_MailHeaderCount = 0x20,
	HC_OP_UCS_MailHeader = 0x21,
	HC_OP_UCS_MailGetBody = 0x22,
	HC_OP_UCS_MailSendBody = 0x23,
	HC_OP_UCS_MailDeleteMsg = 0x24,
	HC_OP_UCS_MailNew = 0x25,
	HC_OP_UCS_Buddy = 0x40,
	HC_OP_UCS_Ignore = 0x41
};

struct WorldServer
{
	std::string long_name;
	std::string address;
	int type;
	std::string lang;
	std::string region;
	int status;
	int players;
};

struct Entity
{
	uint16_t spawn_id;
	std::string name;
	float x, y, z;
	float heading;
	uint8_t level;
	uint8_t class_id;
	uint16_t race_id;
	uint8_t gender;
	uint32_t guild_id;
	uint8_t animation;
	uint8_t hp_percent;
	uint16_t cur_mana;
	uint16_t max_mana;
	
	// Movement tracking
	float delta_x, delta_y, delta_z;
	float delta_heading;
	uint32_t last_update_time;
};

// Chat channel types (from server's eq_constants.h ChatChannelNames enum)
enum ChatChannelType {
	CHAT_CHANNEL_GUILD = 0,
	CHAT_CHANNEL_GROUP = 2,
	CHAT_CHANNEL_SHOUT = 3,
	CHAT_CHANNEL_AUCTION = 4,
	CHAT_CHANNEL_OOC = 5,
	CHAT_CHANNEL_BROADCAST = 6,
	CHAT_CHANNEL_TELL = 7,
	CHAT_CHANNEL_SAY = 8,
	CHAT_CHANNEL_PETITION = 10,
	CHAT_CHANNEL_GMSAY = 11,
	CHAT_CHANNEL_RAID = 15,
	CHAT_CHANNEL_EMOTE = 22
};

// Movement animation types
enum AnimationType {
	ANIM_STAND = 0,
	ANIM_WALK = 1,
	ANIM_RUN = 27,     // Real clients use 27 for run animation
	ANIM_CROUCH_WALK = 3,
	ANIM_JUMP = 4,
	ANIM_FALL = 5,
	ANIM_SWIM_IDLE = 6,
	ANIM_SWIM = 7,
	ANIM_SWIM_ATTACK = 8,
	ANIM_FLY = 9
};

// Client position update packet uses bit-packed format
// Format: spawn_id (2 bytes) + 20 bytes of bit-packed data:
// field1: delta_heading:10, x_pos:19, padding:3
// field2: y_pos:19, animation:10, padding:3
// field3: z_pos:19, delta_y:13
// field4: delta_x:13, heading:12, padding:7
// field5: delta_z:13, padding:19

class EverQuest
{
public:
	EverQuest(const std::string &host, int port, const std::string &user, const std::string &pass, const std::string &server, const std::string &character);
	~EverQuest();

	static void SetDebugLevel(int level) { s_debug_level = level; }
	static int GetDebugLevel() { return s_debug_level; }
	
	// Public chat interface
	void SendChatMessage(const std::string &message, const std::string &channel_name, const std::string &target = "");
	
	// Public movement interface - all movement commands block until complete
	void Move(float x, float y, float z);
	void MoveToEntity(const std::string &name);
	void Follow(const std::string &name);
	void StopFollow();
	void Face(float x, float y, float z);
	void FaceEntity(const std::string &name);
	void SetHeading(float heading) { m_heading = heading; }
	void UpdateMovement();
	void SendPositionUpdate();
	glm::vec3 GetPosition() const { return glm::vec3(m_x, m_y, m_z); }
	float GetHeading() const { return m_heading; }
	bool IsMoving() const { return m_is_moving; }
	bool IsFullyZonedIn() const { return m_zone_connected && m_client_ready_sent; }
	void ListEntities(const std::string& search = "") const;
	void SetPathfinding(bool enabled) { m_use_pathfinding = enabled; }
	bool IsPathfindingEnabled() const { return m_use_pathfinding; }
	bool HasReachedDestination() const;
	void SetMoveSpeed(float speed) { m_move_speed = speed; }
	void SetNavmeshPath(const std::string& path) { m_navmesh_path = path; }
	void SetMapsPath(const std::string& path) { m_maps_path = path; }

private:
	// Utility functions
	static void DumpPacket(const std::string &prefix, uint16_t opcode, const EQ::Net::Packet &p);
	static void DumpPacket(const std::string &prefix, uint16_t opcode, const uint8_t *data, size_t size);
	static std::string GetOpcodeName(uint16_t opcode);

	//Login
	void LoginOnNewConnection(std::shared_ptr<EQ::Net::DaybreakConnection> connection);
	void LoginOnStatusChangeReconnectEnabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to);
	void LoginOnStatusChangeReconnectDisabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to);
	void LoginOnPacketRecv(std::shared_ptr<EQ::Net::DaybreakConnection> conn, const EQ::Net::Packet &p);

	void LoginSendSessionReady();
	void LoginSendLogin();
	void LoginSendServerRequest();
	void LoginSendPlayRequest(uint32_t id);
	void LoginProcessLoginResponse(const EQ::Net::Packet &p);
	void LoginProcessServerPacketList(const EQ::Net::Packet &p);
	void LoginProcessServerPlayResponse(const EQ::Net::Packet &p);

	void LoginDisableReconnect();

	std::unique_ptr<EQ::Net::DaybreakConnectionManager> m_login_connection_manager;
	std::shared_ptr<EQ::Net::DaybreakConnection> m_login_connection;
	std::map<uint32_t, WorldServer> m_world_servers;

	//World
	void ConnectToWorld(const std::string& world_address);

	void WorldOnNewConnection(std::shared_ptr<EQ::Net::DaybreakConnection> connection);
	void WorldOnStatusChangeReconnectEnabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to);
	void WorldOnStatusChangeReconnectDisabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to);
	void WorldOnPacketRecv(std::shared_ptr<EQ::Net::DaybreakConnection> conn, const EQ::Net::Packet &p);

	void WorldSendSessionReady();
	void WorldSendClientAuth();
	void WorldSendEnterWorld(const std::string &character);
	void WorldSendApproveWorld();
	void WorldSendWorldClientCRC();
	void WorldSendWorldClientReady();
	void WorldSendWorldComplete();

	void WorldProcessCharacterSelect(const EQ::Net::Packet &p);
	void WorldProcessGuildsList(const EQ::Net::Packet &p);
	void WorldProcessLogServer(const EQ::Net::Packet &p);
	void WorldProcessApproveWorld(const EQ::Net::Packet &p);
	void WorldProcessEnterWorld(const EQ::Net::Packet &p);
	void WorldProcessPostEnterWorld(const EQ::Net::Packet &p);
	void WorldProcessExpansionInfo(const EQ::Net::Packet &p);
	void WorldProcessMOTD(const EQ::Net::Packet &p);
	void WorldProcessSetChatServer(const EQ::Net::Packet &p);
	void WorldProcessZoneServerInfo(const EQ::Net::Packet &p);

	std::unique_ptr<EQ::Net::DaybreakConnectionManager> m_world_connection_manager;
	std::shared_ptr<EQ::Net::DaybreakConnection> m_world_connection;

	//Variables
	std::string m_host;
	int m_port;
	std::string m_user;
	std::string m_pass;
	std::string m_server;
	std::string m_character;

	std::string m_key;
	uint32_t m_dbid;

	// Debug level
	static int s_debug_level;

	// Sequence numbers for packets
	uint32_t m_login_sequence = 2;
	
	// World server state
	bool m_world_ready = false;
	bool m_enter_world_sent = false;
	std::string m_zone_server_host;
	uint16_t m_zone_server_port = 0;
	
	// Zone server connection
	void ConnectToZone();
	void ZoneOnNewConnection(std::shared_ptr<EQ::Net::DaybreakConnection> connection);
	void ZoneOnStatusChangeReconnectEnabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to);
	void ZoneOnStatusChangeReconnectDisabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to);
	void ZoneOnPacketRecv(std::shared_ptr<EQ::Net::DaybreakConnection> conn, const EQ::Net::Packet &p);
	
	void ZoneSendStreamIdentify();
	void ZoneSendAckPacket();
	void ZoneSendSessionReady();
	void ZoneSendZoneEntry();
	void ZoneSendReqNewZone();
	void ZoneSendSendAATable();
	void ZoneSendUpdateAA();
	void ZoneSendSendTributes();
	void ZoneSendRequestGuildTributes();
	void ZoneSendSpawnAppearance();
	void ZoneSendReqClientSpawn();
	void ZoneSendSendExpZonein();
	void ZoneSendSetServerFilter();
	void ZoneSendClientReady();
	void ZoneSendChannelMessage(const std::string &message, ChatChannelType channel, const std::string &target = "");
	
	void ZoneProcessNewZone(const EQ::Net::Packet &p);
	void ZoneProcessPlayerProfile(const EQ::Net::Packet &p);
	void ZoneProcessCharInventory(const EQ::Net::Packet &p);
	void ZoneProcessZoneSpawns(const EQ::Net::Packet &p);
	void ZoneProcessTimeOfDay(const EQ::Net::Packet &p);
	void ZoneProcessTributeUpdate(const EQ::Net::Packet &p);
	void ZoneProcessTributeTimer(const EQ::Net::Packet &p);
	void ZoneProcessWeather(const EQ::Net::Packet &p);
	void ZoneProcessSendAATable(const EQ::Net::Packet &p);
	void ZoneProcessRespondAA(const EQ::Net::Packet &p);
	void ZoneProcessTributeInfo(const EQ::Net::Packet &p);
	void ZoneProcessSendGuildTributes(const EQ::Net::Packet &p);
	void ZoneProcessSpawnDoor(const EQ::Net::Packet &p);
	void ZoneProcessGroundSpawn(const EQ::Net::Packet &p);
	void ZoneProcessSendZonepoints(const EQ::Net::Packet &p);
	void ZoneProcessSendAAStats(const EQ::Net::Packet &p);
	void ZoneProcessSendExpZonein(const EQ::Net::Packet &p);
	void ZoneProcessWorldObjectsSent(const EQ::Net::Packet &p);
	void ZoneProcessSpawnAppearance(const EQ::Net::Packet &p);
	void ZoneProcessExpUpdate(const EQ::Net::Packet &p);
	void ZoneProcessRaidUpdate(const EQ::Net::Packet &p);
	void ZoneProcessGuildMOTD(const EQ::Net::Packet &p);
	void ZoneProcessNewSpawn(const EQ::Net::Packet &p);
	void ZoneProcessClientUpdate(const EQ::Net::Packet &p);
	void ZoneProcessDeleteSpawn(const EQ::Net::Packet &p);
	void ZoneProcessMobHealth(const EQ::Net::Packet &p);
	void ZoneProcessHPUpdate(const EQ::Net::Packet &p);
	void ZoneProcessChannelMessage(const EQ::Net::Packet &p);
	void ZoneProcessWearChange(const EQ::Net::Packet &p);
	void ZoneProcessMoveDoor(const EQ::Net::Packet &p);
	void ZoneProcessCompletedTasks(const EQ::Net::Packet &p);
	void ZoneProcessDzCompass(const EQ::Net::Packet &p);
	void ZoneProcessDzExpeditionLockoutTimers(const EQ::Net::Packet &p);
	void ZoneProcessBeginCast(const EQ::Net::Packet &p);
	void ZoneProcessManaChange(const EQ::Net::Packet &p);
	void ZoneProcessFormattedMessage(const EQ::Net::Packet &p);
	void ZoneProcessPlayerStateAdd(const EQ::Net::Packet &p);
	void ZoneProcessDeath(const EQ::Net::Packet &p);
	void ZoneProcessPlayerStateRemove(const EQ::Net::Packet &p);
	void ZoneProcessStamina(const EQ::Net::Packet &p);
	
	std::unique_ptr<EQ::Net::DaybreakConnectionManager> m_zone_connection_manager;
	std::shared_ptr<EQ::Net::DaybreakConnection> m_zone_connection;
	
	// Zone state
	bool m_zone_connected = false;
	bool m_zone_session_established = false;
	bool m_zone_entry_sent = false;
	bool m_weather_received = false;  // Must receive weather before ReqNewZone
	bool m_req_new_zone_sent = false;
	bool m_new_zone_received = false;  // Server acknowledged ReqNewZone
	bool m_aa_table_sent = false;
	bool m_update_aa_sent = false;
	bool m_tributes_sent = false;
	bool m_guild_tributes_sent = false;
	bool m_req_client_spawn_sent = false;
	bool m_spawn_appearance_sent = false;
	bool m_exp_zonein_sent = false;
	bool m_send_exp_zonein_received = false;  // Triggers final packets
	bool m_server_filter_sent = false;
	bool m_client_ready_sent = false;  // Final step - we're fully zoned in
	bool m_client_spawned = false;  // Deprecated - use m_client_ready_sent
	uint32_t m_zone_sequence = 2;
	int m_aa_table_count = 0;
	int m_tribute_count = 0;
	int m_guild_tribute_count = 0;
	
	// Entity tracking
	std::map<uint16_t, Entity> m_entities;
	uint16_t m_my_spawn_id = 0;
	uint16_t m_my_character_id = 0;  // Character database ID for ClientUpdate packets
	int m_character_select_index = -1;  // Index of our character in the select list
	
	// Movement and position tracking
	float m_x = 0.0f;
	float m_y = 0.0f;
	float m_z = 0.0f;
	float m_heading = 0.0f;
	uint16_t m_animation = 0;
	uint32_t m_movement_sequence = 0;
	bool m_is_moving = false;
	
	// Movement target
	float m_target_x = 0.0f;
	float m_target_y = 0.0f;
	float m_target_z = 0.0f;
	float m_move_speed = 70.0f; // Normal run speed (base units per second)
	uint32_t m_last_movement_update = 0;
	std::chrono::steady_clock::time_point m_last_position_update_time;
	
	// Follow target
	std::string m_follow_target;
	float m_follow_distance = 10.0f;
	
	// Pathfinding
	std::unique_ptr<IPathfinder> m_pathfinder;
	std::string m_current_zone_name;
	std::vector<glm::vec3> m_current_path;
	size_t m_current_path_index = 0;
	bool m_use_pathfinding = true;
	std::string m_navmesh_path;
	
	// Map for Z-height calculations
	std::unique_ptr<HCMap> m_zone_map;
	std::string m_maps_path;
	
	// Movement methods
	void MoveTo(float x, float y, float z);
	void MoveToWithPath(float x, float y, float z);
	void StopMovement();
	float CalculateHeading(float x1, float y1, float x2, float y2);
	float CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const;
	float CalculateDistance2D(float x1, float y1, float x2, float y2) const;
	
	// Entity helper methods
	Entity* FindEntityByName(const std::string& name);
	
	// Pathfinding methods
	void LoadPathfinder(const std::string& zone_name);
	bool FindPath(float start_x, float start_y, float start_z, float end_x, float end_y, float end_z);
	void FollowPath();
	
	// Map methods
	void LoadZoneMap(const std::string& zone_name);
	float GetBestZ(float x, float y, float z);
	void FixZ();
	
	// Zone connection flow helpers
	void CheckZoneRequestPhaseComplete();
	
	// UCS (Universal Chat Service) connection
	// TODO: Fix UCS implementation to use correct EQStream API
	/*
	void ConnectToUCS(const std::string& host, uint16_t port);
	void UCSOnNewConnection(std::shared_ptr<EQ::Net::EQStream> stream);
	void UCSOnPacketRecv(const EQ::Net::Packet &p);
	void UCSOnConnectionLost();
	
	void UCSSendMailLogin();
	void UCSSendChatMessage(const std::string &channel, const std::string &message);
	void UCSSendTell(const std::string &to, const std::string &message);
	void UCSSendJoinChannel(const std::string &channel);
	void UCSSendLeaveChannel(const std::string &channel);
	
	void UCSProcessChatMessage(const EQ::Net::Packet &p);
	void UCSProcessMailNew(const EQ::Net::Packet &p);
	void UCSProcessBuddy(const EQ::Net::Packet &p);
	
	std::unique_ptr<EQ::Net::EQStreamManager> m_ucs_stream_manager;
	std::shared_ptr<EQ::Net::EQStream> m_ucs_stream;
	bool m_ucs_connected = false;
	*/
	std::string m_ucs_host;
	uint16_t m_ucs_port = 0;
	std::string m_mail_key;
	
	// World server channel message routing
	void WorldSendChannelMessage(const std::string &channel, const std::string &message, const std::string &target = "");
	void WorldProcessChannelMessage(const EQ::Net::Packet &p);
};