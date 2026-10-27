#pragma once

// Display preference storage for the FPSLocker 3.5.0 fork.
// Original overlay by masagrator, GNU GPL v2. See docs/FORK.md.
// This header is host-compilable and is the implementation tests call.

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace fpslocker {

inline constexpr uint8_t kDockedRefreshRates[] = {
	40, 45, 50, 55, 60, 70, 72, 75, 80, 90, 95, 100, 110, 120
};

inline constexpr const char* kHandheldDisplayIniPath = "sdmc:/SaltySD/plugins/FPSLocker/display.ini";
inline constexpr const char* kPreferredRefreshRateKey = "preferredRefreshRate";
inline constexpr const char* kPreferredRefreshRate720pKey = "preferredRefreshRate720p";
inline constexpr const char* kDisclaimerAcknowledgedKey = "displayDisclaimerAcknowledged";
inline constexpr const char* kRefreshRateAllowedKey = "refreshRateAllowed";
inline constexpr const char* kRefreshRateAllowed720pKey = "refreshRateAllowed720p";
inline constexpr const char* kTvNameKey = "tvName";
inline constexpr const char* kAllowPatchesKey = "allowPatchesToForce60InDocked";
inline constexpr const char* kMatchLowestKey = "matchLowestRefreshRate";
inline constexpr const char* kOutOfFocusKey = "bringDefaultRefreshRateWhenOutOfFocus";
inline constexpr size_t kMaxDisplayIniBytes = 16 * 1024; // Shared SaltyNX reader limit.

inline constexpr const char* PreferredRefreshRateKey(bool is720p) {
	return is720p ? kPreferredRefreshRate720pKey : kPreferredRefreshRateKey;
}

inline bool IsDockedRefreshTableRate(uint8_t rate) {
	for (uint8_t entry : kDockedRefreshRates) {
		if (entry == rate) return true;
	}
	return false;
}

inline std::optional<uint8_t> ParsePreferredRefreshRate(std::string_view text) {
	if (text.empty()) return std::nullopt;
	int value = 0;
	const char* begin = text.data();
	const char* end = begin + text.size();
	const std::from_chars_result parsed = std::from_chars(begin, end, value);
	if (parsed.ec != std::errc{} || parsed.ptr != end) return std::nullopt;
	if (value < 0 || value > 255) return std::nullopt;
	const auto rate = static_cast<uint8_t>(value);
	if (!IsDockedRefreshTableRate(rate)) return std::nullopt;
	return rate;
}

inline bool RefreshRateIsAllowlisted(uint8_t rate, std::span<const uint8_t> values, std::span<const bool> enabled) {
	const size_t count = std::min(values.size(), enabled.size());
	for (size_t i = 0; i < count; ++i) {
		if (values[i] == rate) return enabled[i];
	}
	return false;
}

inline std::string FormatRefreshAllowlist(std::span<const uint8_t> values, std::span<const bool> enabled) {
	std::string out = "{";
	bool wrote = false;
	const size_t count = std::min(values.size(), enabled.size());
	for (size_t i = 0; i < count; ++i) {
		if (!enabled[i]) continue;
		if (wrote) out.push_back(',');
		out += std::to_string(values[i]);
		wrote = true;
	}
	out.push_back('}');
	return out;
}

inline std::string DefaultRefreshAllowlist(std::span<const uint8_t> values) {
	std::string out = "{";
	bool wrote = false;
	for (uint8_t rate : values) {
		if (rate != 50 && rate != 60) continue;
		if (wrote) out.push_back(',');
		out += std::to_string(rate);
		wrote = true;
	}
	out.push_back('}');
	return out;
}

inline std::string DockedDisplayIniPath(uint32_t crc) {
	char buffer[80];
	std::snprintf(buffer, sizeof(buffer), "sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/%08" PRIX32 ".ini", crc);
	return buffer;
}

enum class ReadResult { Ok, NotFound, IoError };
enum class ConfigSaveStatus { Saved, ReadFailed, WriteFailed, RenameFailed, Rejected };
enum class IoFault { None, Write, ShortWrite, Flush, Fsync, Close, Rename };

struct PosixFileOps {
	IoFault fault = IoFault::None;

	bool Exists(const std::string& path) const {
		struct stat st {};
		return ::stat(path.c_str(), &st) == 0;
	}

	ReadResult ReadFile(const std::string& path, std::string& out) const {
		FILE* file = std::fopen(path.c_str(), "rb");
		if (!file) return errno == ENOENT ? ReadResult::NotFound : ReadResult::IoError;
		if (std::fseek(file, 0, SEEK_END) != 0) {
			std::fclose(file);
			return ReadResult::IoError;
		}
		const long size = std::ftell(file);
		if (size < 0 || size > static_cast<long>(kMaxDisplayIniBytes) || std::fseek(file, 0, SEEK_SET) != 0) {
			std::fclose(file);
			return ReadResult::IoError;
		}
		out.assign(static_cast<size_t>(size), '\0');
		if (size > 0) {
			const size_t got = std::fread(out.data(), 1, static_cast<size_t>(size), file);
			if (got != static_cast<size_t>(size)) {
				std::fclose(file);
				return ReadResult::IoError;
			}
		}
		if (std::fclose(file) != 0) return ReadResult::IoError;
		return ReadResult::Ok;
	}

	bool WriteFile(const std::string& path, std::string_view data) const {
		if (fault == IoFault::Write) return false;
		FILE* file = std::fopen(path.c_str(), "wb");
		if (!file) return false;
		if (fault == IoFault::ShortWrite) {
			std::fclose(file);
			::remove(path.c_str());
			return false;
		}
		if (!data.empty()) {
			const size_t wrote = std::fwrite(data.data(), 1, data.size(), file);
			if (wrote != data.size()) {
				std::fclose(file);
				::remove(path.c_str());
				return false;
			}
		}
		if (fault == IoFault::Flush || std::fflush(file) != 0) {
			std::fclose(file);
			::remove(path.c_str());
			return false;
		}
#if !defined(__SWITCH__)
		const int descriptor = ::fileno(file);
		if (fault == IoFault::Fsync || descriptor < 0 || ::fsync(descriptor) != 0) {
			std::fclose(file);
			::remove(path.c_str());
			return false;
		}
#endif
		const int closeResult = std::fclose(file);
		if (fault == IoFault::Close || closeResult != 0) {
			::remove(path.c_str());
			return false;
		}
		return true;
	}

	bool Rename(const std::string& from, const std::string& to) const {
		if (fault == IoFault::Rename) return false;
		if (Exists(to)) return false;
		return std::rename(from.c_str(), to.c_str()) == 0;
	}

	bool Remove(const std::string& path) const {
		if (::remove(path.c_str()) == 0 || errno == ENOENT) return true;
		return false;
	}

	bool CreateDirectories(const std::string& directory) const {
		if (directory.empty() || directory == ".") return true;
		std::string prefix;
		size_t index = 0;
		if (directory.rfind("sdmc:", 0) == 0) {
			prefix = "sdmc:";
			index = 5;
		}
		while (index < directory.size()) {
			const char ch = directory[index];
			if (ch == '/' || ch == '\\') {
				if (prefix.empty() || prefix == "sdmc:") prefix.push_back(ch);
				++index;
				continue;
			}
			size_t next = directory.find_first_of("/\\", index);
			if (next == std::string::npos) next = directory.size();
			if (!prefix.empty() && prefix.back() != '/' && prefix.back() != '\\') prefix.push_back('/');
			prefix.append(directory, index, next - index);
			index = next;
			if (prefix == "/" || prefix == "sdmc:/") continue;
			if (Exists(prefix)) continue;
			if (::mkdir(prefix.c_str(), 0777) != 0 && !Exists(prefix)) return false;
		}
		return true;
	}
};

struct KeyUpdate {
	std::string key;
	std::string value;
	bool onlyIfMissing = false;
};

struct IniLine {
	enum class Kind { Raw, Section, Key };
	Kind kind = Kind::Raw;
	std::string name;
	std::string text;
};

inline std::string_view TrimView(std::string_view text) {
	while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
	while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
	return text;
}

inline std::vector<IniLine> ParseIniLines(std::string_view input) {
	std::vector<IniLine> lines;
	size_t start = 0;
	while (start < input.size()) {
		size_t end = input.find('\n', start);
		if (end == std::string_view::npos) end = input.size();
		std::string_view raw = input.substr(start, end - start);
		if (!raw.empty() && raw.back() == '\r') raw.remove_suffix(1);
		IniLine line;
		line.text = std::string(raw);
		const std::string_view trimmed = TrimView(raw);
		if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
			line.kind = IniLine::Kind::Section;
			line.name = std::string(TrimView(trimmed.substr(1, trimmed.size() - 2)));
		} else if (!trimmed.empty() && trimmed.front() != '#' && trimmed.front() != ';') {
			const size_t eq = raw.find('=');
			if (eq != std::string_view::npos) {
				line.kind = IniLine::Kind::Key;
				line.name = std::string(TrimView(raw.substr(0, eq)));
			}
		}
		lines.push_back(std::move(line));
		if (end == input.size()) break;
		start = end + 1;
		if (start == input.size()) break;
	}
	return lines;
}

inline std::string SerializeIniLines(const std::vector<IniLine>& lines) {
	std::string out;
	for (const IniLine& line : lines) {
		out += line.text;
		out.push_back('\n');
	}
	return out;
}

inline void EnsureCommonSection(std::vector<IniLine>& lines) {
	for (const IniLine& line : lines) {
		if (line.kind == IniLine::Kind::Section && line.name == "Common") return;
	}
	size_t insertAt = 0;
	while (insertAt < lines.size() && lines[insertAt].kind != IniLine::Kind::Section) ++insertAt;
	IniLine header;
	header.kind = IniLine::Kind::Section;
	header.name = "Common";
	header.text = "[Common]";
	lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(insertAt), std::move(header));
}

inline std::pair<size_t, size_t> FirstCommonRange(const std::vector<IniLine>& lines) {
	for (size_t i = 0; i < lines.size(); ++i) {
		if (lines[i].kind != IniLine::Kind::Section || lines[i].name != "Common") continue;
		size_t end = lines.size();
		for (size_t j = i + 1; j < lines.size(); ++j) {
			if (lines[j].kind == IniLine::Kind::Section) {
				end = j;
				break;
			}
		}
		return {i, end};
	}
	return {lines.size(), lines.size()};
}

inline void EnsureCommonFirst(std::vector<IniLine>& lines) {
	const std::pair<size_t, size_t> common = FirstCommonRange(lines);
	if (common.first == lines.size() || common.first == 0) return;
	// Legacy SaltyNX checks the beginning of the file, not the first section.
	// Move the Common block before any retained preamble/comments as well.
	std::rotate(
		lines.begin(),
		lines.begin() + static_cast<std::ptrdiff_t>(common.first),
		lines.begin() + static_cast<std::ptrdiff_t>(common.second));
}

inline void ApplyCommonUpdates(std::vector<IniLine>& lines, const std::vector<KeyUpdate>& updates) {
	EnsureCommonSection(lines);
	auto range = FirstCommonRange(lines);
	for (const KeyUpdate& update : updates) {
		bool found = false;
		for (size_t i = range.first + 1; i < range.second; ++i) {
			if (lines[i].kind != IniLine::Kind::Key || lines[i].name != update.key) continue;
			found = true;
			if (!update.onlyIfMissing) {
				lines[i].text = update.key + "=" + update.value;
				lines[i].name = update.key;
			}
		}
		if (found) continue;
		IniLine added;
		added.kind = IniLine::Kind::Key;
		added.name = update.key;
		added.text = update.key + "=" + update.value;
		lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(range.second), std::move(added));
		++range.second;
	}
	EnsureCommonFirst(lines);
}

inline std::optional<std::string> FindCommonValue(const std::vector<IniLine>& lines, std::string_view key) {
	const std::pair<size_t, size_t> range = FirstCommonRange(lines);
	if (range.first == lines.size()) return std::nullopt;
	for (size_t i = range.first + 1; i < range.second; ++i) {
		if (lines[i].kind != IniLine::Kind::Key || lines[i].name != key) continue;
		const size_t eq = lines[i].text.find('=');
		if (eq == std::string::npos) return std::string();
		return lines[i].text.substr(eq + 1);
	}
	return std::nullopt;
}

inline std::string ParentDirectory(const std::string& path) {
	const size_t slash = path.find_last_of("/\\");
	if (slash == std::string::npos) return ".";
	if (slash == 0) return "/";
	return path.substr(0, slash);
}

template <class Ops>
ConfigSaveStatus UpdateCommonKeys(Ops& ops, const std::string& path, const std::vector<KeyUpdate>& updates) {
	if (!ops.CreateDirectories(ParentDirectory(path))) return ConfigSaveStatus::WriteFailed;
	const std::string temporary = path + ".tmp";
	const std::string backup = path + ".bak";
	if (!ops.Exists(path) && ops.Exists(backup)) {
		if (!ops.Rename(backup, path)) return ConfigSaveStatus::ReadFailed;
	}
	std::string original;
	std::vector<IniLine> lines;
	if (ops.Exists(path)) {
		const ReadResult read = ops.ReadFile(path, original);
		if (read != ReadResult::Ok || original.size() > kMaxDisplayIniBytes) return ConfigSaveStatus::ReadFailed;
		lines = ParseIniLines(original);
	}
	ApplyCommonUpdates(lines, updates);
	const std::string serialized = SerializeIniLines(lines);
	if (serialized.size() > kMaxDisplayIniBytes) return ConfigSaveStatus::WriteFailed;
	if (ops.Exists(temporary) && !ops.Remove(temporary)) return ConfigSaveStatus::WriteFailed;
	if (!ops.WriteFile(temporary, serialized)) return ConfigSaveStatus::WriteFailed;
	if (!ops.Exists(path)) {
		if (!ops.Rename(temporary, path)) {
			ops.Remove(temporary);
			return ConfigSaveStatus::RenameFailed;
		}
		return ConfigSaveStatus::Saved;
	}
	if (ops.Exists(backup) && !ops.Remove(backup)) {
		ops.Remove(temporary);
		return ConfigSaveStatus::RenameFailed;
	}
	if (!ops.Rename(path, backup)) {
		ops.Remove(temporary);
		return ConfigSaveStatus::RenameFailed;
	}
	if (!ops.Rename(temporary, path)) {
		ops.Rename(backup, path);
		ops.Remove(temporary);
		return ConfigSaveStatus::RenameFailed;
	}
	ops.Remove(backup);
	return ConfigSaveStatus::Saved;
}

struct DockedPolicy {
	bool dontForce60InDocked = false;
	bool fpsTargetWithoutRRMatchLowest = false;
	bool displaySyncDockedOutOfFocus60 = false;
};

inline std::string PolicyBool(bool value) {
	return value ? "True" : "False";
}

template <class Ops>
ConfigSaveStatus SaveDockedAllowlistAndPolicy(
	Ops& ops,
	const std::string& path,
	std::string_view tvNameIfMissing,
	bool is720p,
	std::span<const uint8_t> rateValues,
	std::span<const bool> enabled,
	const DockedPolicy& policy) {
	const std::string current = FormatRefreshAllowlist(rateValues, enabled);
	const std::string fallback = DefaultRefreshAllowlist(rateValues);
	std::vector<KeyUpdate> updates;
	updates.push_back(KeyUpdate{kTvNameKey, std::string(tvNameIfMissing), true});
	updates.push_back(KeyUpdate{kRefreshRateAllowedKey, is720p ? fallback : current, is720p});
	updates.push_back(KeyUpdate{kAllowPatchesKey, PolicyBool(!policy.dontForce60InDocked), false});
	updates.push_back(KeyUpdate{kMatchLowestKey, PolicyBool(policy.fpsTargetWithoutRRMatchLowest), false});
	updates.push_back(KeyUpdate{kOutOfFocusKey, PolicyBool(policy.displaySyncDockedOutOfFocus60), false});
	updates.push_back(KeyUpdate{kRefreshRateAllowed720pKey, is720p ? current : fallback, !is720p});
	return UpdateCommonKeys(ops, path, updates);
}

template <class Ops>
ConfigSaveStatus SavePreferredRefreshRate(Ops& ops, const std::string& path, bool is720p, uint8_t rate) {
	if (!IsDockedRefreshTableRate(rate)) return ConfigSaveStatus::Rejected;
	const std::vector<KeyUpdate> updates{{PreferredRefreshRateKey(is720p), std::to_string(rate), false}};
	return UpdateCommonKeys(ops, path, updates);
}

inline bool IsDisclaimerAcknowledgedValue(std::string_view value) {
	if (value.size() != 4) return false;
	auto fold = [](char ch) {
		return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch - 'A' + 'a') : ch;
	};
	return fold(value[0]) == 't' && fold(value[1]) == 'r' && fold(value[2]) == 'u' && fold(value[3]) == 'e';
}

template <class Ops>
bool ReadDisplayAcknowledgement(Ops& ops, const std::string& path) {
	std::string data;
	if (ops.ReadFile(path, data) != ReadResult::Ok || data.size() > kMaxDisplayIniBytes) return false;
	const std::optional<std::string> value = FindCommonValue(ParseIniLines(data), kDisclaimerAcknowledgedKey);
	return value.has_value() && IsDisclaimerAcknowledgedValue(*value);
}

template <class Ops>
ConfigSaveStatus SaveDisplayAcknowledgement(Ops& ops, const std::string& path) {
	const std::vector<KeyUpdate> updates{{kDisclaimerAcknowledgedKey, "True", false}};
	return UpdateCommonKeys(ops, path, updates);
}

enum class DisplayContext { Handheld, Docked, Unknown };
enum class WarningFile { None, Handheld, Docked };
enum class WarningAction { Accept, Decline, Back };
enum class WarningResult { Saved, SaveFailed, Dismissed };

inline WarningFile WarningFileForContext(DisplayContext context, bool identityAvailable) {
	if (context == DisplayContext::Unknown) return WarningFile::None;
	if (context == DisplayContext::Docked) return identityAvailable ? WarningFile::Docked : WarningFile::None;
	return WarningFile::Handheld;
}

inline bool WarningActionSavesAcknowledgement(WarningAction action) {
	return action == WarningAction::Accept;
}

template <class SaveFn>
WarningResult ApplyWarningAction(WarningAction action, SaveFn&& save) {
	if (!WarningActionSavesAcknowledgement(action)) return WarningResult::Dismissed;
	return save() ? WarningResult::Saved : WarningResult::SaveFailed;
}

template <class Ops>
bool WarningAlreadyAccepted(
	Ops& ops,
	DisplayContext context,
	bool identityAvailable,
	const std::string& dockedPath,
	const std::string& handheldPath) {
	switch (WarningFileForContext(context, identityAvailable)) {
		case WarningFile::None:
			return false;
		case WarningFile::Docked:
			return ReadDisplayAcknowledgement(ops, dockedPath);
		case WarningFile::Handheld:
			return ReadDisplayAcknowledgement(ops, handheldPath);
	}
	return false;
}

struct DisplayIdentity {
	bool available = false;
	uint32_t edidCrc = 0;
	int height = 0;
};

enum class RateCommitFailure {
	None,
	Disallowed,
	IpcSetFailed,
	ReadbackMismatch,
	IdentityMismatch,
	SaveFailed
};

struct RateCommitResult {
	bool showRequestedRate = false;
	bool saved = false;
	RateCommitFailure failure = RateCommitFailure::None;
};

template <class SetFn, class GetFn, class IdentityFn, class SaveFn>
RateCommitResult CommitManualDockedRefreshRate(
	uint8_t requested,
	bool allowlisted,
	const DisplayIdentity& before,
	SetFn&& setRate,
	GetFn&& getRate,
	IdentityFn&& readIdentity,
	SaveFn&& savePreferred) {
	RateCommitResult result;
	if (!allowlisted || !IsDockedRefreshTableRate(requested)) {
		result.failure = RateCommitFailure::Disallowed;
		return result;
	}
	if (!before.available || (before.height != 720 && before.height != 1080)) {
		result.failure = RateCommitFailure::IdentityMismatch;
		return result;
	}
	if (!setRate(requested)) {
		result.failure = RateCommitFailure::IpcSetFailed;
		return result;
	}
	uint8_t actual = 0;
	if (!getRate(&actual) || actual != requested) {
		result.failure = RateCommitFailure::ReadbackMismatch;
		return result;
	}
	const DisplayIdentity after = readIdentity();
	const bool same = before.available && after.available && before.edidCrc == after.edidCrc && before.height == after.height;
	const bool supportedResolution = before.height == 720 || before.height == 1080;
	if (!same || !supportedResolution) {
		result.failure = RateCommitFailure::IdentityMismatch;
		return result;
	}
	result.showRequestedRate = true;
	if (!savePreferred(requested, before.height == 720, before.edidCrc)) {
		result.failure = RateCommitFailure::SaveFailed;
		return result;
	}
	result.saved = true;
	return result;
}

}
