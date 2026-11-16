#include "site_login_check.h"
#include "../common/http/httplib.h"
#include "../common/json/json.h"

#include <fmt/format.h>

#include <regex>
#include <sstream>
#include <stdexcept>

namespace {
	constexpr time_t check_timeout_seconds = 15;
}

SiteLoginVerdict CheckLoginWithSite(
	const std::string &check_url,
	const std::string &token,
	const std::string &username,
	const std::string &password,
	const std::string &player_address
)
{
#ifndef CPPHTTPLIB_OPENSSL_SUPPORT
	throw std::runtime_error("this login server was built without OpenSSL 1.1.1 or later, and the site check needs https");
#else
	static const std::regex url_pattern("^(https://[^/]+)(/.*)$");
	std::smatch             url_parts;
	if (!std::regex_match(check_url, url_parts, url_pattern)) {
		throw std::runtime_error("site_login_check.check_url is not an https address");
	}

	httplib::Client client(url_parts[1].str());
	client.enable_server_certificate_verification(true);
	client.set_connection_timeout(check_timeout_seconds);
	client.set_read_timeout(check_timeout_seconds);
	client.set_write_timeout(check_timeout_seconds);

	Json::Value request;
	request["username"]      = username;
	request["password"]      = password;
	request["playerAddress"] = player_address;
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";

	const httplib::Headers headers = {{"Authorization", "Bearer " + token}};
	auto response = client.Post(url_parts[2].str(), headers, Json::writeString(writer, request), "application/json");
	if (!response) {
		throw std::runtime_error(fmt::format("the site could not be reached ({})", httplib::to_string(response.error())));
	}

	Json::Value             answer;
	Json::CharReaderBuilder reader;
	std::string             parse_errors;
	std::istringstream      body(response->body);
	if (!Json::parseFromStream(reader, body, &answer, &parse_errors) || !answer.isObject()) {
		throw std::runtime_error(fmt::format("the site's answer (HTTP {}) is not a JSON object", response->status));
	}
	if (response->status == 200 && answer["accountName"].isString()) {
		return {true, answer["accountName"].asString(), ""};
	}
	if (response->status != 200 && answer["error"].isString()) {
		return {false, "", answer["error"].asString()};
	}
	throw std::runtime_error(fmt::format("the site answered HTTP {} without an account or a reason", response->status));
#endif
}
