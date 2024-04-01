#include "isekai/common/file_util.h"

#include <filesystem>
#include <fstream>

#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"

namespace isekai {

std::string FileJoinPath(absl::string_view path1, absl::string_view path2) {
  if (path1.empty()) return std::string(path2);
  if (path2.empty()) return std::string(path1);
  if (path1.back() == '/') {
    if (path2.front() == '/') {
      // If path1 is "path1/", while path2 is "/path2", then we should remove
      // "/" from path2.
      return absl::StrCat(path1, absl::ClippedSubstr(path2, 1));
    }
  } else {
    // if path1 is "path1", while path2 is "path2", then we should add "/" in
    // the middle.
    if (path2.front() != '/') {
      return absl::StrCat(path1, "/", path2);
    }
  }
  return absl::StrCat(path1, path2);
}

absl::Status WriteStringToFile(const absl::string_view file_path,
                               const absl::string_view file_content) {
  // If the dirname of the file path does not exist, create it first.
  std::filesystem::path dir_name =
      std::filesystem::path(file_path).parent_path();
  if (!std::filesystem::exists(dir_name) &&
      !std::filesystem::create_directories(dir_name)) {
    return absl::InternalError(
        absl::StrCat("Fail to create dir: ", dir_name.c_str()));
  }

  std::ofstream output_file(file_path.data());
  output_file << file_content;
  output_file.close();
  return absl::OkStatus();
}

}  // namespace isekai
