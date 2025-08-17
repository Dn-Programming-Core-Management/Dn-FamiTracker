/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/

#include "VersionChecker.h"
#pragma warning (push)
#pragma warning (disable : 4706) // assignment within conditional expression
#include "json/json.hpp"
#pragma warning (pop)
#include <vector>
#include <ctime>
#include "stdafx.h"
#include "version.h"
#include "WinInet.h"
#include <CommCtrl.h>
#pragma comment(lib, "wininet.lib")

namespace {

	LPCWSTR rgpszAcceptTypes[] = { L"application/json", NULL };

	struct CHttpStringReader {
		CHttpStringReader() {
			hOpen = InternetOpenW(L"Dn_FamiTracker", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
			hConnect = InternetConnectW(hOpen, L"api.github.com",
				INTERNET_DEFAULT_HTTPS_PORT, L"", L"", INTERNET_SERVICE_HTTP, 0, 0);
			hRequest = HttpOpenRequestW(hConnect, L"GET", L"/repos/Dn-Programming-Core-Management/Dn-FamiTracker/releases",
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
		ft_version_t current = { VERSION_API, VERSION_MAJ, VERSION_MIN, VERSION_BLD };
		const nlohmann::json* jPtr = nullptr;

		for (const auto& i : j) {
			ft_version_t ver = { };
			auto& [api, maj, min, bld] = ver;
			const std::string& tag = i["tag_name"];
			::sscanf_s(tag.data(), "Dn%u.%u.%u%*1[.r]%u", &api, &maj, &min, &bld);
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

		// convert UniversalSortableDateFormat to tm
		struct tm pubTime;
		pubTime.tm_year		= std::stoi(timeStr.substr(0, 4), nullptr, 10) - 1900;
		pubTime.tm_mon		= std::stoi(timeStr.substr(5, 2), nullptr, 10) - 1;
		pubTime.tm_mday		= std::stoi(timeStr.substr(8, 2), nullptr, 10);

		// TODO: add time since update has been released (i.e., 2 hours ago, etc.)
		//pubTime.tm_hour		= std::stoi(timeStr.substr(11, 2), nullptr, 10);
		//pubTime.tm_min		= std::stoi(timeStr.substr(14, 2), nullptr, 10);
		//pubTime.tm_sec		= std::stoi(timeStr.substr(17, 2), nullptr, 10);

		char chDate[14];
		strftime(chDate, 14, "%b %d, %Y", &pubTime);
		std::string s(chDate);

		std::string desc = json["body"];

		// replace all "* " with " - " for dialog formatting
		const std::string t("\r\n* ");
		std::string::size_type pos = 0;
		size_t count = 0;

		while ((pos = desc.find(t, pos)) != std::string::npos)
		{
			count++;
			desc.erase(pos + 1, 2);
			desc.insert(pos + 1, "\r\n - ");
			pos += t.size();
		}
		int StartUp = int(startup);
		std::string VerInfo = "Version " + verStr + " (released on " + s + ")\n\n";
		std::string url = json["html_url"];

		p.set_value(stVersionCheckResult{ std::move(StartUp), std::move(VerInfo), std::move(desc), std::move(url) });
	}
	else
		p.set_value(std::nullopt);
}
catch (...) {
	p.set_value(std::nullopt);
//	p.set_value(stVersionCheckResult {
//			"Unable to get version information from the source repository.", "", "", "", MB_ICONERROR});
}
