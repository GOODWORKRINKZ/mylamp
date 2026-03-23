#include "update/GitHubReleaseParser.h"

#include <ArduinoJson.h>

#include <tuple>

namespace lamp::update {

namespace {

constexpr size_t kReleaseParserCapacity = 24 * 1024;

std::string readString(JsonVariantConst value) {
  const char* text = value.is<const char*>() ? value.as<const char*>() : nullptr;
  return text == nullptr ? std::string() : std::string(text);
}

bool isReleaseEligible(JsonObjectConst release, bool stableChannel) {
  const bool isDraft = release["draft"] | false;
  if (isDraft) {
    return false;
  }

  const bool isPrerelease = release["prerelease"] | false;
  const std::string tagName = readString(release["tag_name"]);
  if (stableChannel) {
    return !isPrerelease && !tagName.empty() && tagName[0] == 'v';
  }

  return isPrerelease && tagName.rfind("dev-", 0) == 0;
}

std::string stableAssetName(const std::string& hardwareType, const std::string& version) {
  return "mylamp-" + hardwareType + "-" + version + "-release.bin";
}

std::string devAssetPrefix(const std::string& hardwareType) {
  return "mylamp-" + hardwareType + "-dev-";
}

bool isMatchingAsset(const std::string& assetName, const std::string& version,
                     const std::string& hardwareType, bool stableChannel) {
  if (stableChannel) {
    return assetName == stableAssetName(hardwareType, version);
  }

  const std::string prefix = devAssetPrefix(hardwareType);
  return assetName.rfind(prefix, 0) == 0 &&
         assetName.size() >= 4 &&
         assetName.substr(assetName.size() - 4) == ".bin";
}

FirmwareReleaseInfo parseReleaseInfo(JsonObjectConst release, const std::string& channel) {
  FirmwareReleaseInfo info;
  info.channel = channel;
  info.version = readString(release["tag_name"]);
  info.publishedAt = readString(release["published_at"]);
  return info;
}

bool isStableChannel(const std::string& channel) {
  return channel != "dev";
}

std::string normalizeDevVersion(const std::string& version) {
  return version.rfind("dev-", 0) == 0 ? version.substr(4) : version;
}

bool tryParseSemverCore(const std::string& value, int& major, int& minor, int& patch) {
  std::string normalized = value;
  if (!normalized.empty() && normalized[0] == 'v') {
    normalized.erase(0, 1);
  }

  const size_t suffix = normalized.find('-');
  if (suffix != std::string::npos) {
    normalized = normalized.substr(0, suffix);
  }

  const size_t dot1 = normalized.find('.');
  const size_t dot2 = dot1 == std::string::npos ? std::string::npos : normalized.find('.', dot1 + 1);
  if (dot1 == std::string::npos || dot2 == std::string::npos) {
    return false;
  }

  try {
    major = std::stoi(normalized.substr(0, dot1));
    minor = std::stoi(normalized.substr(dot1 + 1, dot2 - dot1 - 1));
    patch = std::stoi(normalized.substr(dot2 + 1));
    return true;
  } catch (...) {
    return false;
  }
}

bool isNewerStableVersion(const std::string& currentVersion, const std::string& candidateVersion) {
  int currentMajor = 0;
  int currentMinor = 0;
  int currentPatch = 0;
  int candidateMajor = 0;
  int candidateMinor = 0;
  int candidatePatch = 0;

  if (!tryParseSemverCore(currentVersion, currentMajor, currentMinor, currentPatch) ||
      !tryParseSemverCore(candidateVersion, candidateMajor, candidateMinor, candidatePatch)) {
    return candidateVersion != currentVersion;
  }

  return std::tie(candidateMajor, candidateMinor, candidatePatch) >
         std::tie(currentMajor, currentMinor, currentPatch);
}

bool selectAsset(JsonObjectConst release, FirmwareReleaseInfo& info, const std::string& hardwareType,
                 bool stableChannel) {
  const JsonArrayConst assets = release["assets"].as<JsonArrayConst>();
  if (assets.isNull()) {
    info.error = "missing-hardware-asset";
    return false;
  }

  for (JsonObjectConst asset : assets) {
    const std::string assetName = readString(asset["name"]);
    if (!isMatchingAsset(assetName, info.version, hardwareType, stableChannel)) {
      continue;
    }

    info.assetName = assetName;
    info.assetUrl = readString(asset["browser_download_url"]);
    break;
  }

  if (info.assetUrl.empty()) {
    info.error = "missing-hardware-asset";
    return false;
  }

  const std::string checksumAssetName = info.assetName + ".sha256";
  for (JsonObjectConst asset : assets) {
    if (readString(asset["name"]) == checksumAssetName) {
      info.checksumUrl = readString(asset["browser_download_url"]);
      break;
    }
  }

  return true;
}

bool processCandidateRelease(JsonObjectConst release, const std::string& currentVersion,
                             const std::string& channel, const std::string& hardwareType,
                             FirmwareReleaseInfo& info) {
  const bool stableChannel = isStableChannel(channel);
  if (!isReleaseEligible(release, stableChannel)) {
    return false;
  }

  FirmwareReleaseInfo candidate = parseReleaseInfo(release, channel);
  if (candidate.version.empty()) {
    return false;
  }

  if (stableChannel) {
    if (!isNewerStableVersion(currentVersion, candidate.version)) {
      return false;
    }
  } else if (normalizeDevVersion(candidate.version) == normalizeDevVersion(currentVersion)) {
    return false;
  }

  if (!selectAsset(release, candidate, hardwareType, stableChannel)) {
    info = candidate;
    return true;
  }

  candidate.available = true;
  info = candidate;
  return true;
}

}  // namespace

FirmwareReleaseInfo GitHubReleaseParser::parse(const char* payload,
                                               const std::string& currentVersion,
                                               const std::string& channel,
                                               const std::string& hardwareType) const {
  DynamicJsonDocument document(kReleaseParserCapacity);
  FirmwareReleaseInfo info;
  info.channel = channel;

  if (payload == nullptr || deserializeJson(document, payload) != DeserializationError::Ok) {
    info.error = "invalid-payload";
    return info;
  }

  if (document.is<JsonArray>()) {
    for (JsonObjectConst release : document.as<JsonArray>()) {
      if (processCandidateRelease(release, currentVersion, channel, hardwareType, info)) {
        return info;
      }
    }
    return info;
  }

  if (document.is<JsonObject>()) {
    processCandidateRelease(document.as<JsonObject>(), currentVersion, channel, hardwareType, info);
    return info;
  }

  info.error = "invalid-payload";
  return info;
}
}  // namespace lamp::update