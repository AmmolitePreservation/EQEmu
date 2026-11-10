#ifndef DBCORE_H
#define DBCORE_H

#ifdef _WINDOWS
#include <winsock2.h>
#include <windows.h>
#endif

#include "../common/mutex.h"
#include "../common/mysql_request_result.h"
#include "../common/types.h"

#include <mysql.h>
#include <string.h>
#include <mutex>

#define CR_SERVER_GONE_ERROR    2006
#define CR_SERVER_LOST          2013

namespace mysql { class PreparedStmt; }

class DBcore {
public:
	enum eStatus {
		Closed, Connected, Error
	};

	DBcore();
	~DBcore();
	eStatus GetStatus() { return pStatus; }
	MySQLRequestResult QueryDatabase(const char *query, uint32 querylen, bool retryOnFailureOnce = true);
	MySQLRequestResult QueryDatabase(const std::string& query, bool retryOnFailureOnce = true);
	MySQLRequestResult QueryDatabaseMulti(const std::string &query);
	// Transactions nest: only the outermost Begin starts one and only the outermost Commit ends it, since MySQL commits
	// an open transaction when it sees a second START TRANSACTION; a Rollback at any depth makes the outermost Commit
	// roll back instead.
	void TransactionBegin();
	MySQLRequestResult TransactionCommit();
	void TransactionRollback();

	// A transaction scope: begun on construction, ended by Commit, and rolled back by the destructor when the scope
	// is left any other way, so an early return never leaves a transaction open. Scopes nest as the calls do.
	class Transaction {
	public:
		explicit Transaction(DBcore &db) : m_db(db) { m_db.TransactionBegin(); }
		~Transaction()
		{
			if (m_open) {
				m_db.TransactionRollback();
			}
		}
		Transaction(const Transaction &) = delete;
		Transaction &operator=(const Transaction &) = delete;

		// False when the commit failed or an inner scope rolled back.
		bool Commit()
		{
			m_open = false;
			return m_db.TransactionCommit().Success();
		}

		void Rollback()
		{
			m_open = false;
			m_db.TransactionRollback();
		}

	private:
		DBcore &m_db;
		bool    m_open{true};
	};
	std::string Escape(const std::string& s);
	uint32 DoEscapeString(char *tobuf, const char *frombuf, uint32 fromlen);
	void ping();

	const std::string &GetOriginHost() const;
	void SetOriginHost(const std::string &origin_host);

	bool DoesTableExist(const std::string& table_name);

	void SetMySQL(const DBcore &o)
	{
		mysql      = o.mysql;
		mysqlOwner = false;
	}
	void SetMutex(Mutex *mutex);

	// only safe on connections shared with other threads if results buffered
	// unsafe to use off main thread due to internal server logging
	// throws std::runtime_error on failure
	mysql::PreparedStmt Prepare(std::string query);

protected:
	bool Open(
		const char *iHost,
		const char *iUser,
		const char *iPassword,
		const char *iDatabase,
		uint32 iPort,
		uint32 *errnum = 0,
		char *errbuf = 0,
		bool iCompress = false,
		bool iSSL = false
	);

private:
	bool Open(uint32 *errnum = nullptr, char *errbuf = nullptr);

	MYSQL*  mysql;
	int     m_transaction_depth{0};
	bool    m_transaction_failed{false};
	bool    mysqlOwner;
	Mutex   *m_mutex;
	eStatus pStatus;

	std::mutex m_query_lock{};

	std::string origin_host;

	char   *pHost;
	char   *pUser;
	char   *pPassword;
	char   *pDatabase;
	bool   pCompress;
	uint32 pPort;
	bool   pSSL;

	// allows multiple queries to be executed within the same query
	// do not use this under normal operation
	// we use this during database migrations only currently
	void SetMultiStatementsOn()
	{
		mysql_set_server_option(mysql, MYSQL_OPTION_MULTI_STATEMENTS_ON);
	}

	// disables multiple statements to be executed in one query
	void SetMultiStatementsOff()
	{
		mysql_set_server_option(mysql, MYSQL_OPTION_MULTI_STATEMENTS_OFF);
	}
};


#endif
