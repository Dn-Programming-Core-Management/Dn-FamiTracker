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


#pragma once

#include <string>
#include <thread>
#include <future>
#include <optional>

struct stVersionCheckResult {
	int StartUp;
	std::string VerInfo;
	std::string VerDesc;
	std::string URL;
};

class CVersionChecker {
public:
	explicit CVersionChecker(bool StartUp);
	~CVersionChecker() noexcept;

	bool IsReady() const;
	std::optional<stVersionCheckResult> GetVersionCheckResult();

private:
	static void ThreadFn(bool startup, std::promise<std::optional<stVersionCheckResult>> p) noexcept;

private:
	std::promise<std::optional<stVersionCheckResult>> promise_;
	std::future<std::optional<stVersionCheckResult>> future_ = promise_.get_future();
	std::thread th_;
};
