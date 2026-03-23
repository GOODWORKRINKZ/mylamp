#pragma once

#include <string>
#include <vector>

namespace lamp::storage {

class IFileStore {
 public:
  virtual ~IFileStore() = default;

  virtual bool writeText(const std::string& path, const std::string& content) = 0;
  virtual bool readText(const std::string& path, std::string& content) const = 0;
  virtual bool remove(const std::string& path) = 0;
  virtual std::vector<std::string> list(const std::string& prefix) const = 0;
};

}  // namespace lamp::storage