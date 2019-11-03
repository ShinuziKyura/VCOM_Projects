#include <iostream>

#include "args_processing.hpp"

void print_version()
{
	std::cout << 
R""delim(
Barcode Detector, version 1.0
Powered by OpenCV 4.1.1

Copyright (C) 2019-2019, Ricardo Santos, all rights reserved.
Third party copyrights are property of their respective owners.
)"delim" 
	<< std::endl;
}

void print_help()
{
	std::cout 
<< R"delim(
					Barcode Detector, v1.0

	Copyright (C) 2019-2019, Ricardo Santos, all rights reserved.
	Third party copyrights are property of their respective owners.
	
					Powered by OpenCV 4.1.1
	
	Usage:
		barcode_detector <file> [<options> [<value>] ...]
	
	Where:
		<file> is the absolute or relative path to a file to be processed as an image.
		<options> may be zero or more options that define how the program should run.
		<value> is the value that a particular option may or may not require.

	Possible options:
		Short name  Long name                   Type            Default
		-gkw,       --gauss-kernel-width        <odd_integer>   5
		-gkh,       --gauss-kernel-height       <odd_integer>   3
		-gsx,       --gauss-sigma-x             <decimal>       0.8
		-gsy,       --gauss-sigma-y             <decimal>       1.6
		-btv,       --binary-threshold-value    <decimal>       20.0
		-mkw,       --morph-kernel-width        <integer>       8
		-mkh,       --morph-kernel-height       <integer>       2
		-mni,       --morph-number-iterations   <integer>       2
		-rms,       --region-minimum-size       <decimal>       60.0

		-d,         --debug                     executes program in debug mode
		-v,         --version                   displays program info and version
		-h,         --help                      displays this message

		Note: The last 3 options are exclusive, meaning only one should be specified.
		      If more than one of these is specified, this message will be displayed.
		      If any other option is specified, it will be ignored.
)delim" 
	<< std::endl;
}

bool process_args(int argc, char ** argv, std::string & filename, bool & debug, bool & version, bool & help, std::unordered_map<std::string, std::string> & options)
{
	if (argc <= 1)
	{
		return false;
	}

	std::vector<std::string> args;
	args.reserve(argc);

	for (size_t idx = 1; idx < argc; ++idx)
	{
		args.emplace_back(argv[idx]);
	}

	options.reserve(argc);

	for (size_t idx = 0; idx < args.size(); ++idx)
	{
		if (args[idx] == ProgramOptions::D || args[idx] == ProgramOptions::D_Ex)
		{
			debug = true;
		}
		else if (args[idx] == ProgramOptions::V || args[idx] == ProgramOptions::V_Ex)
		{
			version = true;
		}
		else if (args[idx] == ProgramOptions::H || args[idx] == ProgramOptions::H_Ex)
		{
			help = true;
		}
		else if (args[idx].front() == '-' && !debug && !version && !help)
		{
			try
			{
				options.emplace(args[idx], args[idx + 1]);
			}
			catch (...)
			{
				return false;
			}
			++idx;
		}
		else if (filename.empty())
		{
			bool const has_quotes = args[idx].front() == '\'' || args[idx].back() == '\'';
			bool const has_double_quotes = args[idx].front() == '\"' || args[idx].back() == '\"';

			if ((has_quotes && has_double_quotes) ||
				(has_quotes && (args[idx].front() != '\'' || args[idx].back() != '\'')) ||
				(has_double_quotes && (args[idx].front() != '\"' || args[idx].back() != '\"')))
			{
				return false;
			}

			filename = has_quotes || has_double_quotes ?
				std::string(std::next(std::begin(args[idx])), std::prev(std::end(args[idx]))) :
				args[idx];
		}
		else
		{
			return false;
		}
	}

	return true;
}
