#pragma once

/*
Makes the assumption that we are always working with ascii (or utf8)
command line arg strings.
Assumes format: --{key1} {value1} --{key2} --{key3} {value3}
Values can be within double quotes.
*/

#include <map>
#include <string>
#include <vector>

namespace Mana {

class CommandLine {
 public:
  // Assumes format: --{key1} {value1} --{key2} --{key3} {value3}
  // Values can be within double quotes.
  // On Windows, we ignore the params and instead get cmdline from Windows API.
  bool Parse(int argc, char* argv[]);
  std::vector<std::string> GetAll();
  bool HasKey(std::string& key);
  std::string Get(std::string& key);

 private:
  std::map<std::string, std::string> map;
};

}  // namespace Mana
