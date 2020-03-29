#include <cool/core/status.h>

namespace cool {

Status Status::Ok() { return Status{}; }

Status GenericError(const std::string &errorMsg) {
  return cool::Status{errorMsg};
}

} // namespace cool