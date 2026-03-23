#include "update/Esp32FirmwareInstaller.h"

#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <mbedtls/sha256.h>

#include "update/ChecksumFileParser.h"

namespace lamp::update {

extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");

namespace {

bool fetchTextWithTls(const std::string& url, std::string& payload, std::string& error) {
  WiFiClientSecure client;
  client.setCACertBundle(rootca_crt_bundle_start);

  HTTPClient http;
  if (!http.begin(client, url.c_str())) {
    error = "http-begin-failed";
    return false;
  }

  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    error = "http-" + std::to_string(httpCode);
    http.end();
    return false;
  }

  payload = http.getString().c_str();
  http.end();
  return true;
}

std::string toHexDigest(const unsigned char* digest, size_t length) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string encoded;
  encoded.reserve(length * 2);

  for (size_t index = 0; index < length; ++index) {
    encoded.push_back(kHex[(digest[index] >> 4) & 0x0F]);
    encoded.push_back(kHex[digest[index] & 0x0F]);
  }

  return encoded;
}

}  // namespace

bool Esp32FirmwareInstaller::install(const FirmwareReleaseInfo& release, std::string& error) {
  error.clear();

  if (release.assetUrl.empty() || release.assetName.empty()) {
    error = "missing-asset-url";
    return false;
  }

  if (release.checksumUrl.empty()) {
    error = "missing-checksum-url";
    return false;
  }

  if (WiFi.status() != WL_CONNECTED) {
    error = "wifi-disconnected";
    return false;
  }

  std::string checksumPayload;
  if (!fetchTextWithTls(release.checksumUrl, checksumPayload, error)) {
    return false;
  }

  const ParsedChecksumFile parsedChecksum = parseChecksumFile(checksumPayload, release.assetName);
  if (!parsedChecksum.valid) {
    error = parsedChecksum.error;
    return false;
  }

  WiFiClientSecure client;
  client.setCACertBundle(rootca_crt_bundle_start);

  HTTPClient http;
  if (!http.begin(client, release.assetUrl.c_str())) {
    error = "http-begin-failed";
    return false;
  }

  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    error = "http-" + std::to_string(httpCode);
    http.end();
    return false;
  }

  mbedtls_sha256_context sha256Context;
  mbedtls_sha256_init(&sha256Context);
  mbedtls_sha256_starts_ret(&sha256Context, 0);

  const int contentLength = http.getSize();
  if (!Update.begin(contentLength > 0 ? static_cast<size_t>(contentLength) : UPDATE_SIZE_UNKNOWN)) {
    error = "ota-begin-failed";
    mbedtls_sha256_free(&sha256Context);
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buffer[1024];
  size_t written = 0;
  while (http.connected() && (contentLength < 0 || written < static_cast<size_t>(contentLength))) {
    const size_t available = stream->available();
    if (available == 0) {
      delay(1);
      continue;
    }

    const int read = stream->readBytes(buffer, available > sizeof(buffer) ? sizeof(buffer) : available);
    if (read <= 0) {
      break;
    }

    mbedtls_sha256_update_ret(&sha256Context, buffer, static_cast<size_t>(read));
    if (Update.write(buffer, static_cast<size_t>(read)) != static_cast<size_t>(read)) {
      error = "ota-write-failed";
      Update.abort();
      mbedtls_sha256_free(&sha256Context);
      http.end();
      return false;
    }
    written += static_cast<size_t>(read);
  }

  unsigned char digest[32];
  mbedtls_sha256_finish_ret(&sha256Context, digest);
  mbedtls_sha256_free(&sha256Context);

  if (contentLength > 0 && written != static_cast<size_t>(contentLength)) {
    error = "ota-write-incomplete";
    Update.abort();
    http.end();
    return false;
  }

  if (toHexDigest(digest, sizeof(digest)) != parsedChecksum.sha256) {
    error = "checksum-mismatch";
    Update.abort();
    http.end();
    return false;
  }

  if (!Update.end()) {
    error = "ota-end-failed";
    http.end();
    return false;
  }

  if (!Update.isFinished()) {
    error = "ota-not-finished";
    http.end();
    return false;
  }

  http.end();
  return true;
}

}  // namespace lamp::update