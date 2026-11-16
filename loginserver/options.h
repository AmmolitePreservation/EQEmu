#ifndef EQEMU_OPTIONS_H
#define EQEMU_OPTIONS_H

class Options {
public:
	Options() :
		m_allow_unregistered(true),
		m_display_expansions(false),
		m_max_expansions_mask(0),
		m_encryption_mode(14),
		m_reject_duplicate_servers(false),
		m_allow_token_login(false),
		m_auto_create_accounts(false) {}

	inline void AllowUnregistered(bool b) { m_allow_unregistered = b; }
	inline void DisplayExpansions(bool b) { m_display_expansions = b; }
	inline void MaxExpansions(int i) { m_max_expansions_mask = i; }
	inline bool IsDisplayExpansions() const { return m_display_expansions; }
	inline int GetMaxExpansions() const { return m_max_expansions_mask; }
	inline bool IsUnregisteredAllowed() const { return m_allow_unregistered; }
	inline void EncryptionMode(int m) { m_encryption_mode = m; }
	inline int GetEncryptionMode() const { return m_encryption_mode; }
	inline void RejectDuplicateServers(bool b) { m_reject_duplicate_servers = b; }
	inline bool IsRejectingDuplicateServers() { return m_reject_duplicate_servers; }
	inline void AllowTokenLogin(bool b) { m_allow_token_login = b; }
	inline bool IsTokenLoginAllowed() const { return m_allow_token_login; }
	inline void AutoCreateAccounts(bool b) { m_auto_create_accounts = b; }
	inline bool CanAutoCreateAccounts() const { return m_auto_create_accounts; }
	inline void EQEmuLoginServerAddress(const std::string &v) { m_eqemu_loginserver_address = v; }
	inline std::string GetEQEmuLoginServerAddress() const { return m_eqemu_loginserver_address; }
	inline void DefaultLoginServerName(const std::string &v) { m_default_loginserver_name = v; }
	inline std::string GetDefaultLoginServerName() const { return m_default_loginserver_name; }
	inline void SiteLoginCheck(const std::string &url, const std::string &token)
	{
		m_site_login_check_url   = url;
		m_site_login_check_token = token;
	}
	// The ammolite site checks password logins (site_login_check.h) while its address is set.
	inline bool IsSiteLoginCheckEnabled() const { return !m_site_login_check_url.empty(); }
	inline std::string GetSiteLoginCheckURL() const { return m_site_login_check_url; }
	inline std::string GetSiteLoginCheckToken() const { return m_site_login_check_token; }
	inline bool IsShowPlayerCountEnabled() const { return m_show_player_count; }
	inline void SetShowPlayerCount(bool show_player_count) { show_player_count = show_player_count; }
	inline bool IsWorldDevTestServersListBottom() const { return m_world_dev_list_bottom; }
	inline void SetWorldDevTestServersListBottom(bool list_bottom) { m_world_dev_list_bottom = list_bottom; }
	inline bool IsWorldSpecialCharacterStartListBottom() const { return m_special_char_list_bottom; }
	inline void SetWorldSpecialCharacterStartListBottom(bool list_bottom) { m_special_char_list_bottom = list_bottom; }

private:
	bool        m_allow_unregistered;
	bool        m_display_expansions;
	bool        m_reject_duplicate_servers;
	bool        m_world_dev_list_bottom;
	bool        m_special_char_list_bottom;
	bool        m_allow_token_login;
	bool        m_show_player_count;
	bool        m_auto_create_accounts;
	int         m_encryption_mode;
	int         m_max_expansions_mask;
	std::string m_eqemu_loginserver_address;
	std::string m_default_loginserver_name;
	std::string m_site_login_check_url;
	std::string m_site_login_check_token;
};


#endif

