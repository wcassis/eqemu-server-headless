#include "eq.h"
#include "../common/net/dns.h"
#include "pathfinder_interface.h"
#include "hc_map.h"
#include <iomanip>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstring>
#include <thread>
#include <chrono>
#include <map>
#include <tuple>

// Movement constants
const float DEFAULT_RUN_SPEED = 100.0f;  // Increased based on Wimplo's observed speeds
const float DEFAULT_WALK_SPEED = 40.0f;   // Units per second (EQ base walk speed)
const float WALK_SPEED_THRESHOLD = 45.0f;  // Speed threshold for walk vs run animation
const float POSITION_UPDATE_INTERVAL_MS = 50.0f;  // Send updates every 50ms when moving

// Following constants
const float FOLLOW_CLOSE_DISTANCE = 10.0f;   // Stop following when this close
const float FOLLOW_FAR_DISTANCE = 30.0f;     // Full speed when this far
const float FOLLOW_MIN_SPEED_MULT = 0.5f;    // Minimum speed multiplier when close
const float FOLLOW_MAX_SPEED_MULT = 1.5f;    // Maximum speed multiplier when far

// Static member definition
int EverQuest::s_debug_level = 0;

const char* eqcrypt_block(const char *buffer_in, size_t buffer_in_sz, char* buffer_out, bool enc) {
	DES_key_schedule k;
	DES_cblock v;

	memset(&k, 0, sizeof(DES_key_schedule));
	memset(&v, 0, sizeof(DES_cblock));

	if (!enc && buffer_in_sz && buffer_in_sz % 8 != 0) {
		return nullptr;
	}

	DES_ncbc_encrypt((const unsigned char*)buffer_in, (unsigned char*)buffer_out, (long)buffer_in_sz, &k, &v, enc);
	return buffer_out;
}

std::string EverQuest::GetOpcodeName(uint16_t opcode) {
	// Login opcodes
	switch (opcode) {
		case HC_OP_SessionReady: return "HC_OP_SessionReady";
		case HC_OP_Login: return "HC_OP_Login";
		case HC_OP_ServerListRequest: return "HC_OP_ServerListRequest";
		case HC_OP_PlayEverquestRequest: return "HC_OP_PlayEverquestRequest";
		case HC_OP_ChatMessage: return "HC_OP_ChatMessage";
		case HC_OP_LoginAccepted: return "HC_OP_LoginAccepted";
		case HC_OP_ServerListResponse: return "HC_OP_ServerListResponse";
		case HC_OP_PlayEverquestResponse: return "HC_OP_PlayEverquestResponse";
	}
	
	// World opcodes
	switch (opcode) {
		case HC_OP_SendLoginInfo: return "HC_OP_SendLoginInfo";
		case HC_OP_GuildsList: return "HC_OP_GuildsList";
		case HC_OP_LogServer: return "HC_OP_LogServer";
		case HC_OP_ApproveWorld: return "HC_OP_ApproveWorld";
		case HC_OP_EnterWorld: return "HC_OP_EnterWorld";
		case HC_OP_PostEnterWorld: return "HC_OP_PostEnterWorld";
		case HC_OP_ExpansionInfo: return "HC_OP_ExpansionInfo";
		case HC_OP_SendCharInfo: return "HC_OP_SendCharInfo";
		case HC_OP_World_Client_CRC1: return "HC_OP_World_Client_CRC1";
		case HC_OP_World_Client_CRC2: return "HC_OP_World_Client_CRC2";
		case HC_OP_AckPacket: return "HC_OP_AckPacket";
		case HC_OP_WorldClientReady: return "HC_OP_WorldClientReady";
		case HC_OP_MOTD: return "HC_OP_MOTD";
		case HC_OP_SetChatServer: return "HC_OP_SetChatServer";
		case HC_OP_SetChatServer2: return "HC_OP_SetChatServer2";
		case HC_OP_ZoneServerInfo: return "HC_OP_ZoneServerInfo";
		case HC_OP_WorldComplete: return "HC_OP_WorldComplete";
	}
	
	// Zone opcodes
	switch (opcode) {
		case HC_OP_ZoneEntry: return "HC_OP_ZoneEntry";
		case HC_OP_NewZone: return "HC_OP_NewZone";
		case HC_OP_ReqClientSpawn: return "HC_OP_ReqClientSpawn";
		case HC_OP_ZoneSpawns: return "HC_OP_ZoneSpawns";
		case HC_OP_SendZonepoints: return "HC_OP_SendZonepoints";
		case HC_OP_ReqNewZone: return "HC_OP_ReqNewZone";
		case HC_OP_PlayerProfile: return "HC_OP_PlayerProfile";
		case HC_OP_CharInventory: return "HC_OP_CharInventory";
		case HC_OP_TimeOfDay: return "HC_OP_TimeOfDay";
		case HC_OP_SpawnDoor: return "HC_OP_SpawnDoor";
		case HC_OP_ClientReady: return "HC_OP_ClientReady";
		case HC_OP_ZoneChange: return "HC_OP_ZoneChange";
		case HC_OP_SetServerFilter: return "HC_OP_SetServerFilter";
		case HC_OP_GroundSpawn: return "HC_OP_GroundSpawn";
		case HC_OP_Weather: return "HC_OP_Weather";
		case HC_OP_ClientUpdate: return "HC_OP_ClientUpdate";
		case HC_OP_SpawnAppearance: return "HC_OP_SpawnAppearance";
		case HC_OP_NewSpawn: return "HC_OP_NewSpawn";
		case HC_OP_DeleteSpawn: return "HC_OP_DeleteSpawn";
		case HC_OP_MobHealth: return "HC_OP_MobHealth";
		case HC_OP_HPUpdate: return "HC_OP_HPUpdate";
		case HC_OP_TributeUpdate: return "HC_OP_TributeUpdate";
		case HC_OP_TributeTimer: return "HC_OP_TributeTimer";
		case HC_OP_SendAATable: return "HC_OP_SendAATable";
		case HC_OP_UpdateAA: return "HC_OP_UpdateAA";
		case HC_OP_RespondAA: return "HC_OP_RespondAA";
		case HC_OP_SendTributes: return "HC_OP_SendTributes";
		case HC_OP_TributeInfo: return "HC_OP_TributeInfo";
		case HC_OP_RequestGuildTributes: return "HC_OP_RequestGuildTributes";
		case HC_OP_SendGuildTributes: return "HC_OP_SendGuildTributes";
		case HC_OP_SendAAStats: return "HC_OP_SendAAStats";
		case HC_OP_SendExpZonein: return "HC_OP_SendExpZonein";
		case HC_OP_WorldObjectsSent: return "HC_OP_WorldObjectsSent";
		case HC_OP_ExpUpdate: return "HC_OP_ExpUpdate";
		case HC_OP_RaidUpdate: return "HC_OP_RaidUpdate";
		case HC_OP_GuildMOTD: return "HC_OP_GuildMOTD";
		case HC_OP_ChannelMessage: return "HC_OP_ChannelMessage";
		case HC_OP_WearChange: return "HC_OP_WearChange";
		case HC_OP_MoveDoor: return "HC_OP_MoveDoor";
		case HC_OP_CompletedTasks: return "HC_OP_CompletedTasks";
		case HC_OP_DzCompass: return "HC_OP_DzCompass";
		case HC_OP_DzExpeditionLockoutTimers: return "HC_OP_DzExpeditionLockoutTimers";
		case HC_OP_BeginCast: return "HC_OP_BeginCast";
		case HC_OP_ManaChange: return "HC_OP_ManaChange";
		case HC_OP_FormattedMessage: return "HC_OP_FormattedMessage";
		case HC_OP_PlayerStateAdd: return "HC_OP_PlayerStateAdd";
		case HC_OP_Death: return "HC_OP_Death";
		case HC_OP_PlayerStateRemove: return "HC_OP_PlayerStateRemove";
		case HC_OP_Stamina: return "HC_OP_Stamina";
		default: return fmt::format("OP_Unknown_{:#06x}", opcode);
	}
}

void EverQuest::DumpPacket(const std::string &prefix, uint16_t opcode, const EQ::Net::Packet &p) {
	DumpPacket(prefix, opcode, (const uint8_t*)p.Data(), p.Length());
}

void EverQuest::DumpPacket(const std::string &prefix, uint16_t opcode, const uint8_t *data, size_t size) {
	if (s_debug_level < 3) return;

	std::cout << fmt::format("[Packet {}] [{}] [{:#06x}] Size [{}]",  
		prefix, GetOpcodeName(opcode), opcode, size) << std::endl;

	if (s_debug_level >= 3) {
		// Hex dump
		std::stringstream ss;
		for (size_t i = 0; i < size; i += 16) {
			ss << std::setfill(' ') << std::setw(5) << i << ": ";
			
			// Hex bytes
			for (size_t j = 0; j < 16; j++) {
				if (i + j < size) {
					ss << std::setfill('0') << std::setw(2) << std::hex << (int)data[i + j] << " ";
				} else {
					ss << "   ";
				}
				if (j == 7) ss << "- ";
			}
			
			ss << " | ";
			
			// ASCII representation
			for (size_t j = 0; j < 16 && i + j < size; j++) {
				char c = data[i + j];
				ss << (isprint(c) ? c : '.');
			}
			
			if (i + 16 < size) {
				ss << "\n";
			}
		}
		std::cout << ss.str() << std::endl;
	}
}

EverQuest::EverQuest(const std::string &host, int port, const std::string &user, const std::string &pass, const std::string &server, const std::string &character)
{
	m_host = host;
	m_port = port;
	m_user = user;
	m_pass = pass;
	m_server = server;
	m_character = character;
	m_dbid = 0;

	EQ::Net::DNSLookup(m_host, port, false, [&](const std::string &addr) {
		if (addr.empty()) {
			std::cout << fmt::format("Could not resolve address: {}",  m_host) << std::endl;
			return;
		}
		else {
			m_host = addr;
			m_login_connection_manager.reset(new EQ::Net::DaybreakConnectionManager());

			m_login_connection_manager->OnNewConnection(std::bind(&EverQuest::LoginOnNewConnection, this, std::placeholders::_1));
			m_login_connection_manager->OnConnectionStateChange(std::bind(&EverQuest::LoginOnStatusChangeReconnectEnabled, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			m_login_connection_manager->OnPacketRecv(std::bind(&EverQuest::LoginOnPacketRecv, this, std::placeholders::_1, std::placeholders::_2));

			m_login_connection_manager->Connect(m_host, m_port);
		}
	});
}

EverQuest::~EverQuest()
{
}

void EverQuest::LoginOnNewConnection(std::shared_ptr<EQ::Net::DaybreakConnection> connection)
{
	m_login_connection = connection;
	std::cout << "Connecting..." << std::endl;
}

void EverQuest::LoginOnStatusChangeReconnectEnabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to)
{
	if (to == EQ::Net::StatusConnected) {
		std::cout << "Login connected." << std::endl;
		LoginSendSessionReady();
	}

	if (to == EQ::Net::StatusDisconnected) {
		std::cout << "Login connection lost before we got to world, reconnecting." << std::endl;
		m_key.clear();
		m_dbid = 0;
		m_login_connection.reset();
		m_login_connection_manager->Connect(m_host, m_port);
	}
}

void EverQuest::LoginOnStatusChangeReconnectDisabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to)
{
	if (to == EQ::Net::StatusDisconnected) {
		m_login_connection.reset();
	}
}

void EverQuest::LoginOnPacketRecv(std::shared_ptr<EQ::Net::DaybreakConnection> conn, const EQ::Net::Packet & p)
{
	auto opcode = p.GetUInt16(0);
	DumpPacket("S->C", opcode, p);
	
	switch (opcode) {
	case HC_OP_ChatMessage:
		if (s_debug_level >= 1) {
			std::cout << "Received HC_OP_ChatMessage, sending login" << std::endl;
		}
		LoginSendLogin();
		break;
	case HC_OP_LoginAccepted:
		LoginProcessLoginResponse(p);
		break;
	case HC_OP_ServerListResponse:
		LoginProcessServerPacketList(p);
		break;
	case HC_OP_PlayEverquestResponse:
		LoginProcessServerPlayResponse(p);
		break;
	default:
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Unhandled login opcode: {:#06x}",  opcode) << std::endl;
		}
		break;
	}
}

void EverQuest::LoginSendSessionReady()
{
	EQ::Net::DynamicPacket p;
	p.Resize(14);
	p.PutUInt16(0, HC_OP_SessionReady);
	p.PutUInt32(2, m_login_sequence++);  // sequence
	p.PutUInt32(6, 0);  // unknown
	p.PutUInt32(10, 2048);  // max length
	
	DumpPacket("C->S", HC_OP_SessionReady, p);
	m_login_connection->QueuePacket(p);
}

void EverQuest::LoginSendLogin()
{
	size_t buffer_len = m_user.length() + m_pass.length() + 2;
	std::unique_ptr<char[]> buffer(new char[buffer_len]);
	memset(buffer.get(), 0, buffer_len);

	strcpy(&buffer[0], m_user.c_str());
	strcpy(&buffer[m_user.length() + 1], m_pass.c_str());

	size_t encrypted_len = buffer_len;

	if (encrypted_len % 8 > 0) {
		encrypted_len = ((encrypted_len / 8) + 1) * 8;
	}

	EQ::Net::DynamicPacket p;
	p.Resize(12 + encrypted_len);
	p.PutUInt16(0, HC_OP_Login);
	p.PutUInt32(2, m_login_sequence++);  // sequence
	p.PutUInt32(6, 0x00020000);  // unknown - matches good log
	
	// Clear the encrypted area first
	memset((char*)p.Data() + 12, 0, encrypted_len);
	eqcrypt_block(&buffer[0], buffer_len, (char*)p.Data() + 12, true);

	DumpPacket("C->S", HC_OP_Login, p);
	m_login_connection->QueuePacket(p);
}

void EverQuest::LoginSendServerRequest()
{
	EQ::Net::DynamicPacket p;
	p.Resize(12);
	p.PutUInt16(0, HC_OP_ServerListRequest);
	p.PutUInt32(2, m_login_sequence++);  // sequence
	p.PutUInt32(6, 0);  // unknown
	p.PutUInt16(10, 0);  // unknown
	
	DumpPacket("C->S", HC_OP_ServerListRequest, p);
	m_login_connection->QueuePacket(p);
}

void EverQuest::LoginSendPlayRequest(uint32_t id)
{
	EQ::Net::DynamicPacket p;
	p.Resize(16);
	p.PutUInt16(0, HC_OP_PlayEverquestRequest);
	p.PutUInt32(2, m_login_sequence++);  // sequence
	p.PutUInt32(6, 0);  // unknown
	p.PutUInt16(10, 0);  // unknown
	p.PutUInt32(12, id);  // server id
	
	DumpPacket("C->S", HC_OP_PlayEverquestRequest, p);
	m_login_connection->QueuePacket(p);
}

void EverQuest::LoginProcessLoginResponse(const EQ::Net::Packet & p)
{
	auto encrypt_size = p.Length() - 12;
	if (encrypt_size % 8 > 0) {
		encrypt_size = (encrypt_size / 8) * 8;
	}

	std::unique_ptr<char[]> decrypted(new char[encrypt_size]);

	eqcrypt_block((char*)p.Data() + 12, encrypt_size, &decrypted[0], false);

	EQ::Net::StaticPacket sp(&decrypted[0], encrypt_size);
	auto response_error = sp.GetUInt16(1);

	if (response_error > 101) {
		std::cout << fmt::format("Error logging in response code: {}",  response_error) << std::endl;
		LoginDisableReconnect();
	}
	else {
		m_key = sp.GetCString(12);
		m_dbid = sp.GetUInt32(8);

		std::cout << fmt::format("Logged in successfully with dbid {} and key {}",  m_dbid, m_key) << std::endl;
		LoginSendServerRequest();
	}
}

void EverQuest::LoginProcessServerPacketList(const EQ::Net::Packet & p)
{
	m_world_servers.clear();
	auto number_of_servers = p.GetUInt32(18);
	size_t idx = 22;

	for (auto i = 0U; i < number_of_servers; ++i) {
		WorldServer ws;
		ws.address = p.GetCString(idx);
		idx += (ws.address.length() + 1);

		ws.type = p.GetInt32(idx);
		idx += 4;

		auto id = p.GetUInt32(idx);
		idx += 4;

		ws.long_name = p.GetCString(idx);
		idx += (ws.long_name.length() + 1);

		ws.lang = p.GetCString(idx);
		idx += (ws.lang.length() + 1);

		ws.region = p.GetCString(idx);
		idx += (ws.region.length() + 1);

		ws.status = p.GetInt32(idx);
		idx += 4;

		ws.players = p.GetInt32(idx);
		idx += 4;

		m_world_servers[id] = ws;
	}

	for (auto server : m_world_servers) {
		if (server.second.long_name.compare(m_server) == 0) {
			std::cout << fmt::format("Found world server {}, attempting to login.",  m_server) << std::endl;
			LoginSendPlayRequest(server.first);
			return;
		}
	}

	std::cout << fmt::format("Got response from login server but could not find world server {} disconnecting.",  m_server) << std::endl;
	LoginDisableReconnect();
}

void EverQuest::LoginProcessServerPlayResponse(const EQ::Net::Packet &p)
{
	auto allowed = p.GetUInt8(12);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("PlayEverquestResponse: allowed={}, server_id={}",  
			allowed, p.GetUInt32(18)) << std::endl;
	}

	if (allowed) {
		auto server = p.GetUInt32(18);
		auto ws = m_world_servers.find(server);
		if (ws != m_world_servers.end()) {
			std::cout << fmt::format("Connecting to world server {} at {}:9000",  
				ws->second.long_name, ws->second.address) << std::endl;
			ConnectToWorld(ws->second.address);
			LoginDisableReconnect();
		} else {
			std::cout << fmt::format("Server ID {} not found in world servers list",  server) << std::endl;
		}
	}
	else {
		auto message = p.GetUInt16(13);
		std::cout << fmt::format("Failed to login to server with message {}",  message) << std::endl;
		LoginDisableReconnect();
	}
}

void EverQuest::LoginDisableReconnect()
{
	m_login_connection_manager->OnConnectionStateChange(std::bind(&EverQuest::LoginOnStatusChangeReconnectDisabled, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_login_connection->Close();
}

void EverQuest::ConnectToWorld(const std::string& world_address)
{
	if (s_debug_level >= 1) {
		std::cout << fmt::format("[EverQuest::ConnectToWorld] Creating new world connection manager for {}:9000",  world_address) << std::endl;
	}
	m_world_connection_manager.reset(new EQ::Net::DaybreakConnectionManager());
	m_world_connection_manager->OnNewConnection(std::bind(&EverQuest::WorldOnNewConnection, this, std::placeholders::_1));
	m_world_connection_manager->OnConnectionStateChange(std::bind(&EverQuest::WorldOnStatusChangeReconnectEnabled, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_world_connection_manager->OnPacketRecv(std::bind(&EverQuest::WorldOnPacketRecv, this, std::placeholders::_1, std::placeholders::_2));
	m_world_connection_manager->Connect(world_address, 9000);
}

void EverQuest::WorldOnNewConnection(std::shared_ptr<EQ::Net::DaybreakConnection> connection)
{
	m_world_connection = connection;
	std::cout << "Connecting to world..." << std::endl;
	if (s_debug_level >= 1) {
		std::cout << fmt::format("World connection object created: {}",  connection ? "valid" : "null") << std::endl;
	}
}

void EverQuest::WorldOnStatusChangeReconnectEnabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to)
{
	if (to == EQ::Net::StatusConnected) {
		std::cout << "World connected." << std::endl;
		// Send login info immediately
		WorldSendClientAuth();
	}

	if (to == EQ::Net::StatusDisconnected) {
		std::cout << "World connection lost, reconnecting." << std::endl;
		m_world_connection.reset();
		// We need to store the world address for reconnects
		// For now, just log the disconnect
	}
}

void EverQuest::WorldOnStatusChangeReconnectDisabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to)
{
	if (to == EQ::Net::StatusDisconnected) {
		m_world_connection.reset();
	}
}

void EverQuest::WorldOnPacketRecv(std::shared_ptr<EQ::Net::DaybreakConnection> conn, const EQ::Net::Packet & p)
{
	if (s_debug_level >= 1) {
		std::cout << "WorldOnPacketRecv called!" << std::endl;
	}
	auto opcode = p.GetUInt16(0);
	DumpPacket("S->C", opcode, p);
	
	switch (opcode) {
	case HC_OP_ChatMessage:
		if (s_debug_level >= 1) {
			std::cout << "Received world HC_OP_ChatMessage, sending login info" << std::endl;
		}
		WorldSendClientAuth();
		break;
	case HC_OP_SessionReady:
		if (s_debug_level >= 1) {
			std::cout << "Received world HC_OP_SessionReady" << std::endl;
		}
		// Server is ready, now send login info
		WorldSendClientAuth();
		break;
	case HC_OP_GuildsList:
		WorldProcessGuildsList(p);
		break;
	case HC_OP_LogServer:
		WorldProcessLogServer(p);
		break;
	case HC_OP_ApproveWorld:
		WorldProcessApproveWorld(p);
		break;
	case HC_OP_EnterWorld:
		WorldProcessEnterWorld(p);
		break;
	case HC_OP_PostEnterWorld:
		WorldProcessPostEnterWorld(p);
		break;
	case HC_OP_ExpansionInfo:
		WorldProcessExpansionInfo(p);
		break;
	case HC_OP_SendCharInfo:
		WorldProcessCharacterSelect(p);
		break;
	case HC_OP_MOTD:
		WorldProcessMOTD(p);
		break;
	case HC_OP_SetChatServer:
	case HC_OP_SetChatServer2:
		WorldProcessSetChatServer(p);
		break;
	case HC_OP_ZoneServerInfo:
		WorldProcessZoneServerInfo(p);
		break;
	default:
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Unhandled world opcode: {}",  GetOpcodeName(opcode)) << std::endl;
		}
		break;
	}
}

void EverQuest::WorldSendSessionReady()
{
	// Don't send SessionReady - world server doesn't expect it
	// The issue must be something else
	return;
}

void EverQuest::WorldSendClientAuth()
{
	EQ::Net::DynamicPacket p;
	p.Resize(466);  // 2 bytes opcode + 464 bytes LoginInfo struct

	p.PutUInt16(0, HC_OP_SendLoginInfo);
	
	// Clear the entire packet
	memset((char*)p.Data() + 2, 0, 464);
	
	// login_info field starts at offset 2 (after opcode)
	char* login_info = (char*)p.Data() + 2;
	
	// First put the account ID (converted from dbid)
	std::string dbid_str = std::to_string(m_dbid);
	size_t dbid_len = dbid_str.length();
	if (dbid_len > 18) dbid_len = 18;
	memcpy(login_info, dbid_str.c_str(), dbid_len);
	
	// Add null terminator after account ID
	login_info[dbid_len] = 0;
	
	// Copy session key after the null terminator
	size_t key_len = m_key.length();
	if (key_len > 15) key_len = 15;
	memcpy(login_info + dbid_len + 1, m_key.c_str(), key_len);
	
	// Set zoning flag to 0 (not zoning, fresh login) at offset 188 of LoginInfo
	p.PutUInt8(2 + 188, 0);

	if (s_debug_level >= 1) {
		std::cout << fmt::format("Sending login info: dbid={}, key={}",  dbid_str, m_key) << std::endl;
	}

	DumpPacket("C->S", HC_OP_SendLoginInfo, p);
	m_world_connection->QueuePacket(p);
}

void EverQuest::WorldSendEnterWorld(const std::string &character)
{
	EQ::Net::DynamicPacket p;
	p.Resize(74);  // Matching good log size
	
	p.PutUInt16(0, HC_OP_EnterWorld);
	
	// Character name at offset 2, null-terminated, 64 bytes
	size_t name_len = character.length();
	if (name_len > 63) name_len = 63;
	memcpy((char*)p.Data() + 2, character.c_str(), name_len);
	p.PutUInt8(2 + name_len, 0);  // null terminator
	
	// Fill rest with zeros
	for (size_t i = 2 + name_len + 1; i < 66; i++) {
		p.PutUInt8(i, 0);
	}
	
	p.PutUInt32(66, 0);  // unknown
	p.PutUInt32(70, 0);  // unknown

	DumpPacket("C->S", HC_OP_EnterWorld, p);
	m_world_connection->QueuePacket(p);
	m_enter_world_sent = true;
}

void EverQuest::WorldProcessCharacterSelect(const EQ::Net::Packet &p)
{
	// For Titanium client, the packet is just the raw CharacterSelect_Struct
	// Let's verify packet size
	if (p.Length() < 1706) {
		std::cout << fmt::format("[ERROR] Character select packet too small: {} bytes", p.Length()) << std::endl;
		return;
	}
	
	// Debug: Let's check names array at offset 1024 + 2 (for opcode)
	std::cout << "[DEBUG] Checking character names in Titanium format:" << std::endl;
	
	// Titanium CharacterSelect_Struct has names at offset 1024
	const size_t names_offset = 1024 + 2; // +2 for opcode
	
	// Check all 10 character slots
	for (int i = 0; i < 10; i++) {
		size_t name_offset = names_offset + (i * 64);
		
		// Get character name
		char name_buf[65] = {0};
		for (int j = 0; j < 64 && name_offset + j < p.Length(); j++) {
			name_buf[j] = p.GetUInt8(name_offset + j);
			if (name_buf[j] == 0) break;
		}
		std::string name(name_buf);
		
		if (name.length() > 0) {
			// Get other character data using Titanium offsets
			uint8_t level = p.GetUInt8(1694 + 2 + i);  // Level array at offset 1694
			uint8_t pclass = p.GetUInt8(1004 + 2 + i); // Class array at offset 1004
			uint32_t race = p.GetUInt32(0 + 2 + (i * 4)); // Race array at offset 0
			uint32_t zone = p.GetUInt32(964 + 2 + (i * 4)); // Zone array at offset 964
			
			std::cout << fmt::format("[DEBUG] Character {}: name='{}', level={}, class={}, race={}, zone={}", 
				i, name, level, pclass, race, zone) << std::endl;
			
			if (m_character.compare(name) == 0) {
				std::cout << fmt::format("[DEBUG] Found our character '{}' at index {}", m_character, i) << std::endl;
				m_character_select_index = i;
				return;
			}
		}
	}

	std::cout << fmt::format("Could not find {}, cannot continue to login.",  m_character) << std::endl;
}

void EverQuest::WorldSendApproveWorld()
{
	EQ::Net::DynamicPacket p;
	p.Resize(274);  // Matching good log response size
	
	p.PutUInt16(0, HC_OP_ApproveWorld);
	// The rest is character-specific data which we'll leave as zeros for now
	
	DumpPacket("C->S", HC_OP_ApproveWorld, p);
	m_world_connection->QueuePacket(p);
}

void EverQuest::WorldSendWorldClientCRC()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2058);  // Matching good log size
	
	// Send CRC1
	p.PutUInt16(0, HC_OP_World_Client_CRC1);
	// Fill with dummy CRC data
	for (size_t i = 2; i < 2058; i++) {
		p.PutUInt8(i, 0);
	}
	
	DumpPacket("C->S", HC_OP_World_Client_CRC1, p);
	m_world_connection->QueuePacket(p);
	
	// Send CRC2
	p.PutUInt16(0, HC_OP_World_Client_CRC2);
	DumpPacket("C->S", HC_OP_World_Client_CRC2, p);
	m_world_connection->QueuePacket(p);
}

void EverQuest::WorldSendWorldClientReady()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_WorldClientReady);
	
	DumpPacket("C->S", HC_OP_WorldClientReady, p);
	m_world_connection->QueuePacket(p);
	m_world_ready = true;
}

void EverQuest::WorldSendWorldComplete()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_WorldComplete);
	
	DumpPacket("C->S", HC_OP_WorldComplete, p);
	m_world_connection->QueuePacket(p);
}

void EverQuest::WorldProcessGuildsList(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received guilds list" << std::endl;
	}
}

void EverQuest::WorldProcessLogServer(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received log server info" << std::endl;
	}
}

void EverQuest::WorldProcessApproveWorld(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "World approved, sending response" << std::endl;
	}
	WorldSendApproveWorld();
	
	// After approval, send CRC packets
	WorldSendWorldClientCRC();
}

void EverQuest::WorldProcessEnterWorld(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Server acknowledged enter world" << std::endl;
	}
}

void EverQuest::WorldProcessPostEnterWorld(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Post enter world received" << std::endl;
	}
}

void EverQuest::WorldProcessExpansionInfo(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		uint32_t expansions = p.GetUInt32(2);
		std::cout << fmt::format("Expansion info: {:#x}",  expansions) << std::endl;
	}
	
	// After receiving initial packets, send WorldClientReady
	if (!m_world_ready) {
		// Send ACK packet first
		EQ::Net::DynamicPacket ack;
		ack.Resize(6);
		ack.PutUInt16(0, HC_OP_AckPacket);
		ack.PutUInt32(2, 0);  // sequence or something
		DumpPacket("C->S", HC_OP_AckPacket, ack);
		m_world_connection->QueuePacket(ack);
		
		// Then send WorldClientReady
		WorldSendWorldClientReady();
		
		// Now send EnterWorld
		if (!m_enter_world_sent) {
			WorldSendEnterWorld(m_character);
		}
	}
}

void EverQuest::WorldProcessMOTD(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received MOTD" << std::endl;
	}
}

void EverQuest::WorldProcessSetChatServer(const EQ::Net::Packet &p)
{
	// Parse the chat server info from world server
	// Format: "host,port,server.character,connection_type,mail_key"
	std::string chat_info = p.GetCString(2);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Received chat server info: {}", chat_info) << std::endl;
	}
	
	// Parse the comma-separated values
	std::vector<std::string> parts;
	std::stringstream ss(chat_info);
	std::string part;
	while (std::getline(ss, part, ',')) {
		parts.push_back(part);
	}
	
	if (parts.size() >= 5) {
		m_ucs_host = parts[0];
		m_ucs_port = std::stoi(parts[1]);
		// parts[2] is server.character
		// parts[3] is connection type (we'll use for authentication)
		m_mail_key = parts[4];
		
		if (s_debug_level >= 1) {
			std::cout << fmt::format("UCS connection info: {}:{}, mail_key: {}", 
				m_ucs_host, m_ucs_port, m_mail_key) << std::endl;
		}
		
		// Connect to UCS server
		// TODO: Fix UCS implementation to use correct EQStream API
		// ConnectToUCS(m_ucs_host, m_ucs_port);
	} else {
		if (s_debug_level >= 1) {
			std::cout << "Invalid chat server info format" << std::endl;
		}
	}
}

void EverQuest::WorldProcessZoneServerInfo(const EQ::Net::Packet &p)
{
	// Extract zone server info
	m_zone_server_host = p.GetCString(2);
	m_zone_server_port = p.GetUInt16(130);
	
	std::cout << fmt::format("Zone server info received: {}:{}",  
		m_zone_server_host, m_zone_server_port) << std::endl;
	
	// Send WorldComplete to acknowledge
	WorldSendWorldComplete();
	
	// Connect to the zone server
	ConnectToZone();
}

// Zone server connection functions
void EverQuest::ConnectToZone()
{
	std::cout << fmt::format("Connecting to zone server at {}:{}",  
		m_zone_server_host, m_zone_server_port) << std::endl;
		
	m_zone_connection_manager.reset(new EQ::Net::DaybreakConnectionManager());
	m_zone_connection_manager->OnNewConnection(std::bind(&EverQuest::ZoneOnNewConnection, this, std::placeholders::_1));
	m_zone_connection_manager->OnConnectionStateChange(std::bind(&EverQuest::ZoneOnStatusChangeReconnectEnabled, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	m_zone_connection_manager->OnPacketRecv(std::bind(&EverQuest::ZoneOnPacketRecv, this, std::placeholders::_1, std::placeholders::_2));
	m_zone_connection_manager->Connect(m_zone_server_host, m_zone_server_port);
}

void EverQuest::ZoneOnNewConnection(std::shared_ptr<EQ::Net::DaybreakConnection> connection)
{
	m_zone_connection = connection;
	std::cout << "Connecting to zone..." << std::endl;
}

void EverQuest::ZoneOnStatusChangeReconnectEnabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to)
{
	if (to == EQ::Net::StatusConnected) {
		std::cout << "Zone connected." << std::endl;
		m_zone_connected = true;
		// Send stream identify immediately after connection
		ZoneSendStreamIdentify();
		m_zone_session_established = true;
		
		// Send ack packet and zone entry immediately after session established
		ZoneSendAckPacket();
		ZoneSendZoneEntry();
	}

	if (to == EQ::Net::StatusDisconnected) {
		std::cout << "Zone connection lost, reconnecting." << std::endl;
		m_zone_connected = false;
		m_zone_session_established = false;
		m_zone_entry_sent = false;
		// m_client_spawned = false; // Deprecated
		m_zone_connection.reset();
		m_zone_connection_manager->Connect(m_zone_server_host, m_zone_server_port);
	}
}

void EverQuest::ZoneOnStatusChangeReconnectDisabled(std::shared_ptr<EQ::Net::DaybreakConnection> conn, EQ::Net::DbProtocolStatus from, EQ::Net::DbProtocolStatus to)
{
	if (to == EQ::Net::StatusDisconnected) {
		m_zone_connection.reset();
	}
}

void EverQuest::ZoneOnPacketRecv(std::shared_ptr<EQ::Net::DaybreakConnection> conn, const EQ::Net::Packet & p)
{
	auto opcode = p.GetUInt16(0);
	DumpPacket("S->C", opcode, p);
	
	switch (opcode) {
	case HC_OP_SessionReady:
		if (s_debug_level >= 1) {
			std::cout << "Zone session established, sending ack and zone entry" << std::endl;
		}
		// Send ack packet
		ZoneSendAckPacket();
		// Send zone entry
		ZoneSendZoneEntry();
		break;
	case HC_OP_PlayerProfile:
		ZoneProcessPlayerProfile(p);
		break;
	case HC_OP_ZoneEntry:
		// Server echoes our zone entry back with ServerZoneEntry_Struct
		std::cout << fmt::format("[DEBUG] Zone entry response, size: {}", p.Length()) << std::endl;
		
		// ServerZoneEntry_Struct contains NewSpawn_Struct which contains Spawn_Struct
		// Let's check for spawn ID at various offsets
		if (p.Length() > 10) {
			// Try to find spawn ID - it might be early in the spawn struct
			std::cout << fmt::format("[DEBUG] Potential spawn IDs in ZoneEntry response:") << std::endl;
			std::cout << fmt::format("[DEBUG]   uint16 at offset 2: {}", p.GetUInt16(2)) << std::endl;
			std::cout << fmt::format("[DEBUG]   uint16 at offset 4: {}", p.GetUInt16(4)) << std::endl;
			std::cout << fmt::format("[DEBUG]   uint32 at offset 2: {}", p.GetUInt32(2)) << std::endl;
			std::cout << fmt::format("[DEBUG]   uint32 at offset 6: {}", p.GetUInt32(6)) << std::endl;
			
			// The spawn struct has name at offset 7, let's check before that
			if (p.Length() > 71) {
				auto spawn_name = p.GetCString(9); // offset 2 + 7
				std::cout << fmt::format("[DEBUG]   Spawn name at offset 7: '{}'", spawn_name) << std::endl;
			}
		}
		break;
	case HC_OP_ZoneSpawns:
		ZoneProcessZoneSpawns(p);
		break;
	case HC_OP_TimeOfDay:
		ZoneProcessTimeOfDay(p);
		break;
	case HC_OP_TributeUpdate:
		ZoneProcessTributeUpdate(p);
		break;
	case HC_OP_TributeTimer:
		ZoneProcessTributeTimer(p);
		break;
	case HC_OP_CharInventory:
		ZoneProcessCharInventory(p);
		break;
	case HC_OP_Weather:
		ZoneProcessWeather(p);
		break;
	case HC_OP_NewZone:
		ZoneProcessNewZone(p);
		break;
	case HC_OP_SendAATable:
		ZoneProcessSendAATable(p);
		break;
	case HC_OP_RespondAA:
		ZoneProcessRespondAA(p);
		break;
	case HC_OP_TributeInfo:
		ZoneProcessTributeInfo(p);
		break;
	case HC_OP_SendGuildTributes:
		ZoneProcessSendGuildTributes(p);
		break;
	case HC_OP_SpawnDoor:
		ZoneProcessSpawnDoor(p);
		break;
	case HC_OP_GroundSpawn:
		ZoneProcessGroundSpawn(p);
		break;
	case HC_OP_SendZonepoints:
		ZoneProcessSendZonepoints(p);
		break;
	case HC_OP_SendAAStats:
		ZoneProcessSendAAStats(p);
		break;
	case HC_OP_SendExpZonein:
		ZoneProcessSendExpZonein(p);
		break;
	case HC_OP_WorldObjectsSent:
		ZoneProcessWorldObjectsSent(p);
		break;
	case HC_OP_SpawnAppearance:
		ZoneProcessSpawnAppearance(p);
		break;
	case HC_OP_ExpUpdate:
		ZoneProcessExpUpdate(p);
		break;
	case HC_OP_RaidUpdate:
		ZoneProcessRaidUpdate(p);
		break;
	case HC_OP_GuildMOTD:
		ZoneProcessGuildMOTD(p);
		break;
	case HC_OP_NewSpawn:
		ZoneProcessNewSpawn(p);
		break;
	case HC_OP_ClientUpdate:
		ZoneProcessClientUpdate(p);
		break;
	case HC_OP_DeleteSpawn:
		ZoneProcessDeleteSpawn(p);
		break;
	case HC_OP_MobHealth:
		ZoneProcessMobHealth(p);
		break;
	case HC_OP_HPUpdate:
		ZoneProcessHPUpdate(p);
		break;
	case HC_OP_ChannelMessage:
		ZoneProcessChannelMessage(p);
		break;
	case HC_OP_WearChange:
		ZoneProcessWearChange(p);
		break;
	case HC_OP_MoveDoor:
		ZoneProcessMoveDoor(p);
		break;
	case HC_OP_CompletedTasks:
		ZoneProcessCompletedTasks(p);
		break;
	case HC_OP_DzCompass:
		ZoneProcessDzCompass(p);
		break;
	case HC_OP_DzExpeditionLockoutTimers:
		ZoneProcessDzExpeditionLockoutTimers(p);
		break;
	case HC_OP_BeginCast:
		ZoneProcessBeginCast(p);
		break;
	case HC_OP_ManaChange:
		ZoneProcessManaChange(p);
		break;
	case HC_OP_FormattedMessage:
		ZoneProcessFormattedMessage(p);
		break;
	case HC_OP_PlayerStateAdd:
		ZoneProcessPlayerStateAdd(p);
		break;
	case HC_OP_Death:
		ZoneProcessDeath(p);
		break;
	case HC_OP_PlayerStateRemove:
		ZoneProcessPlayerStateRemove(p);
		break;
	case HC_OP_Stamina:
		ZoneProcessStamina(p);
		break;
	default:
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Unhandled zone opcode: {}", GetOpcodeName(opcode)) << std::endl;
		}
		break;
	}
}

// Zone packet senders
void EverQuest::ZoneSendSessionReady()
{
	EQ::Net::DynamicPacket p;
	p.Resize(14);
	p.PutUInt16(0, HC_OP_SessionReady);
	p.PutUInt32(2, m_zone_sequence++);  // sequence
	p.PutUInt32(6, 0);  // unknown
	p.PutUInt32(10, 2048);  // max length
	
	DumpPacket("C->S", HC_OP_SessionReady, p);
	m_zone_connection->QueuePacket(p);
}

void EverQuest::ZoneSendZoneEntry()
{
	// Zone entry packet - ClientZoneEntry_Struct
	EQ::Net::DynamicPacket p;
	p.Resize(70);  // 2 bytes opcode + 68 bytes data
	
	p.PutUInt16(0, HC_OP_ZoneEntry);
	
	// ClientZoneEntry_Struct starts at offset 2
	// unknown00 - 32-bit value (0xFFF67726 in good log)
	p.PutUInt32(2, 0xFFF67726);  // This might need to be calculated or could be a checksum
	
	// char_name[64] at offset 6
	size_t name_offset = 6;
	size_t name_len = m_character.length();
	if (name_len > 63) name_len = 63;
	memcpy((char*)p.Data() + name_offset, m_character.c_str(), name_len);
	
	// Fill rest of name field with zeros (64 bytes total for name)
	for (size_t i = name_offset + name_len; i < name_offset + 64; i++) {
		p.PutUInt8(i, 0);
	}
	
	DumpPacket("C->S", HC_OP_ZoneEntry, p);
	m_zone_connection->QueuePacket(p);
	m_zone_entry_sent = true;
}

void EverQuest::ZoneSendReqClientSpawn()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_ReqClientSpawn);
	
	DumpPacket("C->S", HC_OP_ReqClientSpawn, p);
	m_zone_connection->QueuePacket(p);
}

void EverQuest::ZoneSendClientReady()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_ClientReady);
	
	if (s_debug_level >= 1) {
		std::cout << "Sending OP_ClientReady" << std::endl;
	}
	
	DumpPacket("C->S", HC_OP_ClientReady, p);
	m_zone_connection->QueuePacket(p);
	m_client_ready_sent = true;
}

void EverQuest::ZoneSendSetServerFilter()
{
	// Set server filter for what types of messages we want
	// SetServerFilter_Struct is uint32 filters[29] = 116 bytes
	EQ::Net::DynamicPacket p;
	p.Resize(118);  // 2 bytes opcode + 116 bytes data
	
	p.PutUInt16(0, HC_OP_SetServerFilter);
	
	// Set all filters to 0xFFFFFFFF (all enabled) for now
	for (size_t i = 0; i < 29; i++) {
		p.PutUInt32(2 + (i * 4), 0xFFFFFFFF);
	}
	
	if (s_debug_level >= 1) {
		std::cout << "Sending OP_SetServerFilter" << std::endl;
	}

	DumpPacket("C->S", HC_OP_SetServerFilter, p);
	m_zone_connection->QueuePacket(p);
	m_server_filter_sent = true;
}

void EverQuest::ZoneSendStreamIdentify()
{
	// Send stream identify with Titanium zone opcode
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_ZoneEntry);  // 0x7213 - Titanium_zone
	
	if (s_debug_level >= 1) {
		std::cout << "Sending stream identify with opcode 0x7213 (Titanium_zone)" << std::endl;
	}
	
	m_zone_connection->QueuePacket(p, 0, false);  // Send unreliable
}

void EverQuest::ZoneSendAckPacket()
{
	EQ::Net::DynamicPacket p;
	p.Resize(6);
	
	p.PutUInt16(0, HC_OP_AckPacket);
	p.PutUInt32(2, 0);  // sequence or something
	
	DumpPacket("C->S", HC_OP_AckPacket, p);
	m_zone_connection->QueuePacket(p);
}

void EverQuest::ZoneSendReqNewZone()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_ReqNewZone);
	
	DumpPacket("C->S", HC_OP_ReqNewZone, p);
	m_zone_connection->QueuePacket(p);
	m_req_new_zone_sent = true;
}

void EverQuest::ZoneSendSendAATable()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_SendAATable);
	
	DumpPacket("C->S", HC_OP_SendAATable, p);
	m_zone_connection->QueuePacket(p);
	m_aa_table_sent = true;
}

void EverQuest::ZoneSendUpdateAA()
{
	EQ::Net::DynamicPacket p;
	p.Resize(12);  // Typical UpdateAA size
	
	p.PutUInt16(0, HC_OP_UpdateAA);
	// Rest is AA data - zeros for now
	
	DumpPacket("C->S", HC_OP_UpdateAA, p);
	m_zone_connection->QueuePacket(p);
	m_update_aa_sent = true;
}

void EverQuest::ZoneSendSendTributes()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_SendTributes);
	
	DumpPacket("C->S", HC_OP_SendTributes, p);
	m_zone_connection->QueuePacket(p);
	m_tributes_sent = true;
}

void EverQuest::ZoneSendRequestGuildTributes()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_RequestGuildTributes);
	
	DumpPacket("C->S", HC_OP_RequestGuildTributes, p);
	m_zone_connection->QueuePacket(p);
	m_guild_tributes_sent = true;
}

void EverQuest::ZoneSendSpawnAppearance()
{
	EQ::Net::DynamicPacket p;
	p.Resize(14);  // Typical spawn appearance size
	
	p.PutUInt16(0, HC_OP_SpawnAppearance);
	p.PutUInt16(2, 0);  // spawn id (0 = self)
	p.PutUInt16(4, 14);  // type (14 = animation)
	p.PutUInt32(6, 0);  // value
	p.PutUInt32(10, 0);  // unknown
	
	DumpPacket("C->S", HC_OP_SpawnAppearance, p);
	m_zone_connection->QueuePacket(p);
	m_spawn_appearance_sent = true;
}

void EverQuest::ZoneSendSendExpZonein()
{
	EQ::Net::DynamicPacket p;
	p.Resize(2);
	
	p.PutUInt16(0, HC_OP_SendExpZonein);
	
	DumpPacket("C->S", HC_OP_SendExpZonein, p);
	m_zone_connection->QueuePacket(p);
	m_exp_zonein_sent = true;
}

// Zone packet processors
void EverQuest::ZoneProcessNewZone(const EQ::Net::Packet &p)
{
	// NewZone packet contains zone information
	// Extract zone name from NewZone_Struct
	// char_name[64] at offset 0, zone_short_name[32] at offset 64
	if (p.Length() >= 96) {  // Need at least 64 + 32 bytes
		// Skip opcode (2 bytes) and char_name (64 bytes) to get to zone_short_name
		m_current_zone_name = p.GetCString(66);  // 2 (opcode) + 64 (char_name)
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Received new zone data for: {}", m_current_zone_name) << std::endl;
		}
		
		// Load pathfinder for this zone
		LoadPathfinder(m_current_zone_name);
		
		// Load zone map for terrain height calculations
		LoadZoneMap(m_current_zone_name);
	} else {
		if (s_debug_level >= 1) {
			std::cout << "Received new zone data" << std::endl;
		}
	}
	
	m_new_zone_received = true;
	
	// After NewZone, send all parallel requests
	// 1. AA table request
	if (!m_aa_table_sent) {
		ZoneSendSendAATable();
	}
	
	// 2. Update AA
	if (!m_update_aa_sent) {
		ZoneSendUpdateAA();
	}
	
	// 3. Send tributes request
	if (!m_tributes_sent) {
		ZoneSendSendTributes();
	}
	
	// 4. Request guild tributes
	if (!m_guild_tributes_sent) {
		ZoneSendRequestGuildTributes();
	}
}

void EverQuest::ZoneProcessPlayerProfile(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received player profile" << std::endl;
	}
	
	// PlayerProfile contains our character's data including entity ID
	// The entity ID is at offset 14384 in the PlayerProfile_Struct
	// Offset in packet = 2 (opcode) + 14384 = 14386
	if (p.Length() > 14386) {
		uint32_t entity_id = p.GetUInt32(14386);
		uint16_t old_id = m_my_character_id;
		m_my_character_id = static_cast<uint16_t>(entity_id);
		std::cout << fmt::format("[DEBUG] PlayerProfile entity ID at offset 14386: {} (was {})", entity_id, old_id) << std::endl;
		
		// Double check the bytes
		if (p.Length() > 14389) {
			uint8_t b1 = p.GetUInt8(14386);
			uint8_t b2 = p.GetUInt8(14387);
			uint8_t b3 = p.GetUInt8(14388);
			uint8_t b4 = p.GetUInt8(14389);
			std::cout << fmt::format("[DEBUG] Entity ID bytes: {:02x} {:02x} {:02x} {:02x}", b1, b2, b3, b4) << std::endl;
		}
		
		// Also check what's at offset 22 for debugging
		if (p.Length() > 22) {
			uint16_t unknown_id = p.GetUInt16(22);
			std::cout << fmt::format("[DEBUG] Value at offset 22: {}", unknown_id) << std::endl;
		}
		
		// Let's also check some other potential ID locations
		// unknown00016 at offset 16
		if (p.Length() > 18) {
			uint32_t unknown16 = p.GetUInt32(18); // offset 2 + 16
			std::cout << fmt::format("[DEBUG] unknown00016 at offset 16: {}", unknown16) << std::endl;
		}
		
		// Let's dump the first 100 bytes to see what IDs might be there
		std::cout << "[DEBUG] First 100 bytes of PlayerProfile after opcode:" << std::endl;
		for (size_t i = 2; i < std::min<size_t>(102, p.Length()); i += 4) {
			uint32_t val = 0;
			if (i + 3 < p.Length()) {
				val = p.GetUInt32(i);
			}
			std::cout << fmt::format("  Offset {}: {} (0x{:08x})", i-2, val, val) << std::endl;
		}
		
		// Also check around offset 14384 to see what values are there
		std::cout << "[DEBUG] Values around offset 14384:" << std::endl;
		for (size_t i = 14376; i < std::min<size_t>(14396, p.Length()); i += 2) {
			if (i + 1 < p.Length()) {
				uint16_t val16 = p.GetUInt16(i);
				std::cout << fmt::format("  Offset {}: {} (0x{:04x})", i-2, val16, val16) << std::endl;
			}
		}
	}
	
	// Extract our position from the profile
	// Based on PlayerProfile_Struct, position data starts at offset 28 in the struct
	// But we need to add 2 for the opcode, so packet offsets are +2
	// The pattern is: x(30), y(34), z(38), heading(42)
	if (p.Length() > 42) {
		m_x = p.GetFloat(30);  // Struct offset 28 + 2 for opcode = 288.0
		m_y = p.GetFloat(34);  // Struct offset 32 + 2 for opcode = 344.0
		m_z = p.GetFloat(38);  // Struct offset 36 + 2 for opcode = 3.75
		m_heading = p.GetFloat(42);  // Struct offset 40 + 2 for opcode = 128.0
		
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Initial position: ({:.2f}, {:.2f}, {:.2f}) heading {:.1f}", 
				m_x, m_y, m_z, m_heading) << std::endl;
		}
		
		// Add ourselves to the entity list since we won't be in ZoneSpawns
		Entity self_entity;
		self_entity.spawn_id = m_my_character_id;
		self_entity.name = m_character;
		self_entity.x = m_x;
		self_entity.y = m_y;
		self_entity.z = m_z;
		self_entity.heading = m_heading;
		self_entity.level = 1; // We don't parse our level from PlayerProfile yet
		self_entity.class_id = 0;
		self_entity.race_id = 0;
		self_entity.hp_percent = 100;
		self_entity.animation = 0;
		self_entity.delta_x = 0;
		self_entity.delta_y = 0;
		self_entity.delta_z = 0;
		self_entity.delta_heading = 0;
		self_entity.last_update_time = std::time(nullptr);
		
		m_entities[m_my_character_id] = self_entity;
		
		if (s_debug_level >= 1) {
			std::cout << fmt::format("[DEBUG] Added self to entity list: {} (ID: {})", m_character, m_my_character_id) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessCharInventory(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received character inventory" << std::endl;
	}
}

void EverQuest::ZoneProcessZoneSpawns(const EQ::Net::Packet &p)
{
	// ZoneSpawns contains multiple Spawn_Struct entries
	// Based on titanium_structs.h, Spawn_Struct is 385 bytes
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Received zone spawns packet, size: {} bytes", p.Length()) << std::endl;
	}
	
	// Don't clear existing entities - ZoneSpawns packets can come in multiple batches
	// Only add/update spawns, don't remove existing ones
	
	// The ZoneSpawns packet appears to have a header before the spawn data
	size_t offset = 2;
	int spawn_count = 0;
	
	/*	
	// Debug: Dump the beginning of the packet to understand structure
	if (s_debug_level >= 1) {
		std::cout << "First 50 bytes of ZoneSpawns packet:" << std::endl;
		for (int i = 0; i < 50 && i < p.Length(); i++) {
			std::cout << fmt::format("{:02x} ", p.GetUInt8(i));
			if ((i + 1) % 16 == 0) std::cout << std::endl;
		}
		std::cout << std::endl;
		
		// Let's analyze the hex dump more carefully:
		// At offset 9: "Renux_Herkanor000" - but this is the middle of a name
		// The name field in Spawn_Struct is at offset 7
		// If "Renux_Herkanor000" is at packet offset 9, and name is at struct offset 7,
		// then the spawn struct starts at packet offset 9 - 7 = 2
		
		// But wait - we're seeing truncated names like "e_rat005" instead of "a_large_rat005"
		// This means we're starting too late. Let's check different offsets
		
		// First, let's look for a complete name pattern
		for (int test_offset = 0; test_offset < 20 && test_offset + 70 < p.Length(); test_offset++) {
			char c = (char)p.GetUInt8(test_offset);
			if (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z') {
				std::string test_name = p.GetCString(test_offset);
				if (test_name.length() > 3 && test_name.length() < 64) {
					std::cout << fmt::format("  Possible name at offset {}: '{}'", test_offset, test_name) << std::endl;
				}
			}
		}
		
		// If we're getting "e_rat005" instead of "a_large_rat005", we're missing 6 characters
		// Current offset is 8, name field is at +7, so we're reading from packet offset 15
		// We need to start 6 bytes earlier: 8 - 6 = 2
		offset = 2;
		
		// Calculate expected spawn count based on packet size
		size_t data_size = p.Length() - offset;
		size_t expected_spawns = data_size / 385;
		std::cout << fmt::format("Packet has {} bytes of data, expecting approximately {} spawns", 
			data_size, expected_spawns) << std::endl;
	}
	*/

	// Parse Spawn_Struct entries based on titanium_structs.h
	while (offset + 385 <= p.Length()) { // Spawn_Struct is 385 bytes
		Entity entity;
		
		// Based on Spawn_Struct from titanium_structs.h:
		// name is at offset 7 (64 bytes)
		entity.name = p.GetCString(offset + 7);
		
		// If name is empty, we've likely reached the end of spawn data
		if (entity.name.empty()) {
			if (s_debug_level >= 2) {
				std::cout << fmt::format("Found empty name at offset {}, ending spawn parsing", offset) << std::endl;
			}
			break;
		}
		
		// spawnId is at offset 340 in the struct (uint32)
		entity.spawn_id = p.GetUInt32(offset + 340);
		
		// Debug: Check what's around the spawn_id location for first few spawns
		if (spawn_count < 3 && s_debug_level >= 1) {
			std::cout << fmt::format("Spawn at offset {}: Name='{}', checking spawn_id area:", offset, entity.name) << std::endl;
			for (int i = 330; i < 350 && offset + i + 4 < p.Length(); i += 4) {
				uint32_t val = p.GetUInt32(offset + i);
				if (val > 0 && val < 100000) {
					std::cout << fmt::format("  Offset +{}: u32={} (0x{:08x})", i, val, val) << std::endl;
				}
			}
		}
		
		// level is at offset 151
		entity.level = p.GetUInt8(offset + 151);
		
		// Position data from the bitfield structure:
		// x is at offset 94 (19 bits within a bitfield)
		// y is at offset 98 (19 bits)
		// z is at offset 102 (19 bits)
		// heading is at offset 106 (12 bits)
		
		// Read the bitfields
		uint32_t field1 = p.GetUInt32(offset + 94);  // contains deltaHeading and x
		uint32_t field2 = p.GetUInt32(offset + 98);  // contains y and animation
		uint32_t field3 = p.GetUInt32(offset + 102); // contains z and deltaY
		uint32_t field4 = p.GetUInt32(offset + 106); // contains deltaX and heading
		
		// Extract x, y, z (19 bits each, signed)
		// Positions are stored as fixed-point values (multiplied by 8)
		int32_t x_raw = (field1 >> 10) & 0x7FFFF;
		if (x_raw & 0x40000) x_raw |= 0xFFF80000; // Sign extend
		entity.x = static_cast<float>(x_raw) / 8.0f;
		
		int32_t y_raw = field2 & 0x7FFFF;
		if (y_raw & 0x40000) y_raw |= 0xFFF80000; // Sign extend
		entity.y = static_cast<float>(y_raw) / 8.0f;
		
		int32_t z_raw = field3 & 0x7FFFF;
		if (z_raw & 0x40000) z_raw |= 0xFFF80000; // Sign extend
		entity.z = static_cast<float>(z_raw) / 8.0f;
		
		// Extract heading (12 bits, unsigned)
		entity.heading = static_cast<float>((field4 >> 13) & 0xFFF) * 360.0f / 4096.0f;
		
		// race is at offset 284 (uint32)
		entity.race_id = p.GetUInt32(offset + 284);
		
		// class_ is at offset 331
		entity.class_id = p.GetUInt8(offset + 331);
		
		// gender is at offset 334
		entity.gender = p.GetUInt8(offset + 334);
		
		// guildID is at offset 238
		entity.guild_id = p.GetUInt32(offset + 238);
		
		// curHp is at offset 86 (hp percentage)
		entity.hp_percent = p.GetUInt8(offset + 86);
		
		// animation is in the bitfield at offset 98
		entity.animation = (field2 >> 19) & 0x3FF;
		
		// Initialize movement fields
		entity.delta_x = 0;
		entity.delta_y = 0;
		entity.delta_z = 0;
		entity.delta_heading = 0;
		entity.last_update_time = std::time(nullptr);
		
		// Validate the spawn data before adding
		if (entity.spawn_id > 0 && entity.spawn_id < 100000 && !entity.name.empty()) {
			// Check if this is our own character and update our position
			if (entity.name == m_character) {
				if (s_debug_level >= 1) {
					std::cout << fmt::format("[DEBUG] Found our character '{}' in spawn list at ({:.2f}, {:.2f}, {:.2f}), updating position",
						m_character, entity.x, entity.y, entity.z) << std::endl;
				}
				// Update our position to match server spawn position
				m_x = entity.x;
				m_y = entity.y;
				m_z = entity.z;
				// Don't update heading from spawn data as it may be scaled differently
			}
			
			// Add to entity list
			m_entities[entity.spawn_id] = entity;
			spawn_count++;
			
			if (spawn_count <= 5 || s_debug_level >= 2) {
				std::cout << fmt::format("  Loaded spawn {}: {} (ID: {}) Level {} {} Race {} at ({:.2f}, {:.2f}, {:.2f})",
					spawn_count, entity.name, entity.spawn_id, entity.level, 
					entity.class_id, entity.race_id, entity.x, entity.y, entity.z) << std::endl;
			}
		} else {
			if (s_debug_level >= 1) {
				std::cout << fmt::format("  Skipping invalid spawn at offset {}: ID={}, Name='{}'", 
					offset, entity.spawn_id, entity.name) << std::endl;
			}
		}
		
		// Move to next spawn struct
		offset += 385;
	}
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Loaded {} spawns in zone", spawn_count) << std::endl;
	}
}

void EverQuest::ZoneProcessTimeOfDay(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		uint32_t hour = p.GetUInt8(2);
		uint32_t minute = p.GetUInt8(3);
		uint32_t day = p.GetUInt8(4);
		uint32_t month = p.GetUInt8(5);
		uint32_t year = p.GetUInt16(6);
		std::cout << fmt::format("Time of day: {:02d}:{:02d} {:02d}/{:02d}/{}",
			hour, minute, day, month, year) << std::endl;
	}
}

void EverQuest::ZoneProcessSpawnDoor(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received spawn door" << std::endl;
	}
}

void EverQuest::ZoneProcessSendZonepoints(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received zone points" << std::endl;
	}
	
	// After zone points, check if we need to send server filter
	// This is old logic - server filter is now sent after GuildMOTD
	// But keeping it here for compatibility if the flow is different
	if (m_zone_entry_sent && !m_client_ready_sent) {
		// This path shouldn't normally be taken anymore
		if (!m_server_filter_sent && m_send_exp_zonein_received) {
			if (s_debug_level >= 1) {
				std::cout << "ZoneProcessSendZonepoints calling ZoneSendSetServerFilter (fallback)" << std::endl;
			}
			ZoneSendSetServerFilter();
		}

		// Send client ready - This shouldn't happen here anymore
		if (!m_client_ready_sent && m_server_filter_sent) {
			std::cout << "ZoneProcessSendZonepoints calling ZoneSendClientReady" << std::endl;
			ZoneSendClientReady();
		}
		

		// std::cout << "Zone connection complete! Headless client is now in the zone." << std::endl;
	}
}

void EverQuest::ZoneProcessSpawnAppearance(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 2) {
		std::cout << "Spawn appearance update" << std::endl;
	}
}

void EverQuest::ZoneProcessGroundSpawn(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 2) {
		std::cout << "Ground spawn" << std::endl;
	}
}

void EverQuest::ZoneProcessWeather(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Weather update received" << std::endl;
	}
	
	m_weather_received = true;
	
	// Weather is the last packet in Zone Entry phase
	// Now we can send ReqNewZone
	if (!m_req_new_zone_sent) {
		ZoneSendReqNewZone();
	}
}

void EverQuest::ZoneProcessNewSpawn(const EQ::Net::Packet &p)
{
	// NewSpawn packet is 387 bytes total: opcode (2 bytes) + Spawn_Struct (385 bytes)
	// The spawn_id is embedded within the Spawn_Struct, not separate
	
	if (p.Length() < 387) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("NewSpawn packet too small: {} bytes (expected 387)", p.Length()) << std::endl;
		}
		return;
	}
	
	Entity entity;
	
	// Spawn_Struct starts at offset 2 (after opcode)
	size_t offset = 2;
	
	// Use the same parsing logic as ZoneSpawns
	// spawnId is at offset 340 in the struct (uint32)
	entity.spawn_id = p.GetUInt32(offset + 340);
	
	// Name is at offset 7 in Spawn_Struct
	entity.name = p.GetCString(offset + 7);
	
	// Level at offset 151
	entity.level = p.GetUInt8(offset + 151);
	
	// Position data from the bitfield structure (same as ZoneSpawns):
	uint32_t field1 = p.GetUInt32(offset + 94);  // contains deltaHeading and x
	uint32_t field2 = p.GetUInt32(offset + 98);  // contains y and animation
	uint32_t field3 = p.GetUInt32(offset + 102); // contains z and deltaY
	uint32_t field4 = p.GetUInt32(offset + 106); // contains deltaX and heading
	
	// Extract x, y, z (19 bits each, signed) - positions are in 10ths
	int32_t x_raw = (field1 >> 10) & 0x7FFFF;
	if (x_raw & 0x40000) x_raw |= 0xFFF80000; // Sign extend
	entity.x = static_cast<float>(x_raw);
	
	int32_t y_raw = field2 & 0x7FFFF;
	if (y_raw & 0x40000) y_raw |= 0xFFF80000; // Sign extend
	entity.y = static_cast<float>(y_raw);
	
	int32_t z_raw = field3 & 0x7FFFF;
	if (z_raw & 0x40000) z_raw |= 0xFFF80000; // Sign extend
	entity.z = static_cast<float>(z_raw);
	
	// Extract heading (12 bits, unsigned)
	entity.heading = static_cast<float>((field4 >> 13) & 0xFFF) * 360.0f / 4096.0f;
	
	// race is at offset 284 (uint32)
	entity.race_id = p.GetUInt32(offset + 284);
	
	// class_ is at offset 331
	entity.class_id = p.GetUInt8(offset + 331);
	
	// gender is at offset 334
	entity.gender = p.GetUInt8(offset + 334);
	
	// guildID is at offset 238
	entity.guild_id = p.GetUInt32(offset + 238);
	
	// curHp is at offset 86 (hp percentage)
	entity.hp_percent = p.GetUInt8(offset + 86);
	
	// animation is in the bitfield at offset 98
	entity.animation = (field2 >> 19) & 0x3FF;
	
	// Initialize movement fields
	entity.delta_x = 0;
	entity.delta_y = 0;
	entity.delta_z = 0;
	entity.delta_heading = 0;
	entity.last_update_time = std::time(nullptr);
	
	// Validate the spawn data before adding
	if (entity.spawn_id > 0 && entity.spawn_id < 100000 && !entity.name.empty()) {
		// Check if this is our own spawn by comparing name
		if (entity.name == m_character) {
			m_my_spawn_id = entity.spawn_id;
			if (s_debug_level >= 1) {
				std::cout << fmt::format("[DEBUG] Found our own spawn in NewSpawn! Name: {}, Spawn ID: {}, updating position to ({:.2f}, {:.2f}, {:.2f})", 
					entity.name, m_my_spawn_id, entity.x, entity.y, entity.z) << std::endl;
			}
			// Update our position to match server spawn position
			m_x = entity.x;
			m_y = entity.y;
			m_z = entity.z;
		}
		
		// Add to entity list
		m_entities[entity.spawn_id] = entity;
		
		if (s_debug_level >= 1) {
			std::cout << fmt::format("New spawn: {} (ID: {}) Level {} {} Race {} at ({:.2f}, {:.2f}, {:.2f})",
				entity.name, entity.spawn_id, entity.level, 
				entity.class_id, entity.race_id, entity.x, entity.y, entity.z) << std::endl;
		}
	} else {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Invalid spawn data in NewSpawn: ID={}, Name='{}'", 
				entity.spawn_id, entity.name) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessTributeUpdate(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received tribute update" << std::endl;
	}
}

void EverQuest::ZoneProcessTributeTimer(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received tribute timer" << std::endl;
	}
}

void EverQuest::ZoneProcessSendAATable(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received AA table data" << std::endl;
	}
	m_aa_table_count++;
	
	// Check if all Zone Request phase responses received
	CheckZoneRequestPhaseComplete();
}

void EverQuest::ZoneProcessRespondAA(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received AA response" << std::endl;
	}
	
	// Check if all Zone Request phase responses received
	CheckZoneRequestPhaseComplete();
}

void EverQuest::ZoneProcessTributeInfo(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received tribute info" << std::endl;
	}
	m_tribute_count++;
	
	// Check if all Zone Request phase responses received
	CheckZoneRequestPhaseComplete();
}

void EverQuest::ZoneProcessSendGuildTributes(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received guild tributes" << std::endl;
	}
	m_guild_tribute_count++;
	
	// Check if all Zone Request phase responses received
	CheckZoneRequestPhaseComplete();
}

void EverQuest::ZoneProcessSendAAStats(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received AA stats" << std::endl;
	}
}

void EverQuest::ZoneProcessSendExpZonein(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received exp zone in - this triggers SendZoneInPackets()" << std::endl;
	}
	
	m_send_exp_zonein_received = true;
	
	// ExpZonein triggers the server to send ExpUpdate, RaidUpdate, and GuildMOTD
	// We'll wait for GuildMOTD before sending final packets
}

void EverQuest::ZoneProcessWorldObjectsSent(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received world objects sent" << std::endl;
	}
	
	// After world objects, send exp zonein if not already sent
	if (!m_exp_zonein_sent) {
		ZoneSendSendExpZonein();
	}
}

void EverQuest::ZoneProcessExpUpdate(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received exp update" << std::endl;
	}
}

void EverQuest::ZoneProcessRaidUpdate(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received raid update" << std::endl;
	}
}

void EverQuest::ZoneProcessGuildMOTD(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Received guild MOTD" << std::endl;
	}
	
	// GuildMOTD is the last packet from SendZoneInPackets()
	// Now we send SetServerFilter and ClientReady
	if (!m_server_filter_sent) {
		ZoneSendSetServerFilter();
		m_server_filter_sent = true;
	}
	
	if (!m_client_ready_sent) {
		ZoneSendClientReady();
		m_client_ready_sent = true;
		
		std::cout << "Zone connection complete! Headless client is now in the zone." << std::endl;
		if (s_debug_level >= 0) {
			// std::cout << "\nFully connected to zone! Processing commands now.\n" << std::endl;
		}
	}
}

void EverQuest::ZoneProcessClientUpdate(const EQ::Net::Packet &p)
{
	// ClientUpdate packets contain position updates from other players
	// Format based on PlayerPositionUpdateServer_Struct (bit-packed):
	// spawn_id (2 bytes) + 20 bytes of bit-packed data
	
	if (p.Length() < 24) { // 2 (opcode) + 2 (spawn_id) + 20 (bit-packed data)
		if (s_debug_level >= 1) {
			std::cout << fmt::format("ClientUpdate packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	// Read spawn_id (first 2 bytes after opcode)
	uint16_t spawn_id = p.GetUInt16(2);
	
	// Read the bit-packed data as 5 32-bit integers
	uint32_t field1 = p.GetUInt32(4);  // delta_heading:10, x_pos:19, padding:3
	uint32_t field2 = p.GetUInt32(8);  // y_pos:19, animation:10, padding:3
	uint32_t field3 = p.GetUInt32(12); // z_pos:19, delta_y:13
	uint32_t field4 = p.GetUInt32(16); // delta_x:13, heading:12, padding:7
	uint32_t field5 = p.GetUInt32(20); // delta_z:13, padding:19
	
	// Extract values from bit fields
	int32_t delta_heading = (field1 & 0x3FF);          // bits 0-9 (10 bits)
	int32_t x_pos_raw = (field1 >> 10) & 0x7FFFF;      // bits 10-28 (19 bits)
	
	int32_t y_pos_raw = (field2 & 0x7FFFF);            // bits 0-18 (19 bits)
	uint32_t animation = (field2 >> 19) & 0x3FF;       // bits 19-28 (10 bits)
	
	int32_t z_pos_raw = (field3 & 0x7FFFF);            // bits 0-18 (19 bits)
	int32_t delta_y = (field3 >> 19) & 0x1FFF;         // bits 19-31 (13 bits)
	
	int32_t delta_x = (field4 & 0x1FFF);               // bits 0-12 (13 bits)
	uint32_t heading = (field4 >> 13) & 0xFFF;         // bits 13-24 (12 bits)
	
	int32_t delta_z = (field5 & 0x1FFF);               // bits 0-12 (13 bits)
	
	// Sign extend delta values (they are signed)
	if (delta_heading & 0x200) delta_heading |= 0xFFFFFC00; // sign extend from 10 bits
	if (delta_x & 0x1000) delta_x |= 0xFFFFE000;           // sign extend from 13 bits
	if (delta_y & 0x1000) delta_y |= 0xFFFFE000;           // sign extend from 13 bits
	if (delta_z & 0x1000) delta_z |= 0xFFFFE000;           // sign extend from 13 bits
	
	// Sign extend position values (they are signed 19-bit integers)
	if (x_pos_raw & 0x40000) x_pos_raw |= 0xFFF80000;      // sign extend from 19 bits
	if (y_pos_raw & 0x40000) y_pos_raw |= 0xFFF80000;      // sign extend from 19 bits
	if (z_pos_raw & 0x40000) z_pos_raw |= 0xFFF80000;      // sign extend from 19 bits
	
	// Convert raw position values to floats (divided by 8 for fixed-point)
	float x = static_cast<float>(x_pos_raw) / 8.0f;
	float y = static_cast<float>(y_pos_raw) / 8.0f;
	float z = static_cast<float>(z_pos_raw) / 8.0f;
	float dx = static_cast<float>(delta_x) / 8.0f;
	float dy = static_cast<float>(delta_y) / 8.0f;
	float dz = static_cast<float>(delta_z) / 8.0f;
	float dh = static_cast<float>(delta_heading);
	float h = static_cast<float>(heading) * 360.0f / 4096.0f;
	
	// Check if this is an update for our own character
	if (spawn_id == m_my_character_id) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("[DEBUG] Received ClientUpdate for our character! spawn_id={}, pos=({:.2f},{:.2f},{:.2f}), heading={:.1f}",
				spawn_id, x, y, z, h) << std::endl;
		}
		// Update our position to match what the server says
		m_x = x;
		m_y = y;
		m_z = z;
		m_heading = h;
		return;
	}
	
	// Update entity position in our list
	auto it = m_entities.find(spawn_id);
	if (it != m_entities.end()) {
		it->second.x = x;
		it->second.y = y;
		it->second.z = z;
		it->second.heading = h;
		it->second.animation = animation;
		it->second.delta_x = dx;
		it->second.delta_y = dy;
		it->second.delta_z = dz;
		it->second.delta_heading = dh;
		it->second.last_update_time = std::time(nullptr);
		
		if (s_debug_level >= 3) {
			std::cout << fmt::format("ClientUpdate: {} (ID:{}) at ({:.2f},{:.2f},{:.2f}) heading {:.1f}°",
				it->second.name, spawn_id, x, y, z, h) << std::endl;
		}
	} else if (s_debug_level >= 2) {
		std::cout << fmt::format("ClientUpdate for unknown spawn_id: {}", spawn_id) << std::endl;
	}
}

void EverQuest::ZoneProcessDeleteSpawn(const EQ::Net::Packet &p)
{
	// DeleteSpawn is sent when an entity despawns
	// Format: spawn_id (2 bytes)
	
	if (p.Length() < 4) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("DeleteSpawn packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	uint16_t spawn_id = p.GetUInt16(2);
	
	auto it = m_entities.find(spawn_id);
	if (it != m_entities.end()) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Entity {} ({}) despawned", spawn_id, it->second.name) << std::endl;
		}
		m_entities.erase(it);
	} else {
		if (s_debug_level >= 2) {
			std::cout << fmt::format("DeleteSpawn for unknown spawn_id: {}", spawn_id) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessMobHealth(const EQ::Net::Packet &p)
{
	// MobHealth updates an entity's health percentage
	// Format: spawn_id (2 bytes), hp_percent (1 byte)
	
	if (p.Length() < 5) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("MobHealth packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	uint16_t spawn_id = p.GetUInt16(2);
	uint8_t hp_percent = p.GetUInt8(4);
	
	auto it = m_entities.find(spawn_id);
	if (it != m_entities.end()) {
		it->second.hp_percent = hp_percent;
		if (s_debug_level >= 2) {
			std::cout << fmt::format("Entity {} ({}) health: {}%", 
				spawn_id, it->second.name, hp_percent) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessHPUpdate(const EQ::Net::Packet &p)
{
	// HPUpdate is sent for the player's own HP/Mana/Endurance
	// Format: cur_hp (4 bytes), max_hp (4 bytes), cur_mana (2 bytes)
	// Note: Titanium packet is only 12 bytes total (2 opcode + 10 data)
	
	if (p.Length() < 12) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("HPUpdate packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	uint32_t cur_hp = p.GetUInt32(2);
	uint32_t max_hp = p.GetUInt32(6);
	uint16_t cur_mana = p.GetUInt16(10);
	uint16_t cur_end = 0; // Not included in Titanium
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Player HP: {}/{}, Mana: {}, End: {}", 
			cur_hp, max_hp, cur_mana, cur_end) << std::endl;
	}
	
	// Update our own entity if we know our spawn ID
	if (m_my_spawn_id != 0) {
		auto it = m_entities.find(m_my_spawn_id);
		if (it != m_entities.end()) {
			it->second.hp_percent = (max_hp > 0) ? (cur_hp * 100 / max_hp) : 100;
		}
	}
}

void EverQuest::CheckZoneRequestPhaseComplete()
{
	// Check if we've received all expected responses from Zone Request phase
	// We need to have received:
	// 1. At least one AA table (m_aa_table_count > 0)
	// 2. RespondAA packet
	// 3. At least one tribute info (m_tribute_count > 0) 
	// 4. At least one guild tribute (m_guild_tribute_count > 0)
	
	// For simplicity, we'll check if we've received at least one of each
	if (m_new_zone_received && 
	    m_aa_table_count > 0 && 
	    m_tribute_count > 0 && 
	    m_guild_tribute_count > 0 &&
	    !m_req_client_spawn_sent) {
		
		if (s_debug_level >= 1) {
			std::cout << "Zone Request phase complete, sending ReqClientSpawn" << std::endl;
		}
		
		// Move to Player Spawn Request phase
		ZoneSendReqClientSpawn();
		m_req_client_spawn_sent = true;
	}
}

void EverQuest::ZoneSendChannelMessage(const std::string &message, ChatChannelType channel, const std::string &target)
{
	// Based on ChannelMessage_Struct from titanium_structs.h:
	// targetname[64], sender[64], language(4), chan_num(4), unknown(8), skill(4), message[]
	
	// For channel messages, we need to include the null terminator
	size_t message_len = message.length();
	size_t packet_size = 150 + message_len + 1; // +1 for null terminator
	
	EQ::Net::DynamicPacket p;
	p.Resize(packet_size);
	
	p.PutUInt16(0, HC_OP_ChannelMessage);
	
	// Clear the entire packet to ensure null terminators
	memset((char*)p.Data() + 2, 0, packet_size - 2);
	
	// Target name (64 bytes) - used for tells
	if (!target.empty()) {
		size_t target_len = target.length();
		if (target_len > 63) target_len = 63;
		memcpy((char*)p.Data() + 2, target.c_str(), target_len);
		// Null terminator already set by memset
	}
	
	// Sender name (64 bytes) - this is our character name
	size_t name_len = m_character.length();
	if (name_len > 63) name_len = 63;
	memcpy((char*)p.Data() + 66, m_character.c_str(), name_len);
	// Null terminator already set by memset
	
	// Language (0 = common tongue)
	p.PutUInt32(130, 0);
	
	// Channel number
	p.PutUInt32(134, static_cast<uint32_t>(channel));
	
	// Unknown fields (8 bytes) - already 0 from memset
	
	// Skill in language (100 = perfect)
	p.PutUInt32(146, 100);
	
	// Message - copy only the characters, null terminator already set
	memcpy((char*)p.Data() + 150, message.c_str(), message_len);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Sending {} message: '{}'", 
			channel == CHAT_CHANNEL_SAY ? "say" :
			channel == CHAT_CHANNEL_TELL ? "tell" :
			channel == CHAT_CHANNEL_SHOUT ? "shout" :
			channel == CHAT_CHANNEL_OOC ? "ooc" :
			channel == CHAT_CHANNEL_AUCTION ? "auction" :
			channel == CHAT_CHANNEL_GROUP ? "group" :
			channel == CHAT_CHANNEL_GUILD ? "guild" : "unknown",
			message) << std::endl;
	}
	
	DumpPacket("C->S", HC_OP_ChannelMessage, p);
	m_zone_connection->QueuePacket(p);
}

void EverQuest::ZoneProcessChannelMessage(const EQ::Net::Packet &p)
{
	// Process incoming channel messages
	if (p.Length() < 150) { // Minimum size for ChannelMessage_Struct
		if (s_debug_level >= 1) {
			std::cout << fmt::format("ChannelMessage packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	std::string target = p.GetCString(2);
	std::string sender = p.GetCString(66);
	uint32_t language = p.GetUInt32(130);
	uint32_t channel = p.GetUInt32(134);
	uint32_t skill = p.GetUInt32(146);
	std::string message = p.GetCString(150);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("[CHAT] {} ({}): {}", 
			sender,
			channel == CHAT_CHANNEL_SAY ? "say" :
			channel == CHAT_CHANNEL_TELL ? "tell" :
			channel == CHAT_CHANNEL_SHOUT ? "shout" :
			channel == CHAT_CHANNEL_OOC ? "ooc" :
			channel == CHAT_CHANNEL_GROUP ? "group" :
			channel == CHAT_CHANNEL_GUILD ? "guild" :
			channel == CHAT_CHANNEL_EMOTE ? "emote" : 
			fmt::format("chan{}", channel),
			message) << std::endl;
			
		if (!target.empty() && channel == CHAT_CHANNEL_TELL) {
			std::cout << fmt::format("  (Tell to: {})", target) << std::endl;
		}
	}
}

void EverQuest::SendChatMessage(const std::string &message, const std::string &channel_name, const std::string &target)
{
	// Convert channel name to ChatChannelType
	ChatChannelType channel;
	
	// Convert to lowercase for comparison
	std::string channel_lower = channel_name;
	std::transform(channel_lower.begin(), channel_lower.end(), channel_lower.begin(), ::tolower);
	
	if (channel_lower == "say") {
		channel = CHAT_CHANNEL_SAY;
	} else if (channel_lower == "tell") {
		channel = CHAT_CHANNEL_TELL;
		if (target.empty()) {
			std::cout << "Error: Tell requires a target player name" << std::endl;
			return;
		}
	} else if (channel_lower == "shout") {
		channel = CHAT_CHANNEL_SHOUT;
	} else if (channel_lower == "ooc") {
		channel = CHAT_CHANNEL_OOC;
	} else if (channel_lower == "group") {
		channel = CHAT_CHANNEL_GROUP;
	} else if (channel_lower == "guild") {
		channel = CHAT_CHANNEL_GUILD;
	} else if (channel_lower == "auction") {
		channel = CHAT_CHANNEL_AUCTION;
	} else if (channel_lower == "emote") {
		channel = CHAT_CHANNEL_EMOTE;
	} else {
		std::cout << fmt::format("Unknown channel: '{}'. Valid channels: say, tell, shout, ooc, group, guild, auction, emote", channel_name) << std::endl;
		return;
	}
	
	// Check if we're connected to the zone
	if (!m_zone_connection || !m_zone_connected) {
		std::cout << "Error: Not connected to zone server" << std::endl;
		return;
	}
	
	// Route messages based on channel type
	// For now, route all messages through zone server since UCS implementation needs fixing
	ZoneSendChannelMessage(message, channel, target);
}

// UCS (Universal Chat Service) connection implementation
// TODO: Fix UCS implementation to use correct EQStream API
/*
void EverQuest::ConnectToUCS(const std::string& host, uint16_t port)
{
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Connecting to UCS at {}:{}", host, port) << std::endl;
	}
	
	// UCS uses EQStream with legacy options (1-byte opcodes)
	EQ::Net::EQStreamManagerInterfaceOptions ucs_opts(port, false, false);
	ucs_opts.opcode_size = 1;  // Legacy 1-byte opcodes
	ucs_opts.daybreak_options.stale_connection_ms = 600000;
	
	m_ucs_stream_manager = std::make_unique<EQ::Net::EQStreamManager>(ucs_opts);
	
	m_ucs_stream_manager->OnNewConnection([this](std::shared_ptr<EQ::Net::EQStream> stream) {
		UCSOnNewConnection(stream);
	});
	
	m_ucs_stream_manager->OnConnectionLost([this](std::shared_ptr<EQ::Net::EQStream> stream) {
		UCSOnConnectionLost();
	});
	
	// Connect to UCS server
	m_ucs_stream = m_ucs_stream_manager->CreateConnection(host, port);
	m_ucs_stream->SetStreamType(EQ::Net::EQStreamType::Chat);
}

void EverQuest::UCSOnNewConnection(std::shared_ptr<EQ::Net::EQStream> stream)
{
	if (s_debug_level >= 1) {
		std::cout << "UCS connection established" << std::endl;
	}
	
	m_ucs_stream = stream;
	m_ucs_connected = true;
	
	// Set packet callback
	m_ucs_stream->SetPacketCallback([this](const EQ::Net::Packet &p) {
		UCSOnPacketRecv(p);
	});
	
	// Send mail login packet to authenticate
	UCSSendMailLogin();
}

void EverQuest::UCSOnConnectionLost()
{
	if (s_debug_level >= 1) {
		std::cout << "UCS connection lost" << std::endl;
	}
	
	m_ucs_connected = false;
	m_ucs_stream.reset();
}

void EverQuest::UCSOnPacketRecv(const EQ::Net::Packet &p)
{
	if (p.Length() < 1) return;
	
	uint8_t opcode = p.GetUInt8(0);
	
	if (s_debug_level >= 2) {
		std::cout << fmt::format("UCS packet received: opcode={:#04x}, size={}", opcode, p.Length()) << std::endl;
	}
	
	switch (opcode) {
		case HC_OP_ChatMessage:
			UCSProcessChatMessage(p);
			break;
		case HC_OP_UCS_MailNew:
			UCSProcessMailNew(p);
			break;
		case HC_OP_UCS_Buddy:
			UCSProcessBuddy(p);
			break;
		default:
			if (s_debug_level >= 1) {
				std::cout << fmt::format("Unhandled UCS opcode: {:#04x}", opcode) << std::endl;
			}
			break;
	}
}

void EverQuest::UCSSendMailLogin()
{
	if (!m_ucs_stream || !m_ucs_connected) {
		return;
	}
	
	// MailLogin packet format: opcode(1) + mail_key(null-terminated string)
	size_t key_len = m_mail_key.length() + 1; // Include null terminator
	size_t packet_size = 1 + key_len;
	
	EQ::Net::DynamicPacket p;
	p.Resize(packet_size);
	
	p.PutUInt8(0, HC_OP_UCS_MailLogin);
	memcpy((char*)p.Data() + 1, m_mail_key.c_str(), key_len);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Sending UCS MailLogin with key: {}", m_mail_key) << std::endl;
	}
	
	m_ucs_stream->QueuePacket(p);
}

void EverQuest::UCSSendTell(const std::string &to, const std::string &message)
{
	if (!m_ucs_stream || !m_ucs_connected) {
		if (s_debug_level >= 1) {
			std::cout << "UCS not connected, cannot send tell" << std::endl;
		}
		return;
	}
	
	// Tell format via UCS: opcode(1) + target(null-terminated) + sender(null-terminated) + message(null-terminated)
	size_t to_len = to.length() + 1;
	size_t from_len = m_character.length() + 1;
	size_t msg_len = message.length() + 1;
	size_t packet_size = 1 + to_len + from_len + msg_len;
	
	EQ::Net::DynamicPacket p;
	p.Resize(packet_size);
	
	size_t offset = 0;
	p.PutUInt8(offset++, HC_OP_ChatMessage);
	
	memcpy((char*)p.Data() + offset, to.c_str(), to_len);
	offset += to_len;
	
	memcpy((char*)p.Data() + offset, m_character.c_str(), from_len);
	offset += from_len;
	
	memcpy((char*)p.Data() + offset, message.c_str(), msg_len);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Sending UCS tell to {}: {}", to, message) << std::endl;
	}
	
	m_ucs_stream->QueuePacket(p);
}

void EverQuest::UCSSendJoinChannel(const std::string &channel)
{
	if (!m_ucs_stream || !m_ucs_connected) {
		return;
	}
	
	// Join channel format: opcode(1) + channel_name(null-terminated) + character_name(null-terminated)
	size_t ch_len = channel.length() + 1;
	size_t name_len = m_character.length() + 1;
	size_t packet_size = 1 + ch_len + name_len;
	
	EQ::Net::DynamicPacket p;
	p.Resize(packet_size);
	
	size_t offset = 0;
	p.PutUInt8(offset++, HC_OP_UCS_ChatJoin);
	
	memcpy((char*)p.Data() + offset, channel.c_str(), ch_len);
	offset += ch_len;
	
	memcpy((char*)p.Data() + offset, m_character.c_str(), name_len);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Joining UCS channel: {}", channel) << std::endl;
	}
	
	m_ucs_stream->QueuePacket(p);
}

void EverQuest::UCSSendLeaveChannel(const std::string &channel)
{
	if (!m_ucs_stream || !m_ucs_connected) {
		return;
	}
	
	// Leave channel format: opcode(1) + channel_name(null-terminated) + character_name(null-terminated)
	size_t ch_len = channel.length() + 1;
	size_t name_len = m_character.length() + 1;
	size_t packet_size = 1 + ch_len + name_len;
	
	EQ::Net::DynamicPacket p;
	p.Resize(packet_size);
	
	size_t offset = 0;
	p.PutUInt8(offset++, HC_OP_UCS_ChatLeave);
	
	memcpy((char*)p.Data() + offset, channel.c_str(), ch_len);
	offset += ch_len;
	
	memcpy((char*)p.Data() + offset, m_character.c_str(), name_len);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Leaving UCS channel: {}", channel) << std::endl;
	}
	
	m_ucs_stream->QueuePacket(p);
}

void EverQuest::UCSProcessChatMessage(const EQ::Net::Packet &p)
{
	if (p.Length() < 2) return;
	
	// UCS chat message format: opcode(1) + channel(null-terminated) + sender(null-terminated) + message(null-terminated)
	size_t offset = 1; // Skip opcode
	
	std::string channel = p.GetCString(offset);
	offset += channel.length() + 1;
	
	if (offset >= p.Length()) return;
	
	std::string sender = p.GetCString(offset);
	offset += sender.length() + 1;
	
	if (offset >= p.Length()) return;
	
	std::string message = p.GetCString(offset);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("[UCS] {}.{}: {}", channel, sender, message) << std::endl;
	}
}

void EverQuest::UCSProcessMailNew(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "New mail notification received" << std::endl;
	}
}

void EverQuest::UCSProcessBuddy(const EQ::Net::Packet &p)
{
	if (s_debug_level >= 1) {
		std::cout << "Buddy list update received" << std::endl;
	}
}
*/

// Movement implementation
void EverQuest::Move(float x, float y, float z)
{
	// Use pathfinding by default
	MoveToWithPath(x, y, z);
	
	// Block until movement is complete
	while (m_is_moving && IsFullyZonedIn()) {
		// Process network events while waiting
		EQ::EventLoop::Get().Process();
		UpdateMovement();
		std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
	}
}

void EverQuest::MoveToEntity(const std::string &name)
{
	Entity* entity = FindEntityByName(name);
	if (entity) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Found entity '{}' at ({:.2f}, {:.2f}, {:.2f})", 
				entity->name, entity->x, entity->y, entity->z) << std::endl;
		}
		Move(entity->x, entity->y, entity->z);  // This will block until complete
	} else {
		std::cout << fmt::format("Entity '{}' not found", name) << std::endl;
	}
}

void EverQuest::Follow(const std::string &name)
{
	Entity* entity = FindEntityByName(name);
	if (entity) {
		m_follow_target = entity->name;  // Store the actual entity name
		std::cout << fmt::format("Following {}", entity->name) << std::endl;
		
		// Calculate initial path if pathfinding is enabled
		if (m_use_pathfinding && m_pathfinder) {
			std::cout << fmt::format("[DEBUG] Follow: Pathfinding enabled, m_pathfinder={}", 
				m_pathfinder ? "valid" : "null") << std::endl;
			float dist = CalculateDistance2D(m_x, m_y, entity->x, entity->y);
			std::cout << fmt::format("[DEBUG] Follow: Distance to target: {:.2f}, follow_distance: {:.2f}", 
				dist, m_follow_distance) << std::endl;
			if (dist > m_follow_distance) {
				// Find path from current position to target
				std::cout << fmt::format("[DEBUG] Follow: Calculating path from ({:.2f},{:.2f},{:.2f}) to ({:.2f},{:.2f},{:.2f})",
					m_x, m_y, m_z, entity->x, entity->y, entity->z) << std::endl;
				if (FindPath(m_x, m_y, m_z, entity->x, entity->y, entity->z)) {
					// Start following the path
					m_current_path_index = 0;
					m_is_moving = true;
					std::cout << fmt::format("[DEBUG] Follow: Path calculated successfully with {} waypoints", 
						m_current_path.size()) << std::endl;
					for (size_t i = 0; i < std::min(m_current_path.size(), size_t(5)); i++) {
						std::cout << fmt::format("  Waypoint {}: ({:.2f},{:.2f},{:.2f})", 
							i, m_current_path[i].x, m_current_path[i].y, m_current_path[i].z) << std::endl;
					}
				} else {
					// Pathfinding failed, use direct movement
					std::cout << "[DEBUG] Follow: Pathfinding failed, using direct movement" << std::endl;
					m_target_x = entity->x;
					m_target_y = entity->y;
					m_target_z = entity->z;
					m_is_moving = true;
				}
			}
		} else {
			std::cout << fmt::format("[DEBUG] Follow: Pathfinding disabled (m_use_pathfinding={}, m_pathfinder={})",
				m_use_pathfinding, m_pathfinder ? "valid" : "null") << std::endl;
		}
	} else {
		std::cout << fmt::format("Entity '{}' not found", name) << std::endl;
	}
}

void EverQuest::StopFollow()
{
	if (!m_follow_target.empty()) {
		std::cout << fmt::format("Stopped following {}", m_follow_target) << std::endl;
		m_follow_target.clear();
	}
	StopMovement();
}

void EverQuest::Face(float x, float y, float z)
{
	// Calculate heading to face the given position
	float new_heading = CalculateHeading(m_x, m_y, x, y);
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("[DEBUG] Face: current pos ({:.1f},{:.1f}), target ({:.1f},{:.1f}), old heading {:.1f}°, new heading {:.1f}°",
			m_x, m_y, x, y, m_heading, new_heading) << std::endl;
	}
	
	// Always update heading - even small changes should be sent
	// The previous 0.1f threshold was preventing the face command from working
	m_heading = new_heading;
	SendPositionUpdate();
}

void EverQuest::FaceEntity(const std::string &name)
{
	Entity* entity = FindEntityByName(name);
	if (entity) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Facing entity '{}'", entity->name) << std::endl;
		}
		Face(entity->x, entity->y, entity->z);
	} else {
		std::cout << fmt::format("Entity '{}' not found", name) << std::endl;
	}
}

void EverQuest::MoveTo(float x, float y, float z)
{
	m_target_x = x;
	m_target_y = y;
	m_target_z = z;
	m_is_moving = true;
	
	// Calculate initial heading towards target
	float new_heading = CalculateHeading(m_x, m_y, x, y);
	m_heading = new_heading;
	// Send immediate position update to show the heading change
	SendPositionUpdate();
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Moving to ({:.2f}, {:.2f}, {:.2f}) with heading {:.1f}", x, y, z, m_heading) << std::endl;
	}
}

void EverQuest::StopMovement()
{
	if (m_is_moving) {
		m_is_moving = false;
		m_animation = ANIM_STAND;
		
		// Clear any active path
		m_current_path.clear();
		m_current_path_index = 0;
		
		SendPositionUpdate();
		
		if (s_debug_level >= 1) {
			std::cout << "Movement stopped" << std::endl;
		}
	}
}

void EverQuest::UpdateMovement()
{
	// Check if we're following someone
	if (!m_follow_target.empty()) {
		// Find the entity we're following
		for (const auto& pair : m_entities) {
			const Entity& entity = pair.second;
			if (entity.name == m_follow_target) {
				// Check if we're close enough
				float dist = CalculateDistance2D(m_x, m_y, entity.x, entity.y);
				if (dist < m_follow_distance) {
					// We're close enough, stop if we were moving
					if (m_is_moving) {
						StopMovement();
					}
					return;
				} else if (dist > m_follow_distance * 1.5f) {
					// Target has moved significantly, recalculate path
					// Check if final destination changed significantly
					float final_dest_dist = 0.0f;
					if (!m_current_path.empty() && m_current_path.size() > 0) {
						// Compare with the final waypoint in our path
						const auto& final_waypoint = m_current_path.back();
						final_dest_dist = CalculateDistance2D(final_waypoint.x, final_waypoint.y, entity.x, entity.y);
					} else {
						// No path, compare with our immediate target
						final_dest_dist = CalculateDistance2D(m_target_x, m_target_y, entity.x, entity.y);
					}
					
					if (final_dest_dist > 5.0f || m_current_path.empty()) {
						// Recalculate path using pathfinding
						std::cout << fmt::format("[DEBUG] UpdateMovement: Target moved significantly (dist={:.2f})", final_dest_dist) << std::endl;
						if (m_use_pathfinding && m_pathfinder) {
							// Find path from current position to target
							std::cout << fmt::format("[DEBUG] UpdateMovement: Recalculating path from ({:.2f},{:.2f},{:.2f}) to ({:.2f},{:.2f},{:.2f})",
								m_x, m_y, m_z, entity.x, entity.y, entity.z) << std::endl;
							if (FindPath(m_x, m_y, m_z, entity.x, entity.y, entity.z)) {
								// Start following the path
								m_current_path_index = 0;
								m_is_moving = true;
								std::cout << fmt::format("[DEBUG] UpdateMovement: Path recalculated with {} waypoints", 
									m_current_path.size()) << std::endl;
								for (size_t i = 0; i < std::min(m_current_path.size(), size_t(3)); i++) {
									std::cout << fmt::format("  Waypoint {}: ({:.2f},{:.2f},{:.2f})", 
										i, m_current_path[i].x, m_current_path[i].y, m_current_path[i].z) << std::endl;
								}
							} else {
								// Pathfinding failed, use direct movement
								std::cout << "[DEBUG] UpdateMovement: Pathfinding failed, using direct movement" << std::endl;
								m_target_x = entity.x;
								m_target_y = entity.y;
								m_target_z = entity.z;
								m_is_moving = true;
							}
						} else {
							// Fallback to direct movement
							std::cout << "[DEBUG] UpdateMovement: Pathfinding disabled, using direct movement" << std::endl;
							m_target_x = entity.x;
							m_target_y = entity.y;
							m_target_z = entity.z;
							m_is_moving = true;
						}
					}
				}
				break;
			}
		}
	}
	
	if (!m_is_moving) {
		// Send idle position updates like real clients (every 1-2 seconds)
		static std::map<EverQuest*, std::chrono::steady_clock::time_point> last_idle_updates;
		if (last_idle_updates.find(this) == last_idle_updates.end()) {
			last_idle_updates[this] = std::chrono::steady_clock::now();
		}
		
		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_idle_updates[this]);
		
		if (elapsed.count() >= 1500) {  // 1.5 seconds average
			SendPositionUpdate();
			last_idle_updates[this] = now;
		}
		return;
	}
	
	// Check if we're following a path
	if (!m_current_path.empty() && m_current_path_index < m_current_path.size()) {
		// Check if we've reached the current waypoint
		const auto& waypoint = m_current_path[m_current_path_index];
		float dist_to_waypoint = CalculateDistance2D(m_x, m_y, waypoint.x, waypoint.y);
		
		if (s_debug_level >= 2) {
			std::cout << fmt::format("[DEBUG] Following path: waypoint {}/{}, dist to waypoint: {:.2f}", 
				m_current_path_index, m_current_path.size()-1, dist_to_waypoint) << std::endl;
		}
		
		// Stuck detection - track if we're not making progress
		static std::map<EverQuest*, std::pair<float, std::chrono::steady_clock::time_point>> stuck_detection;
		auto now = std::chrono::steady_clock::now();
		
		if (stuck_detection.find(this) == stuck_detection.end()) {
			stuck_detection[this] = {dist_to_waypoint, now};
		} else {
			auto& [last_dist, last_time] = stuck_detection[this];
			auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_time).count();
			
			// If we haven't moved closer to the waypoint in 3 seconds, we're stuck
			if (elapsed >= 3 && std::abs(dist_to_waypoint - last_dist) < 1.0f) {
				std::cout << fmt::format("[WARNING] Stuck at waypoint {} - distance hasn't changed in {} seconds", 
					m_current_path_index, elapsed) << std::endl;
				
				// Skip to next waypoint or stop
				if (m_current_path_index < m_current_path.size() - 1) {
					m_current_path_index++;
					std::cout << "Skipping to next waypoint due to being stuck" << std::endl;
				} else {
					std::cout << "Stuck on final waypoint, stopping movement" << std::endl;
					StopMovement();
					return;
				}
				stuck_detection[this] = {dist_to_waypoint, now};
			} else if (elapsed >= 1) {
				// Update every second
				stuck_detection[this] = {dist_to_waypoint, now};
			}
		}
		
		if (dist_to_waypoint < 5.0f) { // Within 5 units of waypoint
			// Move to next waypoint
			m_current_path_index++;
			
			if (m_current_path_index >= m_current_path.size()) {
				// Reached the end of the path
				std::cout << "[DEBUG] Reached end of path" << std::endl;
				StopMovement();
				return;
			} else {
				// Move to next waypoint
				const auto& next_waypoint = m_current_path[m_current_path_index];
				
				// Check if the next waypoint is different enough from current position
				float dist_to_next = CalculateDistance2D(m_x, m_y, next_waypoint.x, next_waypoint.y);
				if (dist_to_next > 2.0f) {  // Only move if it's far enough away
					MoveTo(next_waypoint.x, next_waypoint.y, next_waypoint.z);
					
					if (s_debug_level >= 2) {
						std::cout << fmt::format("Reached waypoint {}, moving to waypoint {} of {}",
							m_current_path_index - 1, m_current_path_index, m_current_path.size() - 1) << std::endl;
					}
				} else {
					// Skip this waypoint, it's too close
					if (s_debug_level >= 2) {
						std::cout << fmt::format("Skipping waypoint {} (too close: {:.2f} units)",
							m_current_path_index, dist_to_next) << std::endl;
					}
					// Continue to check the next waypoint in the next update
				}
			}
		}
	}
	
	// Calculate distance to target
	float dx = m_target_x - m_x;
	float dy = m_target_y - m_y;
	float dz = m_target_z - m_z;
	float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
	
	// Check if we've reached the target
	if (distance < 2.0f) {  // Stop within 2 units of target
		m_x = m_target_x;
		m_y = m_target_y;
		m_z = m_target_z;
		
		// If following a path, check for next waypoint
		if (!m_current_path.empty() && m_current_path_index < m_current_path.size() - 1) {
			// Continue to next waypoint instead of stopping
			return;
		}
		
		StopMovement();
		return;
	}
	
	// Calculate time delta based on actual elapsed time
	static std::map<EverQuest*, std::chrono::steady_clock::time_point> last_move_times;
	if (last_move_times.find(this) == last_move_times.end()) {
		last_move_times[this] = std::chrono::steady_clock::now();
	}
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_move_times[this]);
	last_move_times[this] = now;
	
	float delta_time = elapsed.count() / 1000.0f;  // Convert to seconds
	if (delta_time > 0.1f) delta_time = 0.1f;  // Cap at 100ms to prevent large jumps
	
	// Calculate movement step using current movement speed
	float current_speed = m_move_speed;
	
	// Adjust speed when following
	if (!m_follow_target.empty()) {
		// Speed up when far, slow down when close
		if (distance > FOLLOW_FAR_DISTANCE) {
			current_speed *= FOLLOW_MAX_SPEED_MULT;
		} else if (distance < FOLLOW_CLOSE_DISTANCE) {
			current_speed *= FOLLOW_MIN_SPEED_MULT;
		} else {
			// Linear interpolation between min and max speed
			float speed_factor = (distance - FOLLOW_CLOSE_DISTANCE) / (FOLLOW_FAR_DISTANCE - FOLLOW_CLOSE_DISTANCE);
			current_speed *= FOLLOW_MIN_SPEED_MULT + (FOLLOW_MAX_SPEED_MULT - FOLLOW_MIN_SPEED_MULT) * speed_factor;
		}
	}
	
	float step = current_speed * delta_time;
	if (step > distance) {
		step = distance;
	}
	
	// Normalize direction and move
	float factor = step / distance;
	m_x += dx * factor;
	m_y += dy * factor;
	m_z += dz * factor;
	
	// Update heading
	m_heading = CalculateHeading(m_x - dx * factor, m_y - dy * factor, m_x, m_y);
	
	// Set animation based on actual movement speed
	float actual_speed = step / delta_time;  // Units per second
	if (actual_speed < WALK_SPEED_THRESHOLD) {
		m_animation = ANIM_WALK;
	} else {
		m_animation = ANIM_RUN;
	}
	
	// Fix Z position periodically to follow terrain
	static std::map<EverQuest*, std::chrono::steady_clock::time_point> last_z_fix_times;
	if (last_z_fix_times.find(this) == last_z_fix_times.end()) {
		last_z_fix_times[this] = now;
	}
	
	auto z_fix_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_z_fix_times[this]);
	// Fix Z every 500ms while moving (less frequent to reduce jerkiness)
	if (z_fix_elapsed.count() >= 500) {
		FixZ();
		last_z_fix_times[this] = now;
	}
	
	// Send position update with proper throttling per instance
	if (!m_last_position_update_time.time_since_epoch().count()) {
		m_last_position_update_time = std::chrono::steady_clock::now();
	}
	
	auto update_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_position_update_time);
	
	// Send updates at regular intervals when moving
	if (update_elapsed.count() >= POSITION_UPDATE_INTERVAL_MS) {
		SendPositionUpdate();
		m_last_position_update_time = now;
	}
}

void EverQuest::SendPositionUpdate()
{
	if (!IsFullyZonedIn()) {
		return;
	}
	
	// Keep track of last position for delta calculations (per instance, not static)
	static std::map<EverQuest*, std::tuple<float, float, float, float>> last_positions;
	
	// Initialize last position if this is the first update
	if (last_positions.find(this) == last_positions.end()) {
		last_positions[this] = std::make_tuple(m_x, m_y, m_z, m_heading);
	}
	
	auto& [last_x, last_y, last_z, last_heading] = last_positions[this];
	
	// Calculate deltas
	float delta_x = m_x - last_x;
	float delta_y = m_y - last_y;
	float delta_z = m_z - last_z;
	float delta_heading_deg = m_heading - last_heading;
	
	// Normalize delta heading to -180 to 180
	while (delta_heading_deg > 180.0f) delta_heading_deg -= 360.0f;
	while (delta_heading_deg < -180.0f) delta_heading_deg += 360.0f;
	
	// Convert heading values for the packet
	// EQ uses a different heading system where values might need adjustment
	int delta_heading_scaled = static_cast<int>(delta_heading_deg * 512.0f / 360.0f) & 0x3FF; // 10 bits
	
	// Convert heading to 12-bit value for packet
	// Server uses EQ12toFloat which divides by 4, then converts: degrees = (raw/4) * 360/512
	// This simplifies to: degrees = raw * 360 / 2048
	// So to convert degrees to raw: raw = degrees * 2048 / 360
	int heading_raw = static_cast<int>(m_heading * 2048.0f / 360.0f);
	// FloatToEQ12 does modulo 2048, so values wrap
	uint16_t heading_scaled = heading_raw % 2048;
	
	if (s_debug_level >= 2) {
		std::cout << fmt::format("[DEBUG] SendPositionUpdate: heading={:.1f}° -> scaled={} (12-bit)", 
			m_heading, heading_scaled) << std::endl;
	}
	
	// Build PlayerPositionUpdateClient_Struct
	struct {
		uint16_t spawn_id;
		uint16_t sequence;
		float y_pos;
		float delta_z;
		float delta_y;
		float delta_x;
		uint32_t anim_and_delta_heading;
		float x_pos;
		float z_pos;
		uint16_t heading_and_padding;
		uint8_t unknown[2];
	} update;
	
	memset(&update, 0, sizeof(update));
	
	// Use the entity ID we extracted from PlayerProfile
	// This is what the server expects in ClientUpdate packets
	update.spawn_id = m_my_character_id;
	update.sequence = ++m_movement_sequence;
	
	// Debug: Log what we're actually sending
	// std::cout << fmt::format("[DEBUG] SendMovementUpdate: Using spawn_id {} (m_my_character_id={})", 
	// 	update.spawn_id, m_my_character_id) << std::endl;
	
	// Validate that we have a valid character ID
	if (m_my_character_id == 0) {
	 	std::cout << "[ERROR] SendMovementUpdate called with m_my_character_id = 0! Not sending update." << std::endl;
		return;
	}
	
	// Check if we're accidentally using another player's ID
	auto it = m_entities.find(m_my_character_id);
	if (it != m_entities.end() && it->second.name != m_character) {
		std::cout << fmt::format("[WARNING] m_my_character_id {} belongs to '{}', not our character '{}'!", 
			m_my_character_id, it->second.name, m_character) << std::endl;
	}
	
	update.y_pos = m_y;
	update.x_pos = m_x;
	update.z_pos = m_z;
	// Real clients send zero deltas based on packet capture analysis
	update.delta_x = 0.0f;
	update.delta_y = 0.0f;
	update.delta_z = 0.0f;
	
	// Pack animation and delta_heading into bitfield
	// Real clients send 0 for delta_heading based on packet capture
	uint32_t anim_value = m_is_moving ? ANIM_RUN : ANIM_STAND;
	update.anim_and_delta_heading = (anim_value & 0x3FF) | (0 << 10) | (1 << 20);
	
	// Pack heading into bitfield (12 bits)
	update.heading_and_padding = heading_scaled;
	
	// Build packet
	EQ::Net::DynamicPacket p;
	p.Resize(sizeof(update) + 2);
	p.PutUInt16(0, HC_OP_ClientUpdate);
	memcpy((char*)p.Data() + 2, &update, sizeof(update));
	
	// Debug: Verify what's in the packet
	if (s_debug_level >= 1) {
		uint16_t packet_spawn_id = p.GetUInt16(2); // spawn_id is first field after opcode
		// std::cout << fmt::format("[DEBUG] ClientUpdate packet: spawn_id in struct = {}, spawn_id in packet = {}", 
		// 	update.spawn_id, packet_spawn_id) << std::endl;
	}
	
	if (s_debug_level >= 2) {
		std::cout << fmt::format("Sending position update: ({:.2f}, {:.2f}, {:.2f}) heading {:.1f}, deltas ({:.2f}, {:.2f}, {:.2f})", 
			m_x, m_y, m_z, m_heading, delta_x, delta_y, delta_z) << std::endl;
	}
	
	DumpPacket("C->S", HC_OP_ClientUpdate, p);
	m_zone_connection->QueuePacket(p);
	
	// Update last position
	last_x = m_x;
	last_y = m_y;
	last_z = m_z;
	last_heading = m_heading;
}

float EverQuest::CalculateHeading(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	
	// Calculate angle in radians
	// In EQ's coordinate system: +X is East, +Y is North (verified)
	// We want: 0° = North, 90° = East, 180° = South, 270° = West
	// Standard atan2(y,x) gives: 0° = East, 90° = North, 180° = West, 270° = South
	// So we need to use atan2(x,y) and then adjust by 90 degrees
	float angle = std::atan2(dx, dy);  // This gives 0° = North, 90° = East
	
	// Convert radians to degrees
	float degrees = angle * 180.0f / M_PI;
	
	// atan2 returns -180 to 180, convert to 0-360
	if (degrees < 0.0f) {
		degrees += 360.0f;
	}
	
	// No 180° adjustment needed - the known good client shows:
	// North ≈ 3.5°, East ≈ 137.7°, South ≈ 89.5°, West ≈ 45.8°
	// (converted from raw values 14, 1567, 1018, 521)
	
	// Debug output with more detail
	if (s_debug_level >= 2) {
		std::cout << fmt::format("[DEBUG] CalculateHeading: from ({:.1f},{:.1f}) to ({:.1f},{:.1f}), dx={:.1f}, dy={:.1f}, raw angle={:.1f}°, adjusted={:.1f}°", 
			x1, y1, x2, y2, dx, dy, angle * 180.0f / M_PI, degrees) << std::endl;
	}
	
	return degrees;
}

float EverQuest::CalculateDistance(float x1, float y1, float z1, float x2, float y2, float z2) const
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float dz = z2 - z1;
	return std::sqrt(dx * dx + dy * dy + dz * dz);
}

float EverQuest::CalculateDistance2D(float x1, float y1, float x2, float y2) const
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	return std::sqrt(dx * dx + dy * dy);
}

bool EverQuest::HasReachedDestination() const
{
	if (!m_is_moving) {
		return true;
	}
	
	// Check if we're close enough to the target (within 2 units)
	float dist = CalculateDistance2D(m_x, m_y, m_target_x, m_target_y);
	return dist < 2.0f;
}

Entity* EverQuest::FindEntityByName(const std::string& name)
{
	// Find entity by name (case-insensitive prefix match)
	// Replace spaces with underscores in search term
	std::string name_lower = name;
	std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
	std::replace(name_lower.begin(), name_lower.end(), ' ', '_');
	
	for (auto& pair : m_entities) {
		Entity& entity = pair.second;
		std::string entity_name_lower = entity.name;
		std::transform(entity_name_lower.begin(), entity_name_lower.end(), entity_name_lower.begin(), ::tolower);
		
		if (entity_name_lower.find(name_lower) == 0) {  // Starts with
			return &entity;
		}
	}
	
	return nullptr;
}

void EverQuest::ListEntities(const std::string& search) const
{
	if (!IsFullyZonedIn()) {
		std::cout << "Not in zone yet" << std::endl;
		return;
	}
	
	if (m_entities.empty()) {
		std::cout << "No entities in zone" << std::endl;
		return;
	}
	
	// Prepare search string
	std::string search_lower = search;
	if (!search.empty()) {
		std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
		std::replace(search_lower.begin(), search_lower.end(), ' ', '_');
	}
	
	if (search.empty()) {
		std::cout << fmt::format("Entities in zone ({} total):", m_entities.size()) << std::endl;
	} else {
		std::cout << fmt::format("Entities matching '{}' in zone:", search) << std::endl;
	}
	
	// Sort entities by distance from player
	std::vector<std::pair<float, const Entity*>> sorted_entities;
	for (const auto& pair : m_entities) {
		const Entity& entity = pair.second;
		
		// Filter by search term if provided
		if (!search.empty()) {
			std::string entity_name_lower = entity.name;
			std::transform(entity_name_lower.begin(), entity_name_lower.end(), entity_name_lower.begin(), ::tolower);
			if (entity_name_lower.find(search_lower) == std::string::npos) {
				continue;  // Skip entities that don't match search
			}
		}
		
		float dist = CalculateDistance(m_x, m_y, m_z, entity.x, entity.y, entity.z);
		sorted_entities.push_back({dist, &entity});
	}
	
	std::sort(sorted_entities.begin(), sorted_entities.end(), 
		[](const auto& a, const auto& b) { return a.first < b.first; });
	
	// Check if any entities matched
	if (sorted_entities.empty() && !search.empty()) {
		std::cout << fmt::format("  No entities found matching '{}'", search) << std::endl;
		return;
	}
	
	// Display up to 20 nearest entities
	int count = 0;
	for (const auto& [dist, entity] : sorted_entities) {
		if (count++ >= 20) {
			std::cout << "  ... and more" << std::endl;
			break;
		}
		
		std::cout << fmt::format("  {} (ID: {}) - Level {} {} - {:.1f} units away at ({:.0f}, {:.0f}, {:.0f})",
			entity->name, entity->spawn_id, entity->level,
			entity->class_id == 0 ? "NPC" : fmt::format("Class {}", entity->class_id),
			dist, entity->x, entity->y, entity->z) << std::endl;
			
		if (entity->hp_percent < 100) {
			std::cout << fmt::format("    HP: {}%", entity->hp_percent) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessWearChange(const EQ::Net::Packet &p)
{
	// WearChange indicates equipment changes on entities
	// Titanium Format: spawn_id (2 bytes), material (2 bytes), color (4 bytes), wear_slot_id (1 byte)
	// Total: 9 bytes + 2 byte opcode = 11 bytes
	
	if (p.Length() != 11) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("WearChange packet wrong size: {} bytes (expected 11)", p.Length()) << std::endl;
		}
		return;
	}
	
	// Skip opcode (2 bytes), then parse struct
	uint16_t spawn_id = p.GetUInt16(2);      // offset 0 in struct, 2 in packet
	uint16_t material = p.GetUInt16(4);      // offset 2 in struct, 4 in packet
	// Skip color (4 bytes) at offset 4-7 in struct, 6-9 in packet
	uint8_t wear_slot = p.GetUInt8(10);      // offset 8 in struct, 10 in packet
	
	if (s_debug_level >= 2) {
		auto it = m_entities.find(spawn_id);
		std::string name = (it != m_entities.end()) ? it->second.name : "Unknown";
		std::cout << fmt::format("Equipment change for {} (ID: {}): slot {} material {}", 
			name, spawn_id, wear_slot, material) << std::endl;
	}
}

void EverQuest::ZoneProcessMoveDoor(const EQ::Net::Packet &p)
{
	// MoveDoor indicates door animations (opening/closing)
	// Format: door_id (1 byte), action (1 byte)
	
	if (p.Length() < 4) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("MoveDoor packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	uint8_t door_id = p.GetUInt8(2);
	uint8_t action = p.GetUInt8(3);
	
	if (s_debug_level >= 2) {
		std::cout << fmt::format("Door {} action: {}", door_id, action) << std::endl;
	}
}

void EverQuest::ZoneProcessCompletedTasks(const EQ::Net::Packet &p)
{
	// CompletedTasks sends the list of completed tasks
	// This is informational - we can track it if needed
	
	if (s_debug_level >= 2) {
		std::cout << fmt::format("Received completed tasks list ({} bytes)", p.Length()) << std::endl;
	}
}

void EverQuest::ZoneProcessDzCompass(const EQ::Net::Packet &p)
{
	// DzCompass updates the expedition/DZ compass direction
	// Format: heading (4 bytes float), x (4 bytes float), y (4 bytes float), z (4 bytes float)
	
	if (s_debug_level >= 2) {
		if (p.Length() >= 18) {
			float heading = p.GetFloat(2);
			float x = p.GetFloat(6);
			float y = p.GetFloat(10);
			float z = p.GetFloat(14);
			std::cout << fmt::format("DZ compass update: heading {:.1f} to ({:.2f}, {:.2f}, {:.2f})", 
				heading, x, y, z) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessDzExpeditionLockoutTimers(const EQ::Net::Packet &p)
{
	// DzExpeditionLockoutTimers sends lockout timer information
	// This is informational for expeditions/DZs
	
	if (s_debug_level >= 2) {
		std::cout << fmt::format("Received DZ expedition lockout timers ({} bytes)", p.Length()) << std::endl;
	}
}

void EverQuest::ZoneProcessBeginCast(const EQ::Net::Packet &p)
{
	// BeginCast indicates an entity is starting to cast a spell
	// Format: spawn_id (2 bytes), spell_id (2 bytes), cast_time (4 bytes)
	
	if (p.Length() < 10) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("BeginCast packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	uint16_t spawn_id = p.GetUInt16(2);
	uint16_t spell_id = p.GetUInt16(4);
	uint32_t cast_time = p.GetUInt32(6);
	
	if (s_debug_level >= 2) {
		auto it = m_entities.find(spawn_id);
		std::string name = (it != m_entities.end()) ? it->second.name : "Unknown";
		std::cout << fmt::format("{} (ID: {}) begins casting spell {} ({}ms)", 
			name, spawn_id, spell_id, cast_time) << std::endl;
	}
}

void EverQuest::ZoneProcessManaChange(const EQ::Net::Packet &p)
{
	// ManaChange updates an entity's mana
	// Assuming format: spawn_id (2 bytes), current_mana (2 bytes), max_mana (2 bytes)
	// Total: 6 bytes + 2 byte opcode = 8 bytes minimum
	
	if (p.Length() < 8) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("ManaChange packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	// Skip opcode (2 bytes), then parse struct
	uint16_t spawn_id = p.GetUInt16(2);   // offset 0 in struct, 2 in packet
	uint16_t cur_mana = p.GetUInt16(4);   // offset 2 in struct, 4 in packet
	uint16_t max_mana = p.GetUInt16(6);   // offset 4 in struct, 6 in packet
	
	// Update entity if we're tracking it
	auto it = m_entities.find(spawn_id);
	if (it != m_entities.end()) {
		it->second.cur_mana = cur_mana;
		it->second.max_mana = max_mana;
	}
	
	if (s_debug_level >= 2) {
		if (spawn_id == m_my_spawn_id) {
			std::cout << fmt::format("Player mana: {}/{}", cur_mana, max_mana) << std::endl;
		} else {
			std::string name = (it != m_entities.end()) ? it->second.name : "Unknown";
			std::cout << fmt::format("{} (ID: {}) mana: {}/{}", 
				name, spawn_id, cur_mana, max_mana) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessFormattedMessage(const EQ::Net::Packet &p)
{
	// FormattedMessage is used for various system messages
	// Titanium Format: unknown0 (4 bytes), string_id (4 bytes), type (4 bytes), message (variable)
	// Total header: 12 bytes + 2 byte opcode = 14 bytes minimum
	
	if (p.Length() < 14) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("FormattedMessage packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	// Skip opcode (2 bytes) and header (12 bytes), then get message string
	uint32_t unknown0 = p.GetUInt32(2);
	uint32_t string_id = p.GetUInt32(6);
	uint32_t type = p.GetUInt32(10);
	
	if (s_debug_level >= 2) {
		std::cout << fmt::format("[FormattedMessage] Packet length: {}, unknown0={}, string_id={}, type={}", 
			p.Length(), unknown0, string_id, type) << std::endl;
		
		// Dump the message portion
		if (p.Length() > 14) {
			const uint8_t* data = static_cast<const uint8_t*>(p.Data()) + 14;
			size_t msg_len = p.Length() - 14;
			std::cout << "Message bytes: ";
			for (size_t i = 0; i < std::min(msg_len, size_t(32)); ++i) {
				std::cout << fmt::format("{:02x} ", data[i]);
			}
			std::cout << std::endl;
		}
	}
	
	// Message starts at offset 14 (2 byte opcode + 12 byte header)
	if (p.Length() > 14) {
		const char* message = reinterpret_cast<const char*>(static_cast<const uint8_t*>(p.Data()) + 14);
		size_t max_len = p.Length() - 14;
		
		// Ensure null termination
		std::string msg(message, strnlen(message, max_len));
		
		// Always show FormattedMessage content for debugging
		std::cout << fmt::format("[FormattedMessage] string_id={}, type={}, message='{}'", 
			string_id, type, msg) << std::endl;
	} else {
		// No message content, just the header
		std::cout << fmt::format("[FormattedMessage] string_id={}, type={}, no message content", 
			string_id, type) << std::endl;
	}
}

void EverQuest::ZoneProcessPlayerStateAdd(const EQ::Net::Packet &p)
{
	// PlayerStateAdd adds a state/buff to a player
	// This could be buffs, debuffs, or other state changes
	
	if (p.Length() < 4) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("PlayerStateAdd packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	// The exact structure depends on the state being added
	// For now, just log that we received it
	if (s_debug_level >= 2) {
		std::cout << fmt::format("PlayerStateAdd received, size: {} bytes", p.Length()) << std::endl;
		
		// Dump first few bytes for analysis
		if (s_debug_level >= 3 && p.Length() >= 6) {
			uint16_t value1 = p.GetUInt16(2);
			uint16_t value2 = p.GetUInt16(4);
			std::cout << fmt::format("  Values: {:#06x}, {:#06x}", value1, value2) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessDeath(const EQ::Net::Packet &p)
{
	// Death packet indicates an entity has died
	// Format typically includes: spawn_id of victim, spawn_id of killer, damage, spell_id
	
	if (p.Length() < 10) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Death packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	// Parse death information
	uint16_t victim_id = p.GetUInt16(2);    // Who died
	uint16_t killer_id = p.GetUInt16(4);    // Who killed them
	uint32_t damage = p.GetUInt32(6);       // Final damage amount
	uint16_t spell_id = 0;
	
	if (p.Length() >= 12) {
		spell_id = p.GetUInt16(10);         // Spell that killed (if any)
	}
	
	// Get entity names if we have them
	std::string victim_name = "Unknown";
	std::string killer_name = "Unknown";
	
	auto victim_it = m_entities.find(victim_id);
	if (victim_it != m_entities.end()) {
		victim_name = victim_it->second.name;
		// Mark entity as dead (0 HP)
		victim_it->second.hp_percent = 0;
	}
	
	auto killer_it = m_entities.find(killer_id);
	if (killer_it != m_entities.end()) {
		killer_name = killer_it->second.name;
	}
	
	if (s_debug_level >= 1) {
		if (spell_id > 0) {
			std::cout << fmt::format("{} ({}) was killed by {} ({}) for {} damage (spell: {})",
				victim_name, victim_id, killer_name, killer_id, damage, spell_id) << std::endl;
		} else {
			std::cout << fmt::format("{} ({}) was killed by {} ({}) for {} damage",
				victim_name, victim_id, killer_name, killer_id, damage) << std::endl;
		}
	}
	
	// If we died, log it prominently
	if (victim_id == m_my_spawn_id) {
		std::cout << "YOU HAVE BEEN SLAIN!" << std::endl;
	}
}

void EverQuest::ZoneProcessPlayerStateRemove(const EQ::Net::Packet &p)
{
	// PlayerStateRemove removes a state/buff from a player
	// This is the counterpart to PlayerStateAdd
	
	if (p.Length() < 4) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("PlayerStateRemove packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	// The exact structure depends on the state being removed
	// For now, just log that we received it
	if (s_debug_level >= 2) {
		std::cout << fmt::format("PlayerStateRemove received, size: {} bytes", p.Length()) << std::endl;
		
		// Dump first few bytes for analysis
		if (s_debug_level >= 3 && p.Length() >= 6) {
			uint16_t value1 = p.GetUInt16(2);
			uint16_t value2 = p.GetUInt16(4);
			std::cout << fmt::format("  Values: {:#06x}, {:#06x}", value1, value2) << std::endl;
		}
	}
}

void EverQuest::ZoneProcessStamina(const EQ::Net::Packet &p)
{
	// Stamina (endurance) update packet
	// Format: spawn_id (2 bytes), current_stamina (4 bytes), max_stamina (4 bytes)
	
	if (p.Length() < 10) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("Stamina packet too small: {} bytes", p.Length()) << std::endl;
		}
		return;
	}
	
	uint16_t spawn_id = p.GetUInt16(2);
	uint32_t current_stamina = p.GetUInt32(4);
	uint32_t max_stamina = p.GetUInt32(8);
	
	if (s_debug_level >= 2) {
		std::cout << fmt::format("Stamina update: spawn_id={}, current={}, max={}", 
			spawn_id, current_stamina, max_stamina) << std::endl;
	}
	
	// Store stamina values if this is for our character
	if (spawn_id == m_my_character_id) {
		// TODO: Store stamina values as member variables if needed
	}
}

// Pathfinding implementation
void EverQuest::LoadPathfinder(const std::string& zone_name)
{
	if (zone_name.empty()) {
		std::cout << "[DEBUG] LoadPathfinder: Zone name is empty, skipping" << std::endl;
		return;
	}
	
	std::cout << fmt::format("[DEBUG] LoadPathfinder: Loading pathfinder for zone '{}'", zone_name) << std::endl;
	
	// Release previous pathfinder if any
	m_pathfinder.reset();
	m_current_path.clear();
	m_current_path_index = 0;
	
	try {
		// Use the IPathfinder factory method to load the navigation mesh
		m_pathfinder.reset(IPathfinder::Load(zone_name, m_navmesh_path));
		
		if (m_pathfinder) {
			// Check if it's actually a PathfinderNavmesh vs PathfinderNull
			// This is a bit of a hack but helps us understand what type we got
			PathfinderOptions test_opts;
			bool partial = false, stuck = false;
			auto test_path = m_pathfinder->FindPath(
				glm::vec3(0, 0, 0), glm::vec3(1, 1, 1), partial, stuck, test_opts);
			
			if (s_debug_level >= 1) {
				std::cout << fmt::format("[DEBUG] Loaded pathfinder for zone: {} (type: {})", 
					zone_name, test_path.empty() ? "NavMesh" : "Null") << std::endl;
			}
		} else {
			if (s_debug_level >= 1) {
				std::cout << fmt::format("[DEBUG] No navigation mesh available for zone: {}", zone_name) << std::endl;
			}
		}
	} catch (const std::exception& e) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("[DEBUG] Failed to load navigation mesh for {}: {}", zone_name, e.what()) << std::endl;
		}
	}
}

bool EverQuest::FindPath(float start_x, float start_y, float start_z, float end_x, float end_y, float end_z)
{
	std::cout << fmt::format("[DEBUG] FindPath called: from ({:.2f},{:.2f},{:.2f}) to ({:.2f},{:.2f},{:.2f})",
		start_x, start_y, start_z, end_x, end_y, end_z) << std::endl;
	
	if (!m_pathfinder) {
		std::cout << "[DEBUG] FindPath: No pathfinder object available (m_pathfinder is null)" << std::endl;
		return false;
	}
	
	// Clear previous path
	m_current_path.clear();
	m_current_path_index = 0;
	
	// Set up pathfinding options
	PathfinderOptions opts;
	opts.smooth_path = true;
	opts.step_size = 10.0f; // Distance between waypoints
	opts.offset = 5.0f; // Height offset for character
	
	// Find the path
	bool partial = false;
	bool stuck = false;
	std::cout << "[DEBUG] FindPath: Calling m_pathfinder->FindPath()..." << std::endl;
	IPathfinder::IPath path = m_pathfinder->FindPath(
		glm::vec3(start_x, start_y, start_z),
		glm::vec3(end_x, end_y, end_z),
		partial,
		stuck,
		opts
	);
	std::cout << fmt::format("[DEBUG] FindPath: Result - path size: {}, partial: {}, stuck: {}", 
		path.size(), partial, stuck) << std::endl;
	
	if (path.empty()) {
		if (s_debug_level >= 1) {
			std::cout << fmt::format("No path found from ({:.1f}, {:.1f}, {:.1f}) to ({:.1f}, {:.1f}, {:.1f})",
				start_x, start_y, start_z, end_x, end_y, end_z) << std::endl;
			if (partial) {
				std::cout << "  (Partial path available)" << std::endl;
			}
			if (stuck) {
				std::cout << "  (Path leads back to start - stuck)" << std::endl;
			}
		}
		return false;
	}
	
	// Convert path nodes to vec3 waypoints
	for (const auto& node : path) {
		// Skip teleport nodes for now
		if (!node.teleport) {
			m_current_path.push_back(node.pos);
		}
	}
	
	if (s_debug_level >= 1) {
		std::cout << fmt::format("Found path with {} waypoints", m_current_path.size()) << std::endl;
		if (s_debug_level >= 2) {
			for (size_t i = 0; i < m_current_path.size() && i < 5; i++) {
				const auto& pos = m_current_path[i];
				std::cout << fmt::format("  Waypoint {}: ({:.1f}, {:.1f}, {:.1f})", 
					i, pos.x, pos.y, pos.z) << std::endl;
			}
			if (m_current_path.size() > 5) {
				std::cout << "  ..." << std::endl;
			}
		}
	}
	
	return true;
}

void EverQuest::MoveToWithPath(float x, float y, float z)
{
	if (!IsFullyZonedIn()) {
		std::cout << "Error: Not in zone yet" << std::endl;
		return;
	}
	
	// If pathfinding is disabled or not available, use direct movement
	if (!m_use_pathfinding || !m_pathfinder) {
		MoveTo(x, y, z);
		return;
	}
	
	// Find path from current position to target
	if (FindPath(m_x, m_y, m_z, x, y, z)) {
		// Face the final destination immediately
		float new_heading = CalculateHeading(m_x, m_y, x, y);
		if (std::abs(new_heading - m_heading) > 0.1f) {
			m_heading = new_heading;
			SendPositionUpdate();
		}
		
		// Start following the path
		m_current_path_index = 0;
		FollowPath();
	} else {
		// Fallback to direct movement if pathfinding fails
		if (s_debug_level >= 1) {
			std::cout << "Pathfinding failed, using direct movement" << std::endl;
		}
		MoveTo(x, y, z);
	}
}

void EverQuest::FollowPath()
{
	if (m_current_path.empty() || m_current_path_index >= m_current_path.size()) {
		StopMovement();
		return;
	}
	
	// Skip waypoints that are too close to current position (within 1 unit)
	while (m_current_path_index < m_current_path.size()) {
		const auto& waypoint = m_current_path[m_current_path_index];
		float dist = CalculateDistance2D(m_x, m_y, waypoint.x, waypoint.y);
		
		if (dist > 1.0f) {
			// This waypoint is far enough, move to it
			MoveTo(waypoint.x, waypoint.y, waypoint.z);
			break;
		} else {
			// Skip this waypoint, it's too close
			if (s_debug_level >= 2) {
				std::cout << fmt::format("[DEBUG] Skipping waypoint {} at ({:.1f},{:.1f},{:.1f}) - too close (dist={:.1f})",
					m_current_path_index, waypoint.x, waypoint.y, waypoint.z, dist) << std::endl;
			}
			m_current_path_index++;
		}
	}
	
	// If we've gone through all waypoints, stop
	if (m_current_path_index >= m_current_path.size()) {
		StopMovement();
		return;
	}
	
	// The UpdateMovement function will handle checking when we reach waypoints
}

void EverQuest::LoadZoneMap(const std::string& zone_name)
{
	if (zone_name.empty()) {
		std::cout << "[DEBUG] LoadZoneMap: Zone name is empty, skipping" << std::endl;
		return;
	}
	
	std::cout << fmt::format("[DEBUG] LoadZoneMap: Loading map for zone '{}'", zone_name) << std::endl;
	
	// Release previous map if any
	m_zone_map.reset();
	
	// Use custom maps path if set, otherwise try common locations
	std::string maps_path = m_maps_path;
	if (maps_path.empty()) {
		// Try to use the same path as navmesh
		if (!m_navmesh_path.empty()) {
			// Navigate from nav directory to maps directory
			size_t nav_pos = m_navmesh_path.find("/nav");
			if (nav_pos != std::string::npos) {
				maps_path = m_navmesh_path.substr(0, nav_pos);
			}
		}
		
		// Fallback to default server location
		if (maps_path.empty()) {
			maps_path = "/home/eqemu/server/maps";
		}
	}
	
	std::cout << fmt::format("[DEBUG] LoadZoneMap: Using maps path: {}", maps_path) << std::endl;
	
	m_zone_map.reset(HCMap::LoadMapFile(zone_name, maps_path));
	
	if (!m_zone_map) {
		std::cout << fmt::format("[WARNING] Failed to load map for zone: {}", zone_name) << std::endl;
	}
}

float EverQuest::GetBestZ(float x, float y, float z)
{
	if (!m_zone_map) {
		return z; // Return current Z if no map loaded
	}
	
	glm::vec3 pos(x, y, z);
	glm::vec3 result;
	float best_z = m_zone_map->FindBestZ(pos, &result);
	
	if (best_z == BEST_Z_INVALID) {
		return z; // Return current Z if no valid ground found
	}
	
	return best_z;
}

void EverQuest::FixZ()
{
	if (!m_zone_map) {
		return; // No map loaded
	}
	
	// Get the best Z for our current position
	float new_z = GetBestZ(m_x, m_y, m_z);
	
	// Only update if the difference is significant but not too large
	float z_diff = std::abs(new_z - m_z);
	if (z_diff > 1.0f && z_diff < 20.0f) {
		// Smooth the Z adjustment to reduce jerkiness
		// Move towards the target Z by a fraction of the difference
		float adjustment = (new_z - m_z) * 0.3f; // Move 30% of the way
		
		if (s_debug_level >= 2) {
			std::cout << fmt::format("[DEBUG] FixZ: Smoothly adjusting Z from {:.2f} towards {:.2f} (adjustment: {:.2f})", 
				m_z, new_z, adjustment) << std::endl;
		}
		m_z += adjustment;
	}
}
 
