#include "pch.h"
#include "target/TargetOS.h"
#include "utils/CommandLine.h"
#include <utility>
#ifdef OS_WIN
#include <shellapi.h>
#endif
#include "utils/Strings.h"

constexpr size_t MAX_ARG_VAL_LEN = 100;

namespace Mana {

bool CommandLine::Parse(int argc, char* argv[]) {
  map_.clear();

  std::vector<std::string> args;

#ifdef OS_WIN
  UNREFERENCED_PARAMETER(argv);

  // if no command-line args are passed,
  // argv array will have one value containing the path to this exe.
  // Note: CommandLineToArgvW removes double-quotes around individual args.
  LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!wargv) {
    return false;
  }

  // convert args to utf8, skip the exe arg
  for (int i = 1; i < argc; ++i) {
    args.push_back(Utf16ToUtf8(wargv[i]));
  }

  LocalFree(wargv);
#else
  // TODO: do other OSes provide the path to the exe as first value? (if not,
  // change 1 to 0 here)
  for (int i = 1; i < argc; ++i) {
    args.push_back(argv[i]);
  }
#endif

  // TODO: does Mac/Linux remove the double(or single) quotes around values for
  // us?
  // TODO: how about additional leading / trailing spaces on Mac/Linux?

  bool isKey;
  char key[MAX_ARG_VAL_LEN];
  char val[MAX_ARG_VAL_LEN];

  for (size_t i = 0; i < args.size(); ++i) {
    const char* arg = args[i].c_str();
    size_t len = strnlen(arg, MAX_ARG_VAL_LEN);

    isKey = len > 2 && arg[0] == '-' && arg[1] == '-';

    if (isKey) {
      if (strncpy_s(key, MAX_ARG_VAL_LEN, &arg[2], MAX_ARG_VAL_LEN)) {
        return false;
      }

      // see if next arg is the corresponding value

      if (i + 1 >= args.size()) {
        // key is a bool (no corresponding value)
        map_[key] = "";
        break;
      }

      ++i;
      arg = args[i].c_str();
      len = strnlen(arg, MAX_ARG_VAL_LEN);
      isKey = len > 2 && arg[0] == '-' && arg[1] == '-';
      if (isKey) {
        // previous key is a bool (no corresponding value)
        map_[key] = "";
        --i;
        continue;
      }

      if (strncpy_s(val, MAX_ARG_VAL_LEN, arg, MAX_ARG_VAL_LEN)) {
        return false;
      }

      map_[key] = val;
    } else {
      // invalid args
      return false;
    }
  }

  return true;
}

//std::vector<std::string> CommandLine::GetAll() {
//  std::vector<std::string> tmp;
//
//  return tmp;
//}

bool CommandLine::HasKey(const std::string& key) {
  return map_.find(key) != map_.end();
}

// TODO: maybe return const string& instead?
std::string CommandLine::Get(const std::string& key) {
  std::string value;

  if (HasKey(key)) {
    value = map_.find(key)->second;
  }

  return std::move(value);
}

}  // namespace Mana
