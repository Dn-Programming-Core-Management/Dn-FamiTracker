/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "VersionChecker.h"
#pragma warning (push)
#pragma warning (disable : 4706) // assignment within conditional expression
#include "json/json.hpp"
#pragma warning (pop)
#include <vector>
#include "stdafx.h"
#include "version.h"
#include "WinInet.h"
#pragma comment(lib, "wininet.lib")

namespace {

	LPCWSTR rgpszAcceptTypes[] = { L"application/json", NULL };

	// // // TODO: cpr maybe
	struct CHttpStringReader {
		CHttpStringReader() {
			hOpen = InternetOpenW(L"Dn_FamiTracker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
			hConnect = InternetConnectW(hOpen, L"api.github.com",
				INTERNET_DEFAULT_HTTPS_PORT, L"", L"", INTERNET_SERVICE_HTTP, 0, 0);
			hRequest = HttpOpenRequestW(hConnect, L"GET", L"/repos/Gumball2415/Dn-FamiTracker/releases",
				L"HTTP/1.0", NULL, rgpszAcceptTypes,
				INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE, NULL);
		}

		~CHttpStringReader() noexcept {
			if (hRequest)
				InternetCloseHandle(hRequest);
			if (hConnect)
				InternetCloseHandle(hConnect);
			if (hOpen)
				InternetCloseHandle(hOpen);
		}

		nlohmann::json ReadJson() {
			if (!hOpen || !hConnect || !hRequest)
				return nlohmann::json{ };
			if (!HttpSendRequest(hRequest, NULL, 0, NULL, 0))
				return nlohmann::json{ };

			std::string jsonStr;
			while (true) {
				DWORD Size;
				if (!InternetQueryDataAvailable(hRequest, &Size, 0, 0))
					return nlohmann::json{ };
				if (!Size)
					break;
				std::vector<char> Buf(Size + 1);
				DWORD Received = 0;
				for (DWORD i = 0; i < Size; i += 1024) {
					DWORD Length = (Size - i < 1024) ? Size % 1024 : 1024;
					if (!InternetReadFile(hRequest, Buf.data() + i, Length, &Received))
						return nlohmann::json{ };
				}
				jsonStr += Buf.data();
			}

			return nlohmann::json::parse(jsonStr);
		}

	private:
		HINTERNET hOpen;
		HINTERNET hConnect;
		HINTERNET hRequest;
	};

	using ft_version_t = std::tuple<int, int, int, int>;

	std::pair<nlohmann::json, ft_version_t> FindBestVersion(const nlohmann::json& j) {
		ft_version_t current = { VERSION_API, VERSION_MAJ, VERSION_MIN, VERSION_REV };
		const nlohmann::json* jPtr = nullptr;

		for (const auto& i : j) {
			ft_version_t ver = { };
			auto& [api, maj, min, rev] = ver;
			const std::string& tag = i["tag_name"];
			::sscanf_s(tag.data(), "Dn%u.%u.%u%*1[.r]%u", &api, &maj, &min, &rev);
			if (ver > current) {
				current = ver;
				jPtr = &i;
			}
		}

		return std::make_pair(jPtr ? *jPtr : nlohmann::json{ }, current);
	}

} // namespace



CVersionChecker::CVersionChecker(bool StartUp) : th_{ &ThreadFn, StartUp, std::move(promise_) } {
}

CVersionChecker::~CVersionChecker() {
	if (th_.joinable())		// // //
		th_.join();
}

bool CVersionChecker::IsReady() const {
	return future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}

std::optional<stVersionCheckResult> CVersionChecker::GetVersionCheckResult() {
	return future_.get();
}

void CVersionChecker::ThreadFn(bool startup, std::promise<std::optional<stVersionCheckResult>> p) noexcept try {
	if (auto [json, verNum] = FindBestVersion(CHttpStringReader{ }.ReadJson()); !json.empty()) {
		auto [api, maj, min, rev] = verNum;
		std::string verStr = std::to_string(api) + '.' + std::to_string(maj) + '.' + std::to_string(min) + '.' + std::to_string(rev);

		std::string timeStr = json["published_at"];

		std::string desc = json["body"];

		std::string msg = "A new version of Dn-FamiTracker is now available:\n\n";
		msg += "Version " + verStr + " (released on " + timeStr + ")\n\n";
		msg += desc + "\n\n";
		msg += "Pressing \"Yes\" will launch the Github web page for this release.\n\n";
		msg += "Pressing \"No\" will disable version checking in the future.\n\n";
		if (startup)
			msg += "(Version checking on startup may be re-enabled in the configuration menu.)";
		std::string url = "https://github.com/Gumball2415/Dn-FamiTracker/releases/tag/Dn" + verStr;

		p.set_value(stVersionCheckResult{ std::move(msg), std::move(url), MB_YESNOCANCEL | MB_ICONINFORMATION });
	}
	else
		p.set_value(std::nullopt);
}
catch (...) {
	p.set_value(std::nullopt);
	p.set_value(stVersionCheckResult {
			"Unable to get version information from the source repository.", "", MB_ICONERROR});
}
