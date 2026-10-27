#include "DisplayConfig.hpp"

#include <cstdio>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void Check(bool condition, const char* text, int line) {
	if (condition) return;
	std::fprintf(stderr, "FAIL %s:%d %s\n", "display_config_test.cpp", line, text);
	++g_failures;
}

#define CHECK(condition) Check(static_cast<bool>(condition), #condition, __LINE__)

struct MemoryFileOps {
	std::map<std::string, std::string> files;
	std::vector<std::string> readPaths;
	int writes = 0;
	bool failRead = false;
	bool failWrite = false;
	bool failTmpRename = false;
	bool failRemoveBackup = false;
	std::string lastDirectory;

	bool Exists(const std::string& path) const {
		return files.contains(path);
	}

	fpslocker::ReadResult ReadFile(const std::string& path, std::string& out) {
		readPaths.push_back(path);
		if (failRead) return fpslocker::ReadResult::IoError;
		const auto it = files.find(path);
		if (it == files.end()) return fpslocker::ReadResult::NotFound;
		out = it->second;
		return fpslocker::ReadResult::Ok;
	}

	bool WriteFile(const std::string& path, std::string_view data) {
		++writes;
		if (failWrite) return false;
		files[path] = std::string(data);
		return true;
	}

	bool Rename(const std::string& from, const std::string& to) {
		if (failTmpRename && from.ends_with(".tmp")) return false;
		if (files.contains(to)) return false;
		const auto it = files.find(from);
		if (it == files.end()) return false;
		files.emplace(to, it->second);
		files.erase(it);
		return true;
	}

	bool Remove(const std::string& path) {
		if (failRemoveBackup && path.ends_with(".bak")) return false;
		files.erase(path);
		return true;
	}

	bool CreateDirectories(const std::string& directory) {
		lastDirectory = directory;
		return true;
	}
};

const char* kRich =
	"# keep\n"
	"[Vendor]\n"
	"name=A B\n"
	"refreshRateAllowed=leave\n"
	"[Common]\n"
	"tvName=My TV\n"
	"refreshRateAllowed={50,60,70}\n"
	"custom=leave me\n"
	"refreshRateAllowed720p={40,120}\n"
	"preferredRefreshRate=90\n"
	"preferredRefreshRate720p=45\n"
	"displayDisclaimerAcknowledged=True\n"
	"allowPatchesToForce60InDocked=False\n"
	"matchLowestRefreshRate=True\n"
	"bringDefaultRefreshRateWhenOutOfFocus=True\n";

const char* kRichExpected =
	"[Common]\n"
	"tvName=My TV\n"
	"refreshRateAllowed={60}\n"
	"custom=leave me\n"
	"refreshRateAllowed720p={40,120}\n"
	"preferredRefreshRate=90\n"
	"preferredRefreshRate720p=45\n"
	"displayDisclaimerAcknowledged=True\n"
	"allowPatchesToForce60InDocked=True\n"
	"matchLowestRefreshRate=False\n"
	"bringDefaultRefreshRateWhenOutOfFocus=False\n"
	"# keep\n"
	"[Vendor]\n"
	"name=A B\n"
	"refreshRateAllowed=leave\n";

std::span<const uint8_t> Rates() {
	return std::span<const uint8_t>(fpslocker::kDockedRefreshRates, std::size(fpslocker::kDockedRefreshRates));
}

void TestParsing() {
	CHECK(fpslocker::ParsePreferredRefreshRate("120") == 120);
	CHECK(fpslocker::ParsePreferredRefreshRate("40") == 40);
	CHECK(fpslocker::ParsePreferredRefreshRate("060") == 60);
	CHECK(!fpslocker::ParsePreferredRefreshRate("").has_value());
	CHECK(!fpslocker::ParsePreferredRefreshRate("0").has_value());
	CHECK(!fpslocker::ParsePreferredRefreshRate("121").has_value());
	CHECK(!fpslocker::ParsePreferredRefreshRate("120.0").has_value());
	CHECK(!fpslocker::ParsePreferredRefreshRate("120abc").has_value());
	CHECK(!fpslocker::ParsePreferredRefreshRate("+120").has_value());
	CHECK(!fpslocker::ParsePreferredRefreshRate("120 ").has_value());
	CHECK(!fpslocker::ParsePreferredRefreshRate(" 120").has_value());
	CHECK(fpslocker::IsDisclaimerAcknowledgedValue("True"));
	CHECK(fpslocker::IsDisclaimerAcknowledgedValue("true"));
	CHECK(fpslocker::IsDisclaimerAcknowledgedValue("TRUE"));
	CHECK(!fpslocker::IsDisclaimerAcknowledgedValue("True "));
	CHECK(!fpslocker::IsDisclaimerAcknowledgedValue("TrueX"));
	CHECK(!fpslocker::IsDisclaimerAcknowledgedValue("Yes"));
	CHECK(fpslocker::DockedDisplayIniPath(0) == "sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/00000000.ini");
	CHECK(fpslocker::DockedDisplayIniPath(0xA1B2C3D4u) == "sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/A1B2C3D4.ini");
	CHECK(std::string(fpslocker::kHandheldDisplayIniPath) == "sdmc:/SaltySD/plugins/FPSLocker/display.ini");
}

void TestAllowlistPreservation() {
	MemoryFileOps ops;
	const std::string path = "sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/00000001.ini";
	ops.files[path] = kRich;
	bool enabled[14] = {};
	for (size_t i = 0; i < 14; ++i) enabled[i] = fpslocker::kDockedRefreshRates[i] == 60;
	fpslocker::DockedPolicy policy;
	const fpslocker::ConfigSaveStatus status = fpslocker::SaveDockedAllowlistAndPolicy(
		ops, path, "Should Not Replace", false, Rates(), std::span<const bool>(enabled, 14), policy);
	CHECK(status == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.files[path] == kRichExpected);
	CHECK(!ops.files.contains(path + ".tmp"));
	CHECK(!ops.files.contains(path + ".bak"));
	CHECK(ops.lastDirectory == "sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays");
}

void TestOtherResolutionAndEmptyList() {
	MemoryFileOps ops;
	const std::string path = "monitor.ini";
	ops.files[path] =
		"[Common]\n"
		"tvName = My TV\n"
		"refreshRateAllowed={40,120}\n"
		"note=a=b\n"
		"refreshRateAllowed720p={50}\n";
	bool enabled[14] = {};
	const fpslocker::ConfigSaveStatus status = fpslocker::SaveDockedAllowlistAndPolicy(
		ops, path, "Unknown", true, Rates(), std::span<const bool>(enabled, 14), fpslocker::DockedPolicy{});
	CHECK(status == fpslocker::ConfigSaveStatus::Saved);
	const std::string& saved = ops.files[path];
	CHECK(saved.find("tvName = My TV\n") != std::string::npos);
	CHECK(saved.find("refreshRateAllowed={40,120}\n") != std::string::npos);
	CHECK(saved.find("note=a=b\n") != std::string::npos);
	CHECK(saved.find("refreshRateAllowed720p={}\n") != std::string::npos);
	CHECK(saved.find("refreshRateAllowed720p=}") == std::string::npos);
	CHECK(saved.find("preferredRefreshRate=") == std::string::npos);
}

void TestFailedReadDoesNotOverwrite() {
	MemoryFileOps ops;
	const std::string path = "monitor.ini";
	ops.files[path] = "preferredRefreshRate=120\ncustom=stay\n";
	ops.failRead = true;
	const std::string before = ops.files[path];
	const fpslocker::ConfigSaveStatus status = fpslocker::SavePreferredRefreshRate(ops, path, false, 100);
	CHECK(status == fpslocker::ConfigSaveStatus::ReadFailed);
	CHECK(ops.writes == 0);
	CHECK(ops.files[path] == before);
	CHECK(!ops.files.contains(path + ".tmp"));
}

void TestRenameRestoresPrevious() {
	MemoryFileOps ops;
	const std::string path = "monitor.ini";
	ops.files[path] = "[Common]\npreferredRefreshRate=40\nkeep=yes\n";
	ops.failTmpRename = true;
	const fpslocker::ConfigSaveStatus status = fpslocker::SavePreferredRefreshRate(ops, path, false, 120);
	CHECK(status == fpslocker::ConfigSaveStatus::RenameFailed);
	CHECK(ops.files[path] == "[Common]\npreferredRefreshRate=40\nkeep=yes\n");
	CHECK(!ops.files.contains(path + ".tmp"));
	CHECK(!ops.files.contains(path + ".bak"));
}

void TestOrphanBackupRecovered() {
	MemoryFileOps ops;
	const std::string path = "monitor.ini";
	ops.files[path + ".bak"] = "[Common]\ntvName=Keep Me\npreferredRefreshRate=40\n";
	const fpslocker::ConfigSaveStatus status = fpslocker::SavePreferredRefreshRate(ops, path, false, 120);
	CHECK(status == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.files[path] == "[Common]\ntvName=Keep Me\npreferredRefreshRate=120\n");
	CHECK(!ops.files.contains(path + ".bak"));
}

void TestDuplicateOwnedKeys() {
	MemoryFileOps ops;
	const std::string path = "monitor.ini";
	ops.files[path] = "[Common]\npreferredRefreshRate=40\npreferredRefreshRate=50\n";
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 120) == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.files[path] == "[Common]\npreferredRefreshRate=120\npreferredRefreshRate=120\n");
}

void TestRejectedRateDoesNotCreateFile() {
	MemoryFileOps ops;
	CHECK(fpslocker::SavePreferredRefreshRate(ops, "new.ini", false, 0) == fpslocker::ConfigSaveStatus::Rejected);
	CHECK(fpslocker::SavePreferredRefreshRate(ops, "new.ini", false, 121) == fpslocker::ConfigSaveStatus::Rejected);
	CHECK(ops.files.empty());
	CHECK(ops.writes == 0);
}

void TestAcknowledgementFiles() {
	MemoryFileOps ops;
	const std::string docked = fpslocker::DockedDisplayIniPath(7);
	const std::string handheld = fpslocker::kHandheldDisplayIniPath;
	ops.files[handheld] = "[Common]\ndisplayDisclaimerAcknowledged=True\npreferredRefreshRate=120\n";
	CHECK(!fpslocker::WarningAlreadyAccepted(ops, fpslocker::DisplayContext::Docked, false, docked, handheld));
	CHECK(ops.readPaths.empty());
	CHECK(!fpslocker::WarningAlreadyAccepted(ops, fpslocker::DisplayContext::Unknown, true, docked, handheld));
	CHECK(ops.readPaths.empty());

	ops.readPaths.clear();
	CHECK(!fpslocker::WarningAlreadyAccepted(ops, fpslocker::DisplayContext::Handheld, true, docked, "missing-handheld.ini"));
	CHECK(ops.readPaths.size() == 1);
	CHECK(ops.readPaths[0] == "missing-handheld.ini");

	ops.files[docked] = "[Extra]\nkeep=1\n[Common]\npreferredRefreshRate720p=50\n";
	CHECK(fpslocker::SaveDisplayAcknowledgement(ops, docked) == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.files[docked].find("[Common]\n") < ops.files[docked].find("[Extra]\n"));
	CHECK(ops.files[docked].find("preferredRefreshRate720p=50\n") != std::string::npos);
	CHECK(ops.files[docked].find("keep=1\n") != std::string::npos);
	CHECK(ops.files[docked].find("displayDisclaimerAcknowledged=True\n") != std::string::npos);
	CHECK(fpslocker::WarningAlreadyAccepted(ops, fpslocker::DisplayContext::Docked, true, docked, handheld));
	CHECK(!fpslocker::WarningAlreadyAccepted(ops, fpslocker::DisplayContext::Handheld, false, docked, "missing-handheld.ini"));
	const std::string once = ops.files[docked];
	CHECK(fpslocker::SaveDisplayAcknowledgement(ops, docked) == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.files[docked] == once);
}

void TestCommonPreambleAndSizeLimit() {
	MemoryFileOps ops;
	const std::string path = "monitor.ini";
	ops.files[path] = "\n; preserved comment\n[Common]\ntvName=Living Room TV\n[Vendor]\nkey=untouched\n";
	CHECK(fpslocker::SaveDisplayAcknowledgement(ops, path) == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.files[path].starts_with("[Common]\n"));
	CHECK(ops.files[path].find("\n; preserved comment\n") != std::string::npos);
	CHECK(ops.files[path].find("tvName=Living Room TV\n") != std::string::npos);
	CHECK(ops.files[path].find("[Vendor]\nkey=untouched\n") != std::string::npos);

	// A no-growth update at the exact shared reader limit must remain usable.
	std::string boundary = "[Common]\npreferredRefreshRate=120\n#";
	boundary.append(fpslocker::kMaxDisplayIniBytes - boundary.size() - 1, 'x');
	boundary += '\n';
	ops.files[path] = boundary;
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 100) == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.files[path].size() == fpslocker::kMaxDisplayIniBytes);
	const std::string saved = ops.files[path];
	const int writes = ops.writes;
	CHECK(fpslocker::SaveDisplayAcknowledgement(ops, path) == fpslocker::ConfigSaveStatus::WriteFailed);
	CHECK(ops.files[path] == saved);
	CHECK(ops.writes == writes);
	CHECK(!ops.files.contains(path + ".tmp"));

	// Even a permissive I/O adapter must not allow oversized originals to be rewritten.
	ops.files[path] = boundary + "x";
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 90) == fpslocker::ConfigSaveStatus::ReadFailed);
	CHECK(ops.files[path] == boundary + "x");
	CHECK(ops.writes == writes);
	ops.files[path] = "[Common]\ndisplayDisclaimerAcknowledged=True\n" + boundary;
	CHECK(!fpslocker::ReadDisplayAcknowledgement(ops, path));
}

void TestWarningActions() {
	int saves = 0;
	auto save = [&saves] {
		++saves;
		return true;
	};
	CHECK(!fpslocker::WarningActionSavesAcknowledgement(fpslocker::WarningAction::Decline));
	CHECK(!fpslocker::WarningActionSavesAcknowledgement(fpslocker::WarningAction::Back));
	CHECK(fpslocker::WarningActionSavesAcknowledgement(fpslocker::WarningAction::Accept));
	CHECK(fpslocker::ApplyWarningAction(fpslocker::WarningAction::Decline, save) == fpslocker::WarningResult::Dismissed);
	CHECK(fpslocker::ApplyWarningAction(fpslocker::WarningAction::Back, save) == fpslocker::WarningResult::Dismissed);
	CHECK(saves == 0);
	CHECK(fpslocker::ApplyWarningAction(fpslocker::WarningAction::Accept, save) == fpslocker::WarningResult::Saved);
	CHECK(saves == 1);
	CHECK(fpslocker::ApplyWarningAction(fpslocker::WarningAction::Accept, [] { return false; }) == fpslocker::WarningResult::SaveFailed);
}

fpslocker::RateCommitResult CommitWith(
	uint8_t requested,
	bool allowlisted,
	fpslocker::DisplayIdentity before,
	bool setOk,
	bool getOk,
	uint8_t actual,
	fpslocker::DisplayIdentity after,
	bool saveOk,
	int* sets,
	int* gets,
	int* identities,
	int* saves) {
	return fpslocker::CommitManualDockedRefreshRate(
		requested,
		allowlisted,
		before,
		[&](uint8_t) {
			++*sets;
			return setOk;
		},
		[&](uint8_t* out) {
			++*gets;
			if (!getOk || out == nullptr) return false;
			*out = actual;
			return true;
		},
		[&] {
			++*identities;
			return after;
		},
		[&](uint8_t, bool, uint32_t) {
			++*saves;
			return saveOk;
		});
}

void TestManualCommit() {
	fpslocker::DisplayIdentity monitor;
	monitor.available = true;
	monitor.edidCrc = 0x11u;
	monitor.height = 1080;
	int sets = 0;
	int gets = 0;
	int identities = 0;
	int saves = 0;
	fpslocker::RateCommitResult result = CommitWith(120, false, monitor, true, true, 120, monitor, true, &sets, &gets, &identities, &saves);
	CHECK(result.failure == fpslocker::RateCommitFailure::Disallowed);
	CHECK(!result.showRequestedRate);
	CHECK(sets == 0);

	sets = gets = identities = saves = 0;
	result = CommitWith(120, true, monitor, false, true, 120, monitor, true, &sets, &gets, &identities, &saves);
	CHECK(result.failure == fpslocker::RateCommitFailure::IpcSetFailed);
	CHECK(!result.showRequestedRate);
	CHECK(sets == 1);
	CHECK(gets == 0);
	CHECK(saves == 0);

	sets = gets = identities = saves = 0;
	result = CommitWith(120, true, monitor, true, true, 60, monitor, true, &sets, &gets, &identities, &saves);
	CHECK(result.failure == fpslocker::RateCommitFailure::ReadbackMismatch);
	CHECK(!result.showRequestedRate);
	CHECK(gets == 1);
	CHECK(identities == 0);
	CHECK(saves == 0);

	fpslocker::DisplayIdentity other = monitor;
	other.edidCrc = 0x22u;
	sets = gets = identities = saves = 0;
	result = CommitWith(120, true, monitor, true, true, 120, other, true, &sets, &gets, &identities, &saves);
	CHECK(result.failure == fpslocker::RateCommitFailure::IdentityMismatch);
	CHECK(!result.showRequestedRate);
	CHECK(!result.saved);
	CHECK(saves == 0);

	fpslocker::DisplayIdentity shortMode = monitor;
	shortMode.height = 480;
	sets = gets = identities = saves = 0;
	result = CommitWith(60, true, shortMode, true, true, 60, shortMode, true, &sets, &gets, &identities, &saves);
	CHECK(result.failure == fpslocker::RateCommitFailure::IdentityMismatch);
	CHECK(!result.showRequestedRate);
	CHECK(sets == 0);
	CHECK(saves == 0);

	fpslocker::DisplayIdentity missing = monitor;
	missing.available = false;
	sets = gets = identities = saves = 0;
	result = CommitWith(120, true, missing, true, true, 120, monitor, true, &sets, &gets, &identities, &saves);
	CHECK(result.failure == fpslocker::RateCommitFailure::IdentityMismatch);
	CHECK(sets == 0 && gets == 0 && saves == 0);

	other = monitor;
	other.height = 720;
	result = CommitWith(120, true, monitor, true, true, 120, other, true, &sets, &gets, &identities, &saves);
	CHECK(result.failure == fpslocker::RateCommitFailure::IdentityMismatch);
	CHECK(saves == 0);

	sets = gets = identities = saves = 0;
	result = CommitWith(100, true, monitor, true, true, 100, monitor, false, &sets, &gets, &identities, &saves);
	CHECK(result.failure == fpslocker::RateCommitFailure::SaveFailed);
	CHECK(result.showRequestedRate);
	CHECK(!result.saved);
	CHECK(saves == 1);

	MemoryFileOps ops;
	const std::string path = fpslocker::DockedDisplayIniPath(monitor.edidCrc);
	ops.files[path] = "[Common]\npreferredRefreshRate720p=45\ncustom=stay\n";
	bool saved720 = false;
	uint32_t savedCrc = 0;
	result = fpslocker::CommitManualDockedRefreshRate(
		100,
		true,
		monitor,
		[](uint8_t) { return true; },
		[](uint8_t* out) {
			*out = 100;
			return true;
		},
		[&] { return monitor; },
		[&](uint8_t rate, bool is720p, uint32_t crc) {
			saved720 = is720p;
			savedCrc = crc;
			return fpslocker::SavePreferredRefreshRate(ops, path, is720p, rate) == fpslocker::ConfigSaveStatus::Saved;
		});
	CHECK(result.saved);
	CHECK(result.showRequestedRate);
	CHECK(result.failure == fpslocker::RateCommitFailure::None);
	CHECK(!saved720);
	CHECK(savedCrc == monitor.edidCrc);
	CHECK(ops.files[path].find("preferredRefreshRate=100\n") != std::string::npos);
	CHECK(ops.files[path].find("preferredRefreshRate720p=45\n") != std::string::npos);
	CHECK(ops.files[path].find("custom=stay\n") != std::string::npos);

	monitor.height = 720;
	result = fpslocker::CommitManualDockedRefreshRate(
		50,
		true,
		monitor,
		[](uint8_t) { return true; },
		[](uint8_t* out) {
			*out = 50;
			return true;
		},
		[&] { return monitor; },
		[&](uint8_t rate, bool is720p, uint32_t crc) {
			return fpslocker::SavePreferredRefreshRate(ops, fpslocker::DockedDisplayIniPath(crc), is720p, rate) == fpslocker::ConfigSaveStatus::Saved;
		});
	CHECK(result.saved);
	const std::string& handheldMode = ops.files[fpslocker::DockedDisplayIniPath(monitor.edidCrc)];
	CHECK(handheldMode.find("preferredRefreshRate720p=50\n") != std::string::npos);
	CHECK(handheldMode.find("preferredRefreshRate=100\n") != std::string::npos);
}

void TestPosixCheckedIo() {
	const std::string root = "/tmp/opencode/fpslocker-dcfg-work";
	fpslocker::PosixFileOps ops;
	CHECK(ops.CreateDirectories(root));
	const std::string path = root + "/keep.ini";
	const std::string original = "[Common]\ncustom=keep\n";
	CHECK(ops.WriteFile(path, original));
	ops.fault = fpslocker::IoFault::Flush;
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 120) == fpslocker::ConfigSaveStatus::WriteFailed);
	std::string read;
	ops.fault = fpslocker::IoFault::None;
	CHECK(ops.ReadFile(path, read) == fpslocker::ReadResult::Ok);
	CHECK(read == original);
	CHECK(!ops.Exists(path + ".tmp"));
	CHECK(!ops.Exists(path + ".bak"));

	ops.fault = fpslocker::IoFault::Fsync;
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 120) == fpslocker::ConfigSaveStatus::WriteFailed);
	ops.fault = fpslocker::IoFault::None;
	CHECK(ops.ReadFile(path, read) == fpslocker::ReadResult::Ok);
	CHECK(read == original);

	ops.fault = fpslocker::IoFault::Close;
	const auto descriptorCount = [] {
		return std::distance(std::filesystem::directory_iterator("/proc/self/fd"), std::filesystem::directory_iterator{});
	};
	const auto descriptors = descriptorCount();
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 120) == fpslocker::ConfigSaveStatus::WriteFailed);
	CHECK(descriptorCount() == descriptors);
	ops.fault = fpslocker::IoFault::ShortWrite;
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 120) == fpslocker::ConfigSaveStatus::WriteFailed);
	ops.fault = fpslocker::IoFault::Write;
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 120) == fpslocker::ConfigSaveStatus::WriteFailed);
	ops.fault = fpslocker::IoFault::None;
	CHECK(ops.ReadFile(path, read) == fpslocker::ReadResult::Ok);
	CHECK(read == original);
	CHECK(!ops.Exists(path + ".tmp"));

	const std::string other = root + "/other.ini";
	CHECK(ops.WriteFile(other, "B"));
	CHECK(!ops.Rename(path, other));
	CHECK(ops.ReadFile(other, read) == fpslocker::ReadResult::Ok);
	CHECK(read == "B");

	const std::string nested = root + "/nested/dir/file.ini";
	CHECK(fpslocker::SaveDisplayAcknowledgement(ops, nested) == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.ReadFile(nested, read) == fpslocker::ReadResult::Ok);
	CHECK(read == "[Common]\ndisplayDisclaimerAcknowledged=True\n");
	CHECK(fpslocker::ReadDisplayAcknowledgement(ops, nested));
	CHECK(fpslocker::SavePreferredRefreshRate(ops, path, false, 90) == fpslocker::ConfigSaveStatus::Saved);
	CHECK(ops.ReadFile(path, read) == fpslocker::ReadResult::Ok);
	CHECK(read.find("custom=keep\n") != std::string::npos);
	CHECK(read.find("preferredRefreshRate=90\n") != std::string::npos);
	CHECK(!ops.Exists(path + ".tmp"));
	CHECK(!ops.Exists(path + ".bak"));

	const std::string largePath = root + "/oversized.ini";
	const std::string limit(fpslocker::kMaxDisplayIniBytes, '#');
	CHECK(ops.WriteFile(largePath, limit));
	CHECK(ops.ReadFile(largePath, read) == fpslocker::ReadResult::Ok);
	CHECK(read == limit);
	CHECK(ops.WriteFile(largePath, limit + "x"));
	CHECK(ops.ReadFile(largePath, read) == fpslocker::ReadResult::IoError);
	CHECK(fpslocker::SaveDisplayAcknowledgement(ops, largePath) == fpslocker::ConfigSaveStatus::ReadFailed);
	CHECK(std::filesystem::file_size(largePath) == limit.size() + 1);
	CHECK(!ops.Exists(largePath + ".tmp"));
}

}

int main() {
	TestParsing();
	TestAllowlistPreservation();
	TestOtherResolutionAndEmptyList();
	TestFailedReadDoesNotOverwrite();
	TestRenameRestoresPrevious();
	TestOrphanBackupRecovered();
	TestDuplicateOwnedKeys();
	TestRejectedRateDoesNotCreateFile();
	TestAcknowledgementFiles();
	TestCommonPreambleAndSizeLimit();
	TestWarningActions();
	TestManualCommit();
	TestPosixCheckedIo();
	if (g_failures != 0) {
		std::fprintf(stderr, "%d checks failed\n", g_failures);
		return 1;
	}
	std::printf("fpslocker display config tests passed\n");
	return 0;
}
