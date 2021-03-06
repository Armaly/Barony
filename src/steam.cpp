/*-------------------------------------------------------------------------------

	BARONY
	File: steam.cpp
	Desc: various callback functions for steam

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "main.hpp"
#include "game.hpp"
#include "stat.hpp"
#include "net.hpp"
#include "menu.hpp"
#include "monster.hpp"
#include "scores.hpp"
#include "entity.hpp"
#include "items.hpp"
#include "interface/interface.hpp"
#include <SDL_thread.h>
#ifdef STEAMWORKS
#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>
#include "steam.hpp"
#endif

#ifdef STEAMWORKS

int numSteamLobbies = 0;
int selectedSteamLobby = 0;
char lobbyText[MAX_STEAM_LOBBIES][48];
void* lobbyIDs[MAX_STEAM_LOBBIES] = { NULL };
int lobbyPlayers[MAX_STEAM_LOBBIES] = { 0 };

void* steamIDRemote[MAXPLAYERS] = {NULL, NULL, NULL, NULL};

char currentLobbyName[32] = { 0 };
Uint32 currentSvFlags = 0;
#ifdef STEAMWORKS
ELobbyType currentLobbyType = k_ELobbyTypePrivate;
#endif
bool stillConnectingToLobby = false;

bool serverLoadingSaveGame = false; // determines whether lobbyToConnectTo is loading a savegame or not
void* currentLobby = NULL; // CSteamID to the current game lobby
void* lobbyToConnectTo = NULL; // CSteamID of the game lobby that user has been invited to
void* steamIDGameServer = NULL; // CSteamID to the current game server
uint32_t steamServerIP = 0; // ipv4 address for the current game server
uint16_t steamServerPort = 0; // port number for the current game server
char pchCmdLine[1024] = { 0 }; // for game join requests

// menu stuff
bool connectingToLobby = false, connectingToLobbyWindow = false;
bool requestingLobbies = false;

const std::string CSteamLeaderboards::leaderboardNames[CSteamLeaderboards::k_numLeaderboards] =
{
	"None",

	"Fastest Time (Normal)",
	"Highest Score (Normal)",

	"Fastest Time (Multiplayer)",
	"Highest Score (Multiplayer)",

	"Fastest Time (Hell Route)",
	"Highest Score (Hell Route)",
	
	"Fastest Time (Hardcore)",
	"Highest Score (Hardcore)",

	"Fastest Time (Classic)",
	"Highest Score (Classic)",

	"Fastest Time (Classic Hardcore)",
	"Highest Score (Classic Hardcore)",

	"Fastest Time (Multiplayer Classic)",
	"Highest Score (Multiplayer Classic)",

	"Fastest Time (Multiplayer Hell Route)",
	"Highest Score (Multiplayer Hell Route)",

	"Fastest Time (Normal - Monsters Only)",
	"Highest Score (Normal - Monsters Only)",

	"Fastest Time (Multiplayer - Monsters Only)",
	"Highest Score (Multiplayer - Monsters Only)",

	"Fastest Time (Hell Route - Monsters Only)",
	"Highest Score (Hell Route - Monsters Only)",

	"Fastest Time (Hardcore - Monsters Only)",
	"Highest Score (Hardcore - Monsters Only)",

	"Fastest Time (Classic - Monsters Only)",
	"Highest Score (Classic - Monsters Only)",

	"Fastest Time (Classic Hardcore - Monsters Only)",
	"Highest Score (Classic Hardcore - Monsters Only)",

	"Fastest Time (Multiplayer Classic - Monsters Only)",
	"Highest Score (Multiplayer Classic - Monsters Only)",

	"Fastest Time (Multiplayer Hell Route - Monsters Only)",
	"Highest Score (Multiplayer Hell Route - Monsters Only)"
};
#endif






/* ***** BEGIN UTTER BODGE ***** */

//These are all an utter bodge. They really, really should not exist, but potato.


#ifdef STEAMWORKS
//TODO: Unused?
void (*cpp_SteamServerClientWrapper_GameServerPingOnServerResponded)(void* steamID);
void (*cpp_SteamServerClientWrapper_OnLobbyDataUpdate)(void* pCallback);
void (*cpp_SteamServerClientWrapper_OnSteamShutdown)(void* callback);
void (*cpp_SteamServerClientWrapper_OnIPCFailure)(void* failure);
void (*cpp_SteamServerClientWrapper_OnP2PSessionRequest)(void* pCallback);
void (*cpp_SteamServerClientWrapper_OnP2PSessionConnectFail)(void* pCallback);
void (*cpp_SteamServerClientWrapper_OnWorkshopItemInstalled)(void* pParam);
void (*cpp_SteamServerClientWrapper_OnGameWebCallback)(void* callback);
//void (*cpp_SteamServerClientWrapper_OnGameOverlayActivated)(void *callback);
void (*cpp_SteamServerClientWrapper_OnSteamServerConnectFailure)(void* callback);
void (*cpp_SteamServerClientWrapper_OnSteamServersDisconnected)(void* callback);
void (*cpp_SteamServerClientWrapper_OnSteamServersConnected)(void* callback);
void (*cpp_SteamServerClientWrapper_OnAvatarImageLoaded)(void* pCallback);
void (*cpp_SteamServerClientWrapper_OnGameJoinRequested)(void* pCallback);
void (*cpp_SteamServerClientWrapper_OnLobbyGameCreated)(void* pCallback);
void (*cpp_SteamServerClientWrapper_OnLobbyEntered)(void* pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyEnter_t.
void (*cpp_SteamServerClientWrapper_OnLobbyMatchListCallback)(void* pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyMatchList_t.
void (*cpp_SteamServerClientWrapper_OnLobbyCreated)(void* pCallback, bool bIOFailure); //Where pCallback is a pointer to type LobbyCreated_t.

void (*cpp_SteamServerWrapper_OnValidateAuthTicketResponse)(void* pResponse);
void (*cpp_SteamServerWrapper_OnPolicyResponse)(void* pPolicyResponse);
void (*cpp_SteamServerWrapper_OnP2PSessionConnectFail)(void* pCallback);
void (*cpp_SteamServerWrapper_OnP2PSessionRequest)(void* p_Callback);
void (*cpp_SteamServerWrapper_OnSteamServersConnectFailure)(void* pConnectFailure);
void (*cpp_SteamServerWrapper_OnSteamServersDisconnected)(void* pLoggedOff);
void (*cpp_SteamServerWrapper_OnSteamServersConnected)(void* pLogonSuccess);
void (*cpp_SteamServerClientWrapper_OnRequestEncryptedAppTicket)(void* pEncryptedAppTicketResponse, bool bIOFailure); //Where pEncryptedAppTicketResponse is of type



class SteamServerWrapper
{
public:
	SteamServerWrapper()
		:
		m_CallbackSteamServersConnected(this, &SteamServerWrapper::OnSteamServersConnected),
		m_CallbackSteamServersDisconnected(this, &SteamServerWrapper::OnSteamServersDisconnected),
		m_CallbackSteamServersConnectFailure( this, &SteamServerWrapper::OnSteamServersConnectFailure ),
		m_CallbackPolicyResponse(this, &SteamServerWrapper::OnPolicyResponse),
		m_CallbackGSAuthTicketResponse(this, &SteamServerWrapper::OnValidateAuthTicketResponse),
		m_CallbackP2PSessionRequest(this, &SteamServerWrapper::OnP2PSessionRequest),
		m_CallbackP2PSessionConnectFail(this, &SteamServerWrapper::OnP2PSessionConnectFail)
	{
		cpp_SteamServerWrapper_OnSteamServersConnected = nullptr;
		cpp_SteamServerWrapper_OnSteamServersDisconnected = nullptr;
		cpp_SteamServerWrapper_OnSteamServersConnectFailure = nullptr;
		cpp_SteamServerWrapper_OnP2PSessionRequest = nullptr;
		cpp_SteamServerWrapper_OnP2PSessionConnectFail = nullptr;
		cpp_SteamServerWrapper_OnPolicyResponse = nullptr;
		cpp_SteamServerWrapper_OnValidateAuthTicketResponse = nullptr;
	}

	STEAM_GAMESERVER_CALLBACK(SteamServerWrapper, OnSteamServersConnected, SteamServersConnected_t, m_CallbackSteamServersConnected);

	STEAM_GAMESERVER_CALLBACK(SteamServerWrapper, OnSteamServersDisconnected, SteamServersDisconnected_t, m_CallbackSteamServersDisconnected);

	STEAM_GAMESERVER_CALLBACK(SteamServerWrapper, OnSteamServersConnectFailure, SteamServerConnectFailure_t, m_CallbackSteamServersConnectFailure);

	STEAM_GAMESERVER_CALLBACK(SteamServerWrapper, OnPolicyResponse, GSPolicyResponse_t, m_CallbackPolicyResponse);

	STEAM_GAMESERVER_CALLBACK(SteamServerWrapper, OnValidateAuthTicketResponse, ValidateAuthTicketResponse_t, m_CallbackGSAuthTicketResponse);

	STEAM_GAMESERVER_CALLBACK(SteamServerWrapper, OnP2PSessionRequest, P2PSessionRequest_t, m_CallbackP2PSessionRequest);

	STEAM_GAMESERVER_CALLBACK(SteamServerWrapper, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_CallbackP2PSessionConnectFail);
}* steam_server_wrapper;

void SteamServerWrapper::OnSteamServersConnected(SteamServersConnected_t* pLogonSuccess)
{
	if (cpp_SteamServerWrapper_OnSteamServersConnected)
	{
		(*cpp_SteamServerWrapper_OnSteamServersConnected)(pLogonSuccess);
	}
}

void SteamServerWrapper::OnSteamServersDisconnected(SteamServersDisconnected_t* pLoggedOff)
{
	if (cpp_SteamServerWrapper_OnSteamServersDisconnected)
	{
		(*cpp_SteamServerWrapper_OnSteamServersDisconnected)(pLoggedOff);
	}
}

void SteamServerWrapper::OnSteamServersConnectFailure(SteamServerConnectFailure_t* pConnectFailure)
{
	if (cpp_SteamServerWrapper_OnSteamServersConnectFailure)
	{
		(*cpp_SteamServerWrapper_OnSteamServersConnectFailure)(pConnectFailure);
	}
}

void SteamServerWrapper::OnPolicyResponse(GSPolicyResponse_t* pPolicyResponse)
{
	if (cpp_SteamServerWrapper_OnPolicyResponse)
	{
		(*cpp_SteamServerWrapper_OnPolicyResponse)(pPolicyResponse);
	}
}

void SteamServerWrapper::OnValidateAuthTicketResponse(ValidateAuthTicketResponse_t* pResponse)
{
	if (cpp_SteamServerWrapper_OnValidateAuthTicketResponse)
	{
		(*cpp_SteamServerWrapper_OnValidateAuthTicketResponse)(pResponse);
	}
}

void SteamServerWrapper::OnP2PSessionRequest(P2PSessionRequest_t* pCallback)
{
	if (cpp_SteamServerWrapper_OnP2PSessionRequest)
	{
		(*cpp_SteamServerWrapper_OnP2PSessionRequest)(pCallback);
	}
}

void SteamServerWrapper::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pCallback)
{
	if (cpp_SteamServerWrapper_OnP2PSessionConnectFail)
	{
		(*cpp_SteamServerWrapper_OnP2PSessionConnectFail)(pCallback);
	}
}

void cpp_SteamServerWrapper_Instantiate()
{
	steam_server_wrapper = new SteamServerWrapper();
}

void cpp_SteamServerWrapper_Destroy()
{
	delete steam_server_wrapper;
	steam_server_wrapper = nullptr;
}




class SteamServerClientWrapper
{
public:
	SteamServerClientWrapper()
		:
		m_LobbyGameCreated(this, &SteamServerClientWrapper::OnLobbyGameCreated),
		m_GameJoinRequested(this, &SteamServerClientWrapper::OnGameJoinRequested),
		m_AvatarImageLoadedCreated(this, &SteamServerClientWrapper::OnAvatarImageLoaded),
		m_SteamServersConnected(this, &SteamServerClientWrapper::OnSteamServersConnected),
		m_SteamServersDisconnected(this, &SteamServerClientWrapper::OnSteamServersDisconnected),
		m_SteamServerConnectFailure(this, &SteamServerClientWrapper::OnSteamServerConnectFailure),
		m_CallbackGameOverlayActivated(this, &SteamServerClientWrapper::OnGameOverlayActivated),
		m_CallbackGameWebCallback( this, &SteamServerClientWrapper::OnGameWebCallback),
		m_CallbackWorkshopItemInstalled(this, &SteamServerClientWrapper::OnWorkshopItemInstalled),
		m_CallbackP2PSessionConnectFail(this, &SteamServerClientWrapper::OnP2PSessionConnectFail),
		m_CallbackP2PSessionRequest(this, &SteamServerClientWrapper::OnP2PSessionRequest),
		m_CallbackLobbyDataUpdate(this, &SteamServerClientWrapper::OnLobbyDataUpdate),
		m_IPCFailureCallback(this, &SteamServerClientWrapper::OnIPCFailure),
		m_SteamShutdownCallback(this, &SteamServerClientWrapper::OnSteamShutdown)
	{
		cpp_SteamServerClientWrapper_OnLobbyGameCreated = nullptr;
		cpp_SteamServerClientWrapper_OnGameJoinRequested = nullptr;
		cpp_SteamServerClientWrapper_OnAvatarImageLoaded = nullptr;
		cpp_SteamServerClientWrapper_OnSteamServersConnected = nullptr;
		cpp_SteamServerClientWrapper_OnSteamServersDisconnected = nullptr;
		cpp_SteamServerClientWrapper_OnSteamServerConnectFailure = nullptr;
		//cpp_SteamServerClientWrapper_OnGameOverlayActivated = nullptr;
		cpp_SteamServerClientWrapper_OnGameWebCallback = nullptr;
		cpp_SteamServerClientWrapper_OnWorkshopItemInstalled = nullptr;
		cpp_SteamServerClientWrapper_OnP2PSessionConnectFail = nullptr;
		cpp_SteamServerClientWrapper_OnP2PSessionRequest = nullptr;
		cpp_SteamServerClientWrapper_OnIPCFailure = nullptr;
		cpp_SteamServerClientWrapper_OnSteamShutdown = nullptr;
		cpp_SteamServerClientWrapper_OnLobbyDataUpdate = nullptr;
	}

	STEAM_CALLBACK(SteamServerClientWrapper, OnLobbyDataUpdate, LobbyDataUpdate_t, m_CallbackLobbyDataUpdate);

	STEAM_CALLBACK(SteamServerClientWrapper, OnLobbyGameCreated, LobbyGameCreated_t, m_LobbyGameCreated);

	STEAM_CALLBACK(SteamServerClientWrapper, OnGameJoinRequested, GameLobbyJoinRequested_t, m_GameJoinRequested);

	//STEAM_CALLBACK( CSpaceWarClient, OnAvatarImageLoaded, AvatarImageLoaded_t, m_AvatarImageLoadedCreated );
	STEAM_CALLBACK(SteamServerClientWrapper, OnAvatarImageLoaded, AvatarImageLoaded_t, m_AvatarImageLoadedCreated); //TODO: Finish.

	STEAM_CALLBACK(SteamServerClientWrapper, OnSteamServersConnected, SteamServersConnected_t, m_SteamServersConnected);

	STEAM_CALLBACK(SteamServerClientWrapper, OnSteamServersDisconnected, SteamServersDisconnected_t, m_SteamServersDisconnected);

	STEAM_CALLBACK(SteamServerClientWrapper, OnSteamServerConnectFailure, SteamServerConnectFailure_t, m_SteamServerConnectFailure);

	STEAM_CALLBACK(SteamServerClientWrapper, OnGameOverlayActivated, GameOverlayActivated_t, m_CallbackGameOverlayActivated);

	//STEAM_CALLBACK( CSpaceWarClient, OnGameWebCallback, GameWebCallback_t, m_CallbackGameWebCallback );
	STEAM_CALLBACK(SteamServerClientWrapper, OnGameWebCallback, GameWebCallback_t, m_CallbackGameWebCallback); //TODO: Finish.

	//STEAM_CALLBACK(CSpaceWarClient, OnWorkshopItemInstalled, ItemInstalled_t, m_CallbackWorkshopItemInstalled);
	STEAM_CALLBACK(SteamServerClientWrapper, OnWorkshopItemInstalled, ItemInstalled_t, m_CallbackWorkshopItemInstalled); //TODO: Finish.

	STEAM_CALLBACK(SteamServerClientWrapper, OnP2PSessionConnectFail, P2PSessionConnectFail_t, m_CallbackP2PSessionConnectFail);

	STEAM_CALLBACK(SteamServerClientWrapper, OnIPCFailure, IPCFailure_t, m_IPCFailureCallback);

	STEAM_CALLBACK(SteamServerClientWrapper, OnSteamShutdown, SteamShutdown_t, m_SteamShutdownCallback);

	STEAM_CALLBACK(SteamServerClientWrapper, OnP2PSessionRequest, P2PSessionRequest_t, m_CallbackP2PSessionRequest);

	void OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure);
	CCallResult<SteamServerClientWrapper, LobbyCreated_t> m_SteamCallResultLobbyCreated;
	void m_SteamCallResultLobbyCreated_Set(SteamAPICall_t hSteamAPICall);

	void OnLobbyEntered( LobbyEnter_t* pCallback, bool bIOFailure );
	CCallResult<SteamServerClientWrapper, LobbyEnter_t> m_SteamCallResultLobbyEntered; //Why isn't this set in the example?
	void m_SteamCallResultLobbyEntered_Set(SteamAPICall_t hSteamAPICall);

	void OnLobbyMatchListCallback( LobbyMatchList_t* pCallback, bool bIOFailure );
	CCallResult<SteamServerClientWrapper, LobbyMatchList_t> m_SteamCallResultLobbyMatchList;
	void m_SteamCallResultLobbyMatchList_Set(SteamAPICall_t hSteamAPICall);

	// Called when SteamUser()->RequestEncryptedAppTicket() returns asynchronously
	void OnRequestEncryptedAppTicket( EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure );
	CCallResult<SteamServerClientWrapper, EncryptedAppTicketResponse_t> m_SteamCallResultEncryptedAppTicket;
	void m_SteamCallResultEncryptedAppTicket_Set(SteamAPICall_t hSteamAPICall);
	void RetrieveSteamIDFromGameServer( uint32_t m_unServerIP, uint16_t m_usServerPort );
	void GetNumberOfCurrentPlayers();

private:
	void OnGetNumberOfCurrentPlayers( NumberOfCurrentPlayers_t *pCallback, bool bIOFailure );
	CCallResult< SteamServerClientWrapper, NumberOfCurrentPlayers_t > m_NumberOfCurrentPlayersCallResult;
	// simple class to marshal callbacks from pinging a game server
	class CGameServerPing : public ISteamMatchmakingPingResponse
	{
	public:
		CGameServerPing()
		{
			m_hGameServerQuery = HSERVERQUERY_INVALID;
			m_pClient = NULL;
		}

		void RetrieveSteamIDFromGameServer( SteamServerClientWrapper* pClient, uint32_t unIP, uint16_t unPort )
		{
			m_pClient = pClient;
			m_hGameServerQuery = SteamMatchmakingServers()->PingServer( unIP, unPort, this );
		}

		void CancelPing()
		{
			m_hGameServerQuery = HSERVERQUERY_INVALID;
		}

		// Server has responded successfully and has updated data
		virtual void ServerResponded( gameserveritem_t& server )
		{
			if ( m_hGameServerQuery != HSERVERQUERY_INVALID && server.m_steamID.IsValid() )
			{
				(*cpp_SteamServerClientWrapper_GameServerPingOnServerResponded)( static_cast<void*>(&server.m_steamID) );
			}

			m_hGameServerQuery = HSERVERQUERY_INVALID;
		}

		// Server failed to respond to the ping request
		virtual void ServerFailedToRespond()
		{
			m_hGameServerQuery = HSERVERQUERY_INVALID;
		}

	private:
		HServerQuery m_hGameServerQuery;	// we're pinging a game server so we can convert IP:Port to a steamID
		SteamServerClientWrapper* m_pClient;
	};
	CGameServerPing m_GameServerPing;
}* steam_server_client_wrapper; //TODO: Initialize this...where?



void SteamServerClientWrapper::OnLobbyDataUpdate(LobbyDataUpdate_t* pCallback)
{
	if (cpp_SteamServerClientWrapper_OnLobbyDataUpdate)
	{
		(*cpp_SteamServerClientWrapper_OnLobbyDataUpdate)(pCallback);
	}
}

void SteamServerClientWrapper::OnP2PSessionRequest(P2PSessionRequest_t* pCallback)
{
	if (cpp_SteamServerClientWrapper_OnP2PSessionRequest)
	{
		(*cpp_SteamServerClientWrapper_OnP2PSessionRequest)(pCallback);
	}
}

void c_RetrieveSteamIDFromGameServer( uint32_t m_unServerIP, uint16_t m_usServerPort )
{
	steam_server_client_wrapper->RetrieveSteamIDFromGameServer( m_unServerIP, m_usServerPort );
}

void SteamServerClientWrapper::RetrieveSteamIDFromGameServer( uint32_t m_unServerIP, uint16_t m_usServerPort )
{
	m_GameServerPing.RetrieveSteamIDFromGameServer( this, m_unServerIP, m_usServerPort );
}

void SteamServerClientWrapper::OnLobbyGameCreated(LobbyGameCreated_t* pCallback)
{
	if (cpp_SteamServerClientWrapper_OnLobbyGameCreated)
	{
		(*cpp_SteamServerClientWrapper_OnLobbyGameCreated)(pCallback);
	}
}

void SteamServerClientWrapper::OnGameJoinRequested(GameLobbyJoinRequested_t* pCallback)
{
	if (cpp_SteamServerClientWrapper_OnGameJoinRequested)
	{
		(*cpp_SteamServerClientWrapper_OnGameJoinRequested)(pCallback);
	}
}

void SteamServerClientWrapper::OnAvatarImageLoaded(AvatarImageLoaded_t* pCallback)
{
	if (cpp_SteamServerClientWrapper_OnAvatarImageLoaded)
	{
		(*cpp_SteamServerClientWrapper_OnAvatarImageLoaded)(pCallback);
	}
}

void SteamServerClientWrapper::OnSteamServersConnected(SteamServersConnected_t* callback)
{
	if (cpp_SteamServerClientWrapper_OnSteamServersConnected)
	{
		(*cpp_SteamServerClientWrapper_OnSteamServersConnected)(callback);
	}
}

void SteamServerClientWrapper::OnSteamServersDisconnected(SteamServersDisconnected_t* callback)
{
	if (cpp_SteamServerClientWrapper_OnSteamServersDisconnected)
	{
		(*cpp_SteamServerClientWrapper_OnSteamServersDisconnected)(callback);
	}
}

void SteamServerClientWrapper::OnSteamServerConnectFailure(SteamServerConnectFailure_t* callback)
{
	if (cpp_SteamServerClientWrapper_OnSteamServerConnectFailure)
	{
		(*cpp_SteamServerClientWrapper_OnSteamServerConnectFailure)(callback);
	}
}

void SteamServerClientWrapper::OnGameOverlayActivated(GameOverlayActivated_t* callback)
{
	if (!callback)
	{
		return;
	}

#ifdef STEAMDEBUG
	printlog("OnGameOverlayActivated\n");
#endif

	if (callback->m_bActive)
	{
		pauseGame(2, MAXPLAYERS);
		SDL_SetRelativeMouseMode(SDL_FALSE); //Uncapture mouse. (Workaround for OSX Steam's inability to display a mouse in the game overlay UI)
		SDL_ShowCursor(SDL_TRUE); //(Workaround for OSX Steam's inability to display a mouse in the game overlay UI)
	}
	else
	{
		if (shootmode && !gamePaused)
		{
			SDL_SetRelativeMouseMode(SDL_TRUE); //Recapture mouse.
		}
		SDL_ShowCursor(SDL_FALSE);
	}
}

void SteamServerClientWrapper::OnGameWebCallback(GameWebCallback_t* callback)
{
	if (cpp_SteamServerClientWrapper_OnGameWebCallback)
	{
		(*cpp_SteamServerClientWrapper_OnGameWebCallback)(callback);
	}
}

void SteamServerClientWrapper::OnWorkshopItemInstalled(ItemInstalled_t* pParam)
{
	if (cpp_SteamServerClientWrapper_OnWorkshopItemInstalled)
	{
		(cpp_SteamServerClientWrapper_OnWorkshopItemInstalled)(pParam);
	}
}

void SteamServerClientWrapper::OnP2PSessionConnectFail(P2PSessionConnectFail_t* pCallback)
{
	if (cpp_SteamServerClientWrapper_OnP2PSessionConnectFail)
	{
		(*cpp_SteamServerClientWrapper_OnP2PSessionConnectFail)(pCallback);
	}
}

void SteamServerClientWrapper::OnIPCFailure(IPCFailure_t* failure)
{
	if (cpp_SteamServerClientWrapper_OnIPCFailure)
	{
		(*cpp_SteamServerClientWrapper_OnIPCFailure)(failure);
	}
}

void SteamServerClientWrapper::OnSteamShutdown(SteamShutdown_t* callback)
{
	if (cpp_SteamServerClientWrapper_OnSteamShutdown)
	{
		(*cpp_SteamServerClientWrapper_OnSteamShutdown)(callback);
	}
}

void SteamServerClientWrapper::OnLobbyCreated(LobbyCreated_t* pCallback, bool bIOFailure)
{
	if (cpp_SteamServerClientWrapper_OnLobbyCreated)
	{
		(*cpp_SteamServerClientWrapper_OnLobbyCreated)(pCallback, bIOFailure);
	}
}

void SteamServerClientWrapper::m_SteamCallResultLobbyCreated_Set(SteamAPICall_t hSteamAPICall)
{
	m_SteamCallResultLobbyCreated.Set(hSteamAPICall, this, &SteamServerClientWrapper::OnLobbyCreated);
}

void SteamServerClientWrapper::OnLobbyMatchListCallback(LobbyMatchList_t* pCallback, bool bIOFailure)
{
	if (cpp_SteamServerClientWrapper_OnLobbyMatchListCallback)
	{
		(*cpp_SteamServerClientWrapper_OnLobbyMatchListCallback)(pCallback, bIOFailure);
	}
}

void SteamServerClientWrapper::m_SteamCallResultLobbyMatchList_Set(SteamAPICall_t hSteamAPICall)
{
	m_SteamCallResultLobbyMatchList.Set( hSteamAPICall, this, &SteamServerClientWrapper::OnLobbyMatchListCallback );
}

void SteamServerClientWrapper::OnLobbyEntered(LobbyEnter_t* pCallback, bool bIOFailure)
{
	if (cpp_SteamServerClientWrapper_OnLobbyEntered)
	{
		(*cpp_SteamServerClientWrapper_OnLobbyEntered)(pCallback, bIOFailure);
	}
}

void SteamServerClientWrapper::m_SteamCallResultLobbyEntered_Set(SteamAPICall_t hSteamAPICall)
{
	m_SteamCallResultLobbyEntered.Set(hSteamAPICall, this, &SteamServerClientWrapper::OnLobbyEntered);
}

void SteamServerClientWrapper::OnRequestEncryptedAppTicket(EncryptedAppTicketResponse_t* pEncryptedAppTicketResponse, bool bIOFailure)
{
	if (cpp_SteamServerClientWrapper_OnRequestEncryptedAppTicket)
	{
		(*cpp_SteamServerClientWrapper_OnRequestEncryptedAppTicket)(pEncryptedAppTicketResponse, bIOFailure);
	}
}

void SteamServerClientWrapper::m_SteamCallResultEncryptedAppTicket_Set(SteamAPICall_t hSteamAPICall)
{
	m_SteamCallResultEncryptedAppTicket.Set(hSteamAPICall, this, &SteamServerClientWrapper::OnRequestEncryptedAppTicket);
}



SteamAPICall_t cpp_SteamMatchmaking_RequestLobbyList()
{
	SteamAPICall_t m_SteamCallResultLobbyMatchList = SteamMatchmaking()->RequestLobbyList();
	steam_server_client_wrapper->m_SteamCallResultLobbyMatchList_Set(m_SteamCallResultLobbyMatchList);
	return m_SteamCallResultLobbyMatchList;
}

SteamAPICall_t cpp_SteamMatchmaking_JoinLobby(CSteamID steamIDLobby)
{
	SteamAPICall_t steamAPICall = SteamMatchmaking()->JoinLobby(steamIDLobby);
	steam_server_client_wrapper->m_SteamCallResultLobbyEntered_Set(steamAPICall);
	return steamAPICall;
}

SteamAPICall_t cpp_SteamMatchmaking_CreateLobby(ELobbyType eLobbyType, int cMaxMembers)
{
	SteamAPICall_t steamAPICall = SteamMatchmaking()->CreateLobby(eLobbyType, cMaxMembers);
	steam_server_client_wrapper->m_SteamCallResultLobbyCreated_Set(steamAPICall);
	return steamAPICall;
}

void cpp_SteamServerClientWrapper_Instantiate()
{
	steam_server_client_wrapper = new SteamServerClientWrapper();
}

void cpp_SteamServerClientWrapper_Destroy()
{
	delete steam_server_client_wrapper;
	steam_server_client_wrapper = nullptr;
}

// Make the asynchronous request to receive the number of current players.
void SteamServerClientWrapper::GetNumberOfCurrentPlayers()
{
	//printlog("Getting Number of Current Players\n");
	SteamAPICall_t hSteamAPICall = SteamUserStats()->GetNumberOfCurrentPlayers();
	m_NumberOfCurrentPlayersCallResult.Set(hSteamAPICall, this, &SteamServerClientWrapper::OnGetNumberOfCurrentPlayers);
}

// Called when SteamUserStats()->GetNumberOfCurrentPlayers() returns asynchronously, after a call to SteamAPI_RunCallbacks().
void SteamServerClientWrapper::OnGetNumberOfCurrentPlayers(NumberOfCurrentPlayers_t *pCallback, bool bIOFailure)
{
	if ( bIOFailure || !pCallback->m_bSuccess )
	{
		//printlog("NumberOfCurrentPlayers_t failed!\n");
		return;
	}

	//printlog("Number of players currently playing: %d\n", pCallback->m_cPlayers);
}

#endif //defined Steamworks

/* ***** END UTTER BODGE ***** */




/*-------------------------------------------------------------------------------

	achievementUnlocked

	Returns true if the given achievement has been unlocked this game,
	false otherwise

-------------------------------------------------------------------------------*/

bool achievementUnlocked(const char* achName)
{
#ifndef STEAMWORKS
	return false;
#else

	// check internal achievement record
	node_t* node;
	for ( node = steamAchievements.first; node != NULL; node = node->next )
	{
		char* ach = (char*)node->element;
		if ( !strcmp(ach, achName) )
		{
			return true;
		}
	}
	return false;

#endif
}

/*-------------------------------------------------------------------------------

	steamAchievement

	Unlocks a steam achievement

-------------------------------------------------------------------------------*/

void steamAchievement(const char* achName)
{
#ifndef STEAMWORKS
	return;
#else

	if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
		|| conductGameChallenges[CONDUCT_LIFESAVING]
		|| gamemods_disableSteamAchievements )
	{
		// cheats/mods have been enabled on savefile, disallow achievements.
		return;
	}

	if ( !strcmp(achName, "BARONY_ACH_BOOTS_OF_SPEED") )
	{
		conductGameChallenges[CONDUCT_BOOTS_SPEED] = 1; // to cover bases when lich or devil dies as we can't remotely update this for clients.
	}
	//messagePlayer(clientnum, "%s", achName);

	if ( !achievementUnlocked(achName) )
	{
		//messagePlayer(clientnum, "You've unlocked an achievement!\n [%s]",c_SteamUserStats_GetAchievementDisplayAttribute(achName,"name"));
		SteamUserStats()->SetAchievement(achName);
		SteamUserStats()->StoreStats();

		char* ach = (char*) malloc(sizeof(char) * (strlen(achName) + 1));
		strcpy(ach, achName);
		node_t* node = list_AddNodeFirst(&steamAchievements);
		node->element = ach;
		node->size = sizeof(char) * (strlen(achName) + 1);
		node->deconstructor = &defaultDeconstructor;
	}

#endif
}

/*-------------------------------------------------------------------------------

	steamAchievementClient

	Tells a client to unlock a steam achievement (server only)

-------------------------------------------------------------------------------*/

void steamAchievementClient(int player, const char* achName)
{
	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	else if ( player == 0 )
	{
		steamAchievement(achName);
	}
	else
	{
		if ( client_disconnected[player] || multiplayer == SINGLE )
		{
			return;
		}
		strcpy((char*)net_packet->data, "SACH");
		strcpy((char*)(&net_packet->data[4]), achName);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 4 + strlen(achName) + 1;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
}

void steamAchievementEntity(Entity* my, const char* achName)
{
	if ( !my )
	{
		return;
	}

	if ( my->behavior == &actPlayer )
	{
		steamAchievementClient(my->skill[2], achName);
	}
}

void steamStatisticUpdate(int statisticNum, ESteamStatTypes type, int value)
{
#ifndef STEAMWORKS
	return;
#else
	if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
		|| conductGameChallenges[CONDUCT_LIFESAVING]
		|| gamemods_disableSteamAchievements )
	{
		// cheats/mods have been enabled on savefile, disallow statistics update.
		return;
	}

	if ( statisticNum >= NUM_STEAM_STATISTICS || statisticNum < 0 )
	{
		return;
	}
	bool indicateProgress = true;
	bool result = false;
	switch ( type )
	{
		case STEAM_STAT_INT:
		{
			int oldValue = g_SteamStats[statisticNum].m_iValue;
			g_SteamStats[statisticNum].m_iValue += value;
			switch ( statisticNum )
			{
				case STEAM_STAT_RHINESTONE_COWBOY:
				case STEAM_STAT_TOUGH_AS_NAILS:
				case STEAM_STAT_UNSTOPPABLE_FORCE:
				case STEAM_STAT_BOMBARDIER:
				case STEAM_STAT_IN_THE_MIX:
				case STEAM_STAT_FREE_REFILLS:
				case STEAM_STAT_TAKE_THIS_OUTSIDE:
				case STEAM_STAT_BLOOD_SPORT:
				case STEAM_STAT_IRON_GUT:
				case STEAM_STAT_BOTTLE_NOSED:
				case STEAM_STAT_BARFIGHT_CHAMP:
				case STEAM_STAT_VOLATILE:
				case STEAM_STAT_SURROGATES:
				case STEAM_STAT_KILL_COMMAND:
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					break;
				case STEAM_STAT_ALTER_EGO:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 1000 && ((oldValue / 1000) < (g_SteamStats[statisticNum].m_iValue / 1000)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 5000) < (g_SteamStats[statisticNum].m_iValue / 5000)) )
					{
						indicateProgress = true;
					}
					break;
				case STEAM_STAT_BAD_BLOOD:
					indicateProgress = false;
					g_SteamStats[statisticNum].m_iValue =
						std::min(g_SteamStats[statisticNum].m_iValue, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( g_SteamStats[statisticNum].m_iValue == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						indicateProgress = true;
					}
					else if ( oldValue == 0 && g_SteamStats[statisticNum].m_iValue > 0 )
					{
						indicateProgress = true;
					}
					else if ( oldValue < 20 && ((oldValue / 20) < (g_SteamStats[statisticNum].m_iValue / 20)) )
					{
						indicateProgress = true;
					}
					else if ( ((oldValue / 50) < (g_SteamStats[statisticNum].m_iValue / 50)) )
					{
						indicateProgress = true;
					}
					break;
				default:
					break;
			}
			break;
		}
		case STEAM_STAT_FLOAT:
			break;
		default:
			break;
	}
	g_SteamStatistics->StoreStats(); // update server's stat counter.
	if ( indicateProgress )
	{
		steamIndicateStatisticProgress(statisticNum, type);
	}
#endif
}

void steamStatisticUpdateClient(int player, int statisticNum, ESteamStatTypes type, int value)
{
#ifndef STEAMWORKS
	return;
#else
	if ( conductGameChallenges[CONDUCT_CHEATS_ENABLED] 
		|| conductGameChallenges[CONDUCT_LIFESAVING]
		|| gamemods_disableSteamAchievements )
	{
		// cheats/mods have been enabled on savefile, disallow statistics update.
		return;
	}

	if ( statisticNum >= NUM_STEAM_STATISTICS || statisticNum < 0 )
	{
		return;
	}

	if ( multiplayer == CLIENT )
	{
		return;
	}

	if ( player == 0 )
	{
		steamStatisticUpdate(statisticNum, type, value);
		return;
	}
	else if ( player < 0 || player >= MAXPLAYERS )
	{
		return;
	}
	else
	{
		if ( client_disconnected[player] || multiplayer == SINGLE )
		{
			return;
		}
		strcpy((char*)net_packet->data, "SSTA");
		net_packet->data[4] = static_cast<Uint8>(statisticNum);
		net_packet->data[5] = static_cast<Uint8>(type);
		SDLNet_Write16(value, &net_packet->data[6]);
		net_packet->address.host = net_clients[player - 1].host;
		net_packet->address.port = net_clients[player - 1].port;
		net_packet->len = 8;
		sendPacketSafe(net_sock, -1, net_packet, player - 1);
	}
#endif
}


void steamIndicateStatisticProgress(int statisticNum, ESteamStatTypes type)
{
#ifndef STEAMWORKS
	return;
#else

	if ( statisticNum >= NUM_STEAM_STATISTICS || statisticNum < 0 )
	{
		return;
	}

	int iVal = g_SteamStats[statisticNum].m_iValue;
	float fVal = g_SteamStats[statisticNum].m_flValue;
	if ( type == STEAM_STAT_INT )
	{
		switch ( statisticNum )
		{
			case STEAM_STAT_RHINESTONE_COWBOY:
			case STEAM_STAT_TOUGH_AS_NAILS:
			case STEAM_STAT_UNSTOPPABLE_FORCE:
			case STEAM_STAT_BOMBARDIER:
			case STEAM_STAT_IN_THE_MIX:
			case STEAM_STAT_FREE_REFILLS:
			case STEAM_STAT_BLOOD_SPORT:
			case STEAM_STAT_BARFIGHT_CHAMP:
			case STEAM_STAT_SURROGATES:
			case STEAM_STAT_KILL_COMMAND:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					if ( iVal == 1 || (iVal > 0 && iVal % 5 == 0) )
					{
						SteamUserStats()->IndicateAchievementProgress(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
							iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
						if ( iVal == steamStatAchStringsAndMaxVals[statisticNum].second )
						{
							steamAchievement(steamStatAchStringsAndMaxVals[statisticNum].first.c_str());
						}
					}
				}
				break;
			case STEAM_STAT_IRON_GUT:
			case STEAM_STAT_BOTTLE_NOSED:
			case STEAM_STAT_VOLATILE:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					if ( iVal == 1 || (iVal > 0 && iVal % 4 == 0) )
					{
						SteamUserStats()->IndicateAchievementProgress(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
							iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
						if ( iVal == steamStatAchStringsAndMaxVals[statisticNum].second )
						{
							steamAchievement(steamStatAchStringsAndMaxVals[statisticNum].first.c_str());
						}
					}
				}
				break;
			case STEAM_STAT_BAD_BLOOD:
			case STEAM_STAT_ALTER_EGO:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					SteamUserStats()->IndicateAchievementProgress(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
						iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
					if ( iVal == steamStatAchStringsAndMaxVals[statisticNum].second )
					{
						steamAchievement(steamStatAchStringsAndMaxVals[statisticNum].first.c_str());
					}
				}
				break;
			case STEAM_STAT_TAKE_THIS_OUTSIDE:
				if ( !achievementUnlocked(steamStatAchStringsAndMaxVals[statisticNum].first.c_str()) )
				{
					if ( iVal == 1 || (iVal > 0 && iVal % 2 == 0) )
					{
						SteamUserStats()->IndicateAchievementProgress(steamStatAchStringsAndMaxVals[statisticNum].first.c_str(),
							iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
						if ( iVal == steamStatAchStringsAndMaxVals[statisticNum].second )
						{
							steamAchievement(steamStatAchStringsAndMaxVals[statisticNum].first.c_str());
						}
					}
				}
				break;
			default:
				break;
		}
		//messagePlayer(clientnum, "%s: %d, %d", steamStatAchStringsAndMaxVals[statisticNum].first.c_str(), 
			//iVal, steamStatAchStringsAndMaxVals[statisticNum].second);
	}
#endif // !STEAMWORKS
}

#ifdef STEAMWORKS
//#define STEAMDEBUG

/*-------------------------------------------------------------------------------

	steam callback functions

	handle various steam callbacks; bound in init_game.c

-------------------------------------------------------------------------------*/

//Helper func. //TODO: Bugger.
void* cpp_P2PSessionRequest_t_m_steamIDRemote(void* P2PSessionRequest_t_instance)
{
	CSteamID* id = new CSteamID; //TODO: Memleak?
	*id = static_cast<P2PSessionRequest_t*>(P2PSessionRequest_t_instance)->m_steamIDRemote;
	return id;
}

void steam_OnP2PSessionRequest( void* p_Callback )
{
#ifdef STEAMDEBUG
	printlog( "OnP2PSessionRequest\n" );
#endif
	SteamNetworking()->AcceptP2PSessionWithUser(*static_cast<CSteamID* >(cpp_P2PSessionRequest_t_m_steamIDRemote(p_Callback)));
}

//Helper func. //TODO: Bugger.
void cpp_Free_CSteamID(void* steamID)
{
	CSteamID* id = static_cast<CSteamID*>(steamID);
	delete id;
}

//Helper func. //TODO: Bugger.
void* cpp_SteamMatchmaking_GetLobbyByIndex(int iLobby)
{
	CSteamID* id = new CSteamID();
	*id = SteamMatchmaking()->GetLobbyByIndex(iLobby);
	return id;
}

void steam_OnLobbyMatchListCallback( void* pCallback, bool bIOFailure )
{
	Uint32 iLobby;

	if ( !requestingLobbies )
	{
		return;
	}

	for ( iLobby = 0; iLobby < MAX_STEAM_LOBBIES; iLobby++ )
	{
		if ( lobbyIDs[iLobby] )
		{
			cpp_Free_CSteamID(lobbyIDs[iLobby]); //TODO: This is an utter bodge. Make it not a list of void pointers and then just directly delete the ID.
			lobbyIDs[iLobby] = NULL;
		}
	}
	requestingLobbies = false;

	if ( bIOFailure )
	{
		// we had a Steam I/O failure - we probably timed out talking to the Steam back-end servers
		// doesn't matter in this case, we can just act if no lobbies were received
	}

	// lobbies are returned in order of closeness to the user, so add them to the list in that order
	numSteamLobbies = std::min<uint32>(static_cast<LobbyMatchList_t*>(pCallback)->m_nLobbiesMatching, MAX_STEAM_LOBBIES);
	for ( iLobby = 0; iLobby < numSteamLobbies; iLobby++ )
	{
		void* steamIDLobby = cpp_SteamMatchmaking_GetLobbyByIndex( iLobby ); //TODO: Bugger this void pointer!

		// add the lobby to the list
		lobbyIDs[iLobby] = steamIDLobby;

		// pull some info from the lobby metadata (name, players, etc)
		const char* lobbyName = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(steamIDLobby), "name"); //TODO: Again with the void pointers.
		const char* lobbyVersion = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(steamIDLobby), "ver"); //TODO: VOID.
		int numPlayers = SteamMatchmaking()->GetNumLobbyMembers(*static_cast<CSteamID*>(steamIDLobby)); //TODO MORE VOID POINTERS.
		const char* lobbyNumMods = SteamMatchmaking()->GetLobbyData(*static_cast<CSteamID*>(steamIDLobby), "svNumMods"); //TODO: VOID.
		int numMods = atoi(lobbyNumMods);
		string versionText = lobbyVersion;
		if ( versionText ==  "" )
		{
			//If the lobby version is null
			versionText = "Unknown version";
		}

		if ( lobbyName && lobbyName[0] && numPlayers )
		{
			// set the lobby data
			if ( numMods > 0 )
			{
				snprintf(lobbyText[iLobby], 47, "%s (%s) [MODDED]", lobbyName, versionText.c_str()); //TODO: shorten?
			}
			else
			{
				snprintf( lobbyText[iLobby], 47, "%s (%s)", lobbyName, versionText.c_str()); //TODO: Perhaps a better method would be to print the name and the version as two separate strings ( because some steam names are ridiculously long).
			}
			lobbyPlayers[iLobby] = numPlayers;
		}
		else
		{
			// we don't have info about the lobby yet, request it
			SteamMatchmaking()->RequestLobbyData(*static_cast<CSteamID*>(steamIDLobby));

			// results will be returned via LobbyDataUpdate_t callback
			snprintf( lobbyText[iLobby], 47, "Lobby %d", static_cast<CSteamID*>(steamIDLobby)->GetAccountID() ); //TODO: MORE VOID POINTER BUGGERY.
			lobbyPlayers[iLobby] = 0;
		}
	}
}

//Helper func. //TODO: Bugger it!
void* cpp_LobbyDataUpdated_pCallback_m_ulSteamIDLobby(void* pCallback)
{
	CSteamID* id = new CSteamID();
	*id = static_cast<LobbyDataUpdate_t*>(pCallback)->m_ulSteamIDLobby;
	return id;
}

void steam_OnLobbyDataUpdatedCallback( void* pCallback )
{
#ifdef STEAMDEBUG
	printlog( "OnLobbyDataUpdatedCallback\n" );
#endif

	// finish processing lobby invite?
	if ( stillConnectingToLobby )
	{
		stillConnectingToLobby = false;

		void processLobbyInvite();
		processLobbyInvite();
		return;
	}

	// update current lobby info
	void* tempSteamID = cpp_LobbyDataUpdated_pCallback_m_ulSteamIDLobby(pCallback); //TODO: BUGGER VOID POINTER.
	if ( currentLobby )
	{
		if ( (static_cast<CSteamID*>(currentLobby))->ConvertToUint64() == (static_cast<CSteamID*>(tempSteamID))->ConvertToUint64() )
		{
			// extract the display name from the lobby metadata
			const char* lobbyName = SteamMatchmaking()->GetLobbyData( *static_cast<CSteamID*>(currentLobby), "name" );
			if ( lobbyName )
			{
				snprintf( currentLobbyName, 31, lobbyName );
			}

			// get the server flags
			const char* svFlagsChar = SteamMatchmaking()->GetLobbyData( *static_cast<CSteamID*>(currentLobby), "svFlags" );
			if ( svFlagsChar )
			{
				svFlags = atoi(svFlagsChar);
			}
		}
	}
	cpp_Free_CSteamID(tempSteamID);
}

//Helper func. //TODO: BUGGER THIS.
void* cpp_LobbyCreated_Lobby(void* pCallback)
{
	CSteamID* id = new CSteamID;
	*id = static_cast<LobbyCreated_t*>(pCallback)->m_ulSteamIDLobby;
	return id;
}

void steam_OnLobbyCreated( void* pCallback, bool bIOFailure )
{
#ifdef STEAMDEBUG
	printlog( "OnLobbyCreated\n" );
#endif
	if ( static_cast<EResult>(static_cast<LobbyCreated_t*>(pCallback)->m_eResult) == k_EResultOK )   //TODO: Make sure port from c_EResult to EResult works flawlessly.
	{
		if ( currentLobby )
		{
			SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
			cpp_Free_CSteamID(currentLobby); //TODO: BUGGER THIS.
			currentLobby = nullptr;
		}
		currentLobby = cpp_LobbyCreated_Lobby(pCallback);

		// set the name of the lobby
		snprintf( currentLobbyName, 31, "%s's lobby", SteamFriends()->GetPersonaName() );
		SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "name", currentLobbyName); //TODO: Bugger void pointer!

		// set the game version of the lobby
		SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "ver", VERSION); //TODO: Bugger void pointer!

		// set lobby server flags
		char svFlagsChar[16];
		snprintf(svFlagsChar, 15, "%d", svFlags);
		SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "svFlags", svFlagsChar); //TODO: Bugger void pointer!

		// set load game status on lobby
		char loadingsavegameChar[16];
		snprintf(loadingsavegameChar, 15, "%d", loadingsavegame);
		SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "loadingsavegame", loadingsavegameChar); //TODO: Bugger void pointer!

		char svNumMods[16];
		snprintf(svNumMods, 15, "%d", gamemods_numCurrentModsLoaded);
		SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), "svNumMods", svNumMods); //TODO: Bugger void pointer!

		if ( gamemods_numCurrentModsLoaded > 0 )
		{
			int count = 0;
			for ( std::vector<std::pair<std::string, std::string>>::iterator it = gamemods_mountedFilepaths.begin(); it != gamemods_mountedFilepaths.end(); ++it )
			{
				for ( std::vector<std::pair<std::string, uint64>>::iterator itMap = gamemods_workshopLoadedFileIDMap.begin();
					itMap != gamemods_workshopLoadedFileIDMap.end(); ++itMap )
				{
					if ( (itMap->first).compare(it->second) == 0 )
					{
						char svModFileID[64];
						snprintf(svModFileID, 64, "%d", static_cast<int>(itMap->second));
						char tagName[32] = "";
						snprintf(tagName, 32, "svMod%d", count);
						SteamMatchmaking()->SetLobbyData(*static_cast<CSteamID*>(currentLobby), tagName, svModFileID); //TODO: Bugger void pointer!
						++count;
						break;
					}
				}
			}
		}
	}
	else
	{
		printlog( "warning: failed to create steam lobby.\n");
	}
}

void processLobbyInvite()
{
	if ( !intro )
	{
		stillConnectingToLobby = true;
		return;
	}
	if ( !lobbyToConnectTo )
	{
		printlog( "warning: tried to process invitation to null lobby" );
		stillConnectingToLobby = false;
		return;
	}
	const char* loadingSaveGameChar = SteamMatchmaking()->GetLobbyData( *static_cast<CSteamID*>(lobbyToConnectTo), "loadingsavegame" );

	if ( loadingSaveGameChar && loadingSaveGameChar[0] )
	{
		Uint32 temp32 = atoi(loadingSaveGameChar);
		Uint32 gameKey = getSaveGameUniqueGameKey(false);
		if ( temp32 && temp32 == gameKey )
		{
			loadingsavegame = temp32;
			buttonLoadMultiplayerGame(NULL);
		}
		else if ( !temp32 )
		{
			loadingsavegame = 0;
			buttonOpenCharacterCreationWindow(NULL);
		}
		else
		{
			printlog("warning: received invitation to lobby with which you have an incompatible save game.\n");
			if ( lobbyToConnectTo )
			{
				cpp_Free_CSteamID(lobbyToConnectTo);    //TODO: Bodge this bodge!
			}
			lobbyToConnectTo = NULL;
		}
		stillConnectingToLobby = false;
	}
	else
	{
		stillConnectingToLobby = true;
		SteamMatchmaking()->RequestLobbyData(*static_cast<CSteamID*>(lobbyToConnectTo));
		printlog("warning: failed to determine whether lobby is using a saved game or not...\n");
	}
}

//Helper func. //TODO: Bugger.
void* cpp_GameJoinRequested_m_steamIDLobby(void* pCallback)
{
	CSteamID* id = new CSteamID;
	*id = static_cast<GameLobbyJoinRequested_t*>(pCallback)->m_steamIDLobby;
	return id;
}

void steam_OnGameJoinRequested( void* pCallback )
{
#ifdef STEAMDEBUG
	printlog( "OnGameJoinRequested\n" );
#endif

	// return to a state where we can join the lobby
	if ( !intro )
	{
		buttonEndGameConfirm(NULL);
	}
	else if ( multiplayer != SINGLE )
	{
		buttonDisconnect(NULL);
	}

	// close current window
	if ( subwindow )
	{
		if ( score_window )
		{
			// reset class loadout
			stats[0]->sex = static_cast<sex_t>(0);
			stats[0]->appearance = 0;
			stats[0]->playerRace = RACE_HUMAN;
			strcpy(stats[0]->name, "");
			stats[0]->type = HUMAN;
			client_classes[0] = 0;
			stats[0]->clearStats();
			initClass(0);
		}
		score_window = 0;
		gamemods_window = 0;
		lobby_window = false;
		settings_window = false;
		charcreation_step = 0;
		subwindow = 0;
		if ( SDL_IsTextInputActive() )
		{
			SDL_StopTextInput();
		}
	}
	list_FreeAll(&button_l);
	deleteallbuttons = true;

	if ( lobbyToConnectTo )
	{
		cpp_Free_CSteamID(lobbyToConnectTo); //TODO: Utter bodge.
	}
	lobbyToConnectTo = cpp_GameJoinRequested_m_steamIDLobby(pCallback);
	processLobbyInvite();
}

//Helper func. //TODO: Bugger.
void cpp_SteamMatchmaking_JoinLobbyPCH(const char* pchLobbyID)
{
	CSteamID steamIDLobby( (uint64)atoll( pchLobbyID ) );
	if ( steamIDLobby.IsValid() )
	{
		SteamAPICall_t steamAPICall = SteamMatchmaking()->JoinLobby(steamIDLobby);
		steam_server_client_wrapper->m_SteamCallResultLobbyEntered_Set(steamAPICall);
	}
}

// searches (char pchCmdLine[]) for a connect lobby command
void steam_ConnectToLobby()
{
#ifdef STEAMDEBUG
	printlog( "ConnectToLobby\n" );
#endif

	// parse out the connect
	char pchLobbyID[1024];

	// look for +connect_lobby command
	const char* pchConnectLobbyParam = "+connect_lobby";
	const char* pchConnectLobby = strstr( pchCmdLine, pchConnectLobbyParam );
	if ( pchConnectLobby )
	{
		// address should be right after the +connect_lobby, +1 on the end to skip the space
		strcpy( pchLobbyID, (char*)(pchConnectLobby + strlen(pchConnectLobbyParam) + 1 ));
	}

	// join lobby
	if (  pchLobbyID )
	{
		//c_SteamMatchmaking_JoinLobbyPCH( pchLobbyID, &steam_OnLobbyEntered );
		cpp_SteamMatchmaking_JoinLobbyPCH( pchLobbyID);
	}
}

void* cpp_pCallback_m_ulSteamIDLobby( void* pCallback )
{
	CSteamID* id = new CSteamID();
	*id = static_cast<LobbyEnter_t*>(pCallback)->m_ulSteamIDLobby;
	return id;
}

void steam_OnLobbyEntered( void* pCallback, bool bIOFailure )
{
#ifdef STEAMDEBUG
	printlog( "OnLobbyEntered\n" );
#endif

	if ( !connectingToLobby )
	{
		return;
	}

	if ( static_cast<LobbyEnter_t*>(pCallback)->m_EChatRoomEnterResponse != k_EChatRoomEnterResponseSuccess )
	{
		// lobby join failed
		connectingToLobby = false;
		connectingToLobbyWindow = false;
		openFailedConnectionWindow(CLIENT);
		return;
	}

	// success

	// move forward the state
	if ( currentLobby )
	{
		SteamMatchmaking()->LeaveLobby(*static_cast<CSteamID*>(currentLobby));
		cpp_Free_CSteamID(currentLobby); //TODO: Bugger.
		currentLobby = NULL;
	}
	currentLobby = cpp_pCallback_m_ulSteamIDLobby(pCallback); //TODO: More buggery.
	connectingToLobby = false;
}

void steam_GameServerPingOnServerResponded(void* steamID)
{
#ifdef STEAMDEBUG
	printlog( "GameServerPingOnServerResponded\n" );
#endif

	steamIDGameServer = steamID;
}

void steam_OnP2PSessionConnectFail( void* pCallback )
{
#ifdef STEAMDEBUG
	printlog( "OnP2PSessionConnectFail\n" );
#endif

	printlog("warning: failed to establish steam P2P connection.\n");

	if ( intro )
	{
		connectingToLobby = false;
		connectingToLobbyWindow = false;
		openFailedConnectionWindow(multiplayer);
	}
}

void CSteamLeaderboards::FindLeaderboard(const char *pchLeaderboardName)
{
	if ( !SteamUser()->BLoggedOn() )
	{
		return;
	}
	m_CurrentLeaderboard = NULL;
	b_LeaderboardInit = false;
	SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard(pchLeaderboardName);
	m_callResultFindLeaderboard.Set(hSteamAPICall, this,
		&CSteamLeaderboards::OnFindLeaderboard);
	// call OnFindLeaderboard when result of async API call
}

void CSteamLeaderboards::OnFindLeaderboard(LeaderboardFindResult_t *pCallback, bool bIOFailure)
{
	// see if we encountered an error during the call
	if ( !pCallback->m_bLeaderboardFound || bIOFailure )
	{
		printlog("[STEAM]: Error, could not find leaderboard %s!", leaderboardNames[LeaderboardUpload.boardIndex].c_str());
		ClearUploadData();
		return;
	}
	b_LeaderboardInit = true;
	m_CurrentLeaderboard = pCallback->m_hSteamLeaderboard;
	//DownloadScores(k_ELeaderboardDataRequestGlobal, 0, k_numEntriesToRetrieve);
}

bool CSteamLeaderboards::DownloadScores(ELeaderboardDataRequest dataRequestType, int rangeStart, int rangeEnd)
{
	if ( !m_CurrentLeaderboard )
	{
		return false;
	}
	b_ScoresDownloaded = false;

	// load the specified leaderboard data around the current user
	SteamAPICall_t hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(
		m_CurrentLeaderboard, dataRequestType, rangeStart, rangeEnd);
	m_callResultDownloadScore.Set(hSteamAPICall, this,
		&CSteamLeaderboards::OnDownloadScore);

	return true;
}

void CSteamLeaderboards::OnDownloadScore(LeaderboardScoresDownloaded_t *pCallback, bool bIOFailure)
{
	if ( !bIOFailure )
	{
		m_nLeaderboardEntries = std::min(pCallback->m_cEntryCount, (int)CSteamLeaderboards::k_numEntriesToRetrieve);
		for ( int i = 0; i < m_nLeaderboardEntries; ++i )
		{
			SteamUserStats()->GetDownloadedLeaderboardEntry(pCallback->m_hSteamLeaderboardEntries, 
				i, &m_leaderboardEntries[i], downloadedTags[i], k_numLeaderboardTags);
			leaderBoardSteamUsernames[i] = SteamFriends()->GetFriendPersonaName(m_leaderboardEntries[i].m_steamIDUser);
		}
		b_ScoresDownloaded = true;
	}
}

void CSteamLeaderboards::UploadScore(int scoreToSet, int tags[k_numLeaderboardTags])
{
	if ( !m_CurrentLeaderboard )
	{
		return;
	}

	// load the specified leaderboard data around the current user
	SteamAPICall_t hSteamAPICall = SteamUserStats()->UploadLeaderboardScore(m_CurrentLeaderboard, 
		k_ELeaderboardUploadScoreMethodKeepBest, scoreToSet, tags,
		k_numLeaderboardTags);
	m_callResultUploadScore.Set(hSteamAPICall, this,
		&CSteamLeaderboards::OnUploadScore);
}

void CSteamLeaderboards::OnUploadScore(LeaderboardScoreUploaded_t *pCallback, bool bIOFailure)
{
	if ( !bIOFailure && pCallback->m_bSuccess )
	{
		m_CurrentLeaderboard = pCallback->m_hSteamLeaderboard;
		LastUploadResult.b_ScoreUploadComplete = pCallback->m_bSuccess;
		LastUploadResult.b_ScoreChanged = pCallback->m_bScoreChanged;
		LastUploadResult.globalRankNew = pCallback->m_nGlobalRankNew;
		LastUploadResult.globalRankPrev = pCallback->m_nGlobalRankPrevious;
		LastUploadResult.scoreUploaded = pCallback->m_nScore;
	}
	else
	{
		LastUploadResult.b_ScoreUploadComplete = false;
		LastUploadResult.b_ScoreChanged = false;
		LastUploadResult.globalRankNew = 0;
		LastUploadResult.globalRankPrev = 0;
		LastUploadResult.scoreUploaded = 0;
	}
}

void CSteamLeaderboards::ClearUploadData()
{
	for ( int c = 0; c < k_numLeaderboardTags; ++c )
	{
		LeaderboardUpload.tags[c] = 0;
	}
	LeaderboardUpload.score = 0;
	LeaderboardUpload.time = 0;
	LeaderboardUpload.status = LEADERBOARD_STATE_NONE;
	LeaderboardUpload.boardIndex = LEADERBOARD_NONE;
	LeaderboardUpload.uploadInit = false;

	LastUploadResult.b_ScoreChanged = false;
	LastUploadResult.b_ScoreUploadComplete = false;
	LastUploadResult.globalRankNew = 0;
	LastUploadResult.globalRankPrev;
	LastUploadResult.scoreUploaded = 0;
}

void CSteamLeaderboards::ProcessLeaderboardUpload()
{
	if ( LeaderboardUpload.status == LEADERBOARD_STATE_NONE )
	{
		return;
	}

	if ( ticks % 25 == 0 )
	{
		if ( LeaderboardUpload.status == LEADERBOARD_STATE_FIND_LEADERBOARD_TIME )
		{
			FindLeaderboard(leaderboardNames[LeaderboardUpload.boardIndex].c_str());
			LeaderboardUpload.status = LEADERBOARD_STATE_UPLOADING_TIME;
			LastUploadResult.b_ScoreUploadComplete = false;
			LastUploadResult.b_ScoreChanged = false;
		}
		else if ( LeaderboardUpload.status == LEADERBOARD_STATE_UPLOADING_TIME )
		{
			if ( b_LeaderboardInit )
			{
				if ( !LeaderboardUpload.uploadInit )
				{
					UploadScore(LeaderboardUpload.time, LeaderboardUpload.tags);
					LeaderboardUpload.uploadInit = true;
				}
				if ( LastUploadResult.b_ScoreUploadComplete == true )
				{
					LeaderboardUpload.uploadInit = false;
					LeaderboardUpload.status = LEADERBOARD_STATE_READY_TIME;
					printlog("[STEAM]: Successfully uploaded leaderboard time to board name %s.", leaderboardNames[LeaderboardUpload.boardIndex].c_str());
					if ( LastUploadResult.b_ScoreChanged )
					{
						printlog("[STEAM]: Registered a new fastest time on the leaderboard!");
					}
					else
					{
						printlog("[STEAM]: You did not beat your previous leaderboard time.");
					}
					LastUploadResult.b_ScoreUploadComplete = false;
				}
			}
		}
		else if ( LeaderboardUpload.status == LEADERBOARD_STATE_READY_TIME )
		{
			FindLeaderboard(leaderboardNames[LeaderboardUpload.boardIndex + 1].c_str());
			LeaderboardUpload.status = LEADERBOARD_STATE_UPLOADING_SCORE;
			LastUploadResult.b_ScoreUploadComplete = false;
			LastUploadResult.b_ScoreChanged = false;
		}
		else if ( LeaderboardUpload.status == LEADERBOARD_STATE_UPLOADING_SCORE )
		{
			if ( b_LeaderboardInit )
			{
				if ( !LeaderboardUpload.uploadInit )
				{
					UploadScore(LeaderboardUpload.score, LeaderboardUpload.tags);
					LeaderboardUpload.uploadInit = true;
				}
				if ( LastUploadResult.b_ScoreUploadComplete == true )
				{
					LeaderboardUpload.uploadInit = false;
					LeaderboardUpload.status = LEADERBOARD_STATE_NONE;
					printlog("[STEAM]: Successfully uploaded leaderboard score to board name %s.", leaderboardNames[LeaderboardUpload.boardIndex + 1].c_str());
					LastUploadResult.b_ScoreUploadComplete = false;
					if ( LastUploadResult.b_ScoreChanged )
					{
						printlog("[STEAM]: Registered a new highest score on the leaderboard!");
					}
					else
					{
						printlog("[STEAM]: You did not beat your previous leaderboard score.");
					}
					ClearUploadData();
					DownloadScores(k_ELeaderboardDataRequestGlobal, 0, k_numEntriesToRetrieve);
				}
			}
		}
	}
}
#endif
