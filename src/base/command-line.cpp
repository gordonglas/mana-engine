#include "pch.h"
#include "base/command-line.h"
#include <shellapi.h>
#include <stdio.h> // size_t
#include <string.h> // strncpy_s
#include "base/strings.h"

constexpr size_t MAX_ARG_VAL_LEN = 100;

namespace Mana
{
	bool CommandLine::Parse(int argc, char* argv[])
	{
		map.clear();

		std::vector<std::string> args;

#ifdef OS_WIN
		UNREFERENCED_PARAMETER(argv);

		// if no command-line args are passed,
		// argv array will have one value containing the path to this exe.
		LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
		if (!wargv)
		{
			return false;
		}

		// convert args to utf8, skip the exe arg
		for (int i = 1; i < argc; ++i)
		{
			args.push_back(Utf16ToUtf8(wargv[i]));
		}

		LocalFree(wargv);
#else
		for (int i = 0; i < argc; ++i)
		{
			args.push_back(argv[i]);
		}
#endif

		// TODO: HERE!!! test this function!
		// TODO: does Windows/Mac/Linux remove the double quotes around values for us?
		// TODO: how about additional leading / trailing spaces?

		bool isKey;
		char key[MAX_ARG_VAL_LEN];
		char val[MAX_ARG_VAL_LEN];

		for (size_t i = 0; i < args.size(); ++i)
		{
			const char* arg = args[i].c_str();
			int len = strnlen(arg, MAX_ARG_VAL_LEN);

			isKey = len > 2 && arg[0] == '-' && arg[1] == '-';

			if (isKey)
			{
				if (strncpy_s(key, MAX_ARG_VAL_LEN, &arg[2], MAX_ARG_VAL_LEN))
				{
					return false;
				}

				// see if next arg is the corresponding value

				if (i + 1 >= (size_t)argc)
				{
					// key is a bool (no corresponding value)
					map[key] = "";
					break;
				}

				++i;
				arg = args[i].c_str();
				len = strnlen(arg, MAX_ARG_VAL_LEN);
				isKey = len > 2 && arg[0] == '-' && arg[1] == '-';
				if (isKey)
				{
					// previous key is a bool (no corresponding value)
					map[key] = "";
					--i;
					continue;
				}

				if (strncpy_s(val, MAX_ARG_VAL_LEN, arg, MAX_ARG_VAL_LEN))
				{
					return false;
				}

				map[key] = val;
			}
			else
			{
				// invalid args
				return false;
			}
		}

		return true;
	}

	std::vector<std::string> CommandLine::GetAll()
	{
		std::vector<std::string> tmp;

		return tmp;
	}

	bool CommandLine::HasKey(std::string& key)
	{
		return map.find(key) != map.end();
	}

	std::string CommandLine::Get(std::string& key)
	{
		std::string value;

		if (HasKey(key))
		{
			value = map.find(key)->second;
		}

		return value;
	}
}
