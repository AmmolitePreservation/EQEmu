#ifndef EQEMU_CLIENTMANAGER_H
#define EQEMU_CLIENTMANAGER_H

#include "../common/global_define.h"
#include "../common/opcodemgr.h"
#include "../common/net/eqstream.h"
#include "client.h"
#include <list>

class ClientManager {
public:
	ClientManager();
	~ClientManager();
	void Process();
	Client *GetClient(unsigned int c, const std::string &loginserver);

	/**
	 * The login session the world's answer to a play request is for: one account may have many sessions at once.
	 */
	Client *GetClientBySession(uint32 session_id);
private:
	void ProcessDisconnect();

	std::list<Client *>      m_clients;
	OpcodeManager            *m_sod_ops;
	EQ::Net::EQStreamManager *m_sod_stream;
	OpcodeManager            *m_larion_ops;
	EQ::Net::EQStreamManager *m_larion_stream;
};

#endif

