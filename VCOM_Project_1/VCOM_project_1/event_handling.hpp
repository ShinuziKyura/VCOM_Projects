#ifndef EVENT_HANDLING_HEADER
#define EVENT_HANDLING_HEADER

#include <string>
#include <vector>

class Image;

struct EventType
{
	enum _event_type : int
	{
		Exit = 0,

		ShowHelp,
		ShowImageInfo,
		ShowBarcodeInfo,

		ListImages,
		PrevImage,
		NextImage,

		BeginSelectImage,
		ContinueSelectImage,
		CancelSelectImage,
		EndSelectImage,

		AdjustImage,
	};
};

int WaitEvent(bool is_debugging);
void ProcessEvent(bool is_debugging, int const & in_event, std::vector<Image> const & in_img_array, Image & out_img, std::vector<double> & out_params);

void ConsoleClear();
void ConsoleOut(std::string const & in_string, int const & in_line = 0);

#endif
