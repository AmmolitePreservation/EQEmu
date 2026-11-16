#ifndef EQEMU_SITE_LOGIN_CHECK_H
#define EQEMU_SITE_LOGIN_CHECK_H

#include <string>

/**
 * The ammolite site's check of a login: with "site_login_check" in login.json, a password login asks the site whether
 * the name and password a player typed are good (the player's site name with their site password, or with a login
 * key the site gave the client Ankh started) and the login server reads no password of its own for it.
 */
struct SiteLoginVerdict {
	bool        accepted;
	// The login account the site names, made by the site at the player's first good login.
	std::string account_name;
	// The site's sentence for a refused login.
	std::string refusal;
};

/**
 * Asks the site at check_url (https) with the login server's token. Throws std::runtime_error when the site cannot be
 * reached or answers anything but an account or a reason.
 */
SiteLoginVerdict CheckLoginWithSite(
	const std::string &check_url,
	const std::string &token,
	const std::string &username,
	const std::string &password,
	const std::string &player_address
);

#endif
