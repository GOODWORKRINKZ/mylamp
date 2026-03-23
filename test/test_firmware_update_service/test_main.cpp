#include <unity.h>

#include <string>

#include "update/FirmwareUpdateService.h"

namespace {

class FakeReleaseSource final : public lamp::update::IFirmwareReleaseSource {
 public:
  lamp::update::BuildIdentity lastBuildIdentity;
  lamp::settings::UpdateSettings lastUpdateSettings;
  lamp::update::FirmwareReleaseInfo nextResult;

  lamp::update::FirmwareReleaseInfo check(
      const lamp::update::BuildIdentity& buildIdentity,
      const lamp::settings::UpdateSettings& updateSettings) override {
    lastBuildIdentity = buildIdentity;
    lastUpdateSettings = updateSettings;
    return nextResult;
  }
};

class FakeInstaller final : public lamp::update::IFirmwareInstaller {
 public:
  bool nextResult = true;
  std::string nextError;
  lamp::update::FirmwareReleaseInfo lastRelease;
  bool called = false;

  bool install(const lamp::update::FirmwareReleaseInfo& release, std::string& error) override {
    called = true;
    lastRelease = release;
    error = nextError;
    return nextResult;
  }
};

lamp::update::BuildIdentity makeBuildIdentity() {
  return lamp::update::BuildIdentity{
      "mylamp", "v0.1.0", "stable", "esp32-c3-supermini", "c3-cylinder32x16"};
}

void test_check_returns_available_release_when_source_finds_newer_build() {
  FakeReleaseSource source;
  source.nextResult.available = true;
  source.nextResult.channel = "stable";
  source.nextResult.version = "v0.2.0";
  source.nextResult.assetName = "mylamp-c3-cylinder32x16-v0.2.0-release.bin";

  FakeInstaller installer;
  lamp::update::FirmwareUpdateService service(makeBuildIdentity(), source, installer);

  lamp::settings::UpdateSettings updateSettings;
  updateSettings.channel = "stable";
  const lamp::update::FirmwareReleaseInfo& release = service.check(updateSettings);

  TEST_ASSERT_TRUE(release.available);
  TEST_ASSERT_EQUAL_STRING("v0.2.0", release.version.c_str());
  TEST_ASSERT_EQUAL_STRING("c3-cylinder32x16", source.lastBuildIdentity.hardwareType.c_str());
  TEST_ASSERT_EQUAL_STRING("stable", source.lastUpdateSettings.channel.c_str());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::update::FirmwareUpdateState::kAvailable),
                        static_cast<int>(service.status().state));
}

void test_install_requires_available_release() {
  FakeReleaseSource source;
  FakeInstaller installer;
  lamp::update::FirmwareUpdateService service(makeBuildIdentity(), source, installer);

  std::string error;
  const bool installed = service.install(error);

  TEST_ASSERT_FALSE(installed);
  TEST_ASSERT_FALSE(installer.called);
  TEST_ASSERT_EQUAL_STRING("no-available-release", error.c_str());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::update::FirmwareUpdateState::kError),
                        static_cast<int>(service.status().state));
}

void test_install_propagates_installer_error() {
  FakeReleaseSource source;
  source.nextResult.available = true;
  source.nextResult.channel = "dev";
  source.nextResult.version = "dev-develop-a1b2c3d-20260323-204500";
  source.nextResult.assetName = "mylamp-c3-cylinder32x16-dev-develop-a1b2c3d-20260323-204500.bin";

  FakeInstaller installer;
  installer.nextResult = false;
  installer.nextError = "checksum-mismatch";

  lamp::update::FirmwareUpdateService service(makeBuildIdentity(), source, installer);

  lamp::settings::UpdateSettings updateSettings;
  updateSettings.channel = "dev";
  service.check(updateSettings);

  std::string error;
  const bool installed = service.install(error);

  TEST_ASSERT_FALSE(installed);
  TEST_ASSERT_TRUE(installer.called);
  TEST_ASSERT_EQUAL_STRING("checksum-mismatch", error.c_str());
  TEST_ASSERT_EQUAL_STRING("checksum-mismatch", service.status().error.c_str());
  TEST_ASSERT_EQUAL_STRING(
      "mylamp-c3-cylinder32x16-dev-develop-a1b2c3d-20260323-204500.bin",
      installer.lastRelease.assetName.c_str());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(lamp::update::FirmwareUpdateState::kError),
                        static_cast<int>(service.status().state));
}

}  // namespace

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_check_returns_available_release_when_source_finds_newer_build);
  RUN_TEST(test_install_requires_available_release);
  RUN_TEST(test_install_propagates_installer_error);
  return UNITY_END();
}