#ifndef CC_SERVERCONNECTION_H
#define CC_SERVERCONNECTION_H
#include "Input.h"
#include "Vectors.h"
/* Represents a connection to either a singleplayer or multiplayer server.
   Copyright 2014-2017 ClassicalSharp | Licensed under BSD-3
*/

enum OPCODE_ {
	OPCODE_HANDSHAKE,             OPCODE_PING,
	OPCODE_LEVEL_BEGIN, OPCODE_LEVEL_DATA, OPCODE_LEVEL_END,
	OPCODE_SET_BLOCK_CLIENT,      OPCODE_SET_BLOCK,
	OPCODE_ADD_ENTITY,            OPCODE_ENTITY_TELEPORT,
	OPCODE_RELPOS_AND_ORI_UPDATE, OPCODE_RELPOS_UPDATE, 
	OPCODE_ORI_UPDATE,            OPCODE_REMOVE_ENTITY,
	OPCODE_MESSAGE,               OPCODE_KICK,
	OPCODE_SET_PERMISSION,

	/* CPE packets */
	OPCODE_EXT_INFO,            OPCODE_EXT_ENTRY,
	OPCODE_SET_REACH,           OPCODE_CUSTOM_BLOCK_LEVEL,
	OPCODE_HOLD_THIS,           OPCODE_SET_TEXT_HOTKEY,
	OPCODE_EXT_ADD_PLAYER_NAME,    OPCODE_EXT_ADD_ENTITY,
	OPCODE_EXT_REMOVE_PLAYER_NAME, OPCODE_ENV_SET_COLOR,
	OPCODE_MAKE_SELECTION,         OPCODE_REMOVE_SELECTION,
	OPCODE_SET_BLOCK_PERMISSION,   OPCODE_SET_MODEL,
	OPCODE_ENV_SET_MAP_APPEARANCE, OPCODE_ENV_SET_WEATHER,
	OPCODE_HACK_CONTROL,        OPCODE_EXT_ADD_ENTITY2,
	OPCODE_PLAYER_CLICK,        OPCODE_DEFINE_BLOCK,
	OPCODE_UNDEFINE_BLOCK,      OPCODE_DEFINE_BLOCK_EXT,
	OPCODE_BULK_BLOCK_UPDATE,   OPCODE_SET_TEXT_COLOR,
	OPCODE_ENV_SET_MAP_URL,     OPCODE_ENV_SET_MAP_PROPERTY,
	OPCODE_SET_ENTITY_PROPERTY, OPCODE_TWO_WAY_PING,
	OPCODE_SET_INVENTORY_ORDER,

	OPCODE_COUNT
};

struct PickedPos;
struct Stream;
struct IGameComponent;
struct ScheduledTask;
extern struct IGameComponent ServerConnection_Component;

int PingList_NextPingData(void);
void PingList_Update(int data);
int PingList_AveragePingMs(void);

/* Whether the player is connected to singleplayer/loopback server. */
extern bool ServerConnection_IsSinglePlayer;
/* Whether the player has been disconnected from the server. */
extern bool ServerConnection_Disconnected;
/* The current name of the server. (Shows as first line when loading) */
extern String ServerConnection_ServerName;
/* The current MOTD of the server. (Shows as second line when loading) */
extern String ServerConnection_ServerMOTD;
/* The software name the client identifies itself as being to the server. */
/* By default this is the same as PROGRAM_APP_NAME */
extern String ServerConnection_AppName;

struct ServerConnectionFuncs {
	/* Begins connecting to the server. */
	/* NOTE: Usually asynchronous, but not always. */
	void (*BeginConnect)(void);
	/* Ticks state of the server. */
	void (*Tick)(struct ScheduledTask* task);
	/* Sends a block update to the server. */
	void (*SendBlock)(int x, int y, int z, BlockID old, BlockID now);
	/* Sends a chat message to the server. */
	void (*SendChat)(const String* text);
	/* Sends a position update to the server. */
	void (*SendPosition)(Vector3 pos, float rotY, float headX);
	/* Sends a PlayerClick packet to the server. */
	void (*SendPlayerClick)(MouseButton button, bool isDown, EntityID targetId, struct PickedPos* pos);
};

/* Currently active connection to a server. */
extern struct ServerConnectionFuncs ServerConnection;
extern uint8_t* ServerConnection_WriteBuffer;

/* Whether the server supports separate tab list from entities in world. */
extern bool ServerConnection_SupportsExtPlayerList;
/* Whether the server supports packet with detailed info on mouse clicks. */
extern bool ServerConnection_SupportsPlayerClick;
/* Whether the server supports combining multiple chat packets into one. */
extern bool ServerConnection_SupportsPartialMessages;
/* Whether the server supports all of code page 437, not just ASCII. */
extern bool ServerConnection_SupportsFullCP437;

void ServerConnection_RetrieveTexturePack(const String* url);
void ServerConnection_DownloadTexturePack(const String* url);

typedef void (*Net_Handler)(uint8_t* data);
extern uint16_t Net_PacketSizes[OPCODE_COUNT];
extern Net_Handler Net_Handlers[OPCODE_COUNT];
void Net_Set(uint8_t opcode, Net_Handler handler, int size);
void Net_SendPacket(void);
#endif
