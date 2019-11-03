#ifndef ARGS_PROCESSING_HEADER
#define ARGS_PROCESSING_HEADER

#include <string>
#include <unordered_map>

struct ProgramOptions
{
	static constexpr char const * GKW = "-gkw";
	static constexpr char const * GKH = "-gkh";
	static constexpr char const * GSX = "-gsx";
	static constexpr char const * GSY = "-gsy";
	static constexpr char const * BTV = "-btv";
	static constexpr char const * MKW = "-mkw";
	static constexpr char const * MKH = "-mkh";
	static constexpr char const * MNI = "-mni";
	static constexpr char const * RMS = "-rms";
	static constexpr char const * D = "-d";
	static constexpr char const * V = "-v";
	static constexpr char const * H = "-h";
	
	static constexpr char const * GKW_Ex = "--gauss-kernel-width";
	static constexpr char const * GKH_Ex = "--gauss-kernel-height";
	static constexpr char const * GSX_Ex = "--gauss-sigma-x";
	static constexpr char const * GSY_Ex = "--gauss-sigma-y";
	static constexpr char const * BTV_Ex = "--binary-threshold-value";
	static constexpr char const * MKW_Ex = "--morph-kernel-width";
	static constexpr char const * MKH_Ex = "--morph-kernel-height";
	static constexpr char const * MNI_Ex = "--morph-number-iterations";
	static constexpr char const * RMS_Ex = "--region-minimum-size";
	static constexpr char const * D_Ex = "--debug";
	static constexpr char const * V_Ex = "--version";
	static constexpr char const * H_Ex = "--help";
};

void print_version();
void print_help();

bool process_args(int argc, char ** argv, std::string & filename, bool & debug, bool & version, bool & help, std::unordered_map<std::string, std::string> & options);
template <class Value_Type>
bool process_option(std::unordered_map<std::string, std::string> const & in_options_map, char const * in_option, char const * in_option_ex, Value_Type & out_value);

#include "args_processing.tpp"

#endif
