#include <opencv2/highgui.hpp>

#include "event_handling.hpp"
#include "opencv_utility.hpp"

int WaitEvent(bool is_debugging)
{
	static bool is_selecting = false;
	static int selected_image = 0;

	static bool should_wait = false;
	while (should_wait)
	{
		int key_code = cv::waitKey(0);

		if (key_code == VK_ESCAPE)
		{
			return EventType::Exit;
		}

		if (is_debugging)
		{
			if (key_code == 0x68) // 'h' ASCII code
			{
				return EventType::ShowHelp;
			}
			if (key_code == 0x69) // 'i' ASCII code
			{
				return EventType::ShowImageInfo;
			}

			if (key_code == 0x6C) // 'l' ASCII code
			{
				return EventType::ListImages;
			}
			if (key_code == 0x70) // 'p' ASCII code
			{
				return EventType::PrevImage;
			}
			if (key_code == 0x6E) // 'n' ASCII code
			{
				return EventType::NextImage;
			}

			if (key_code == 0x73) // 's' ASCII code
			{
				is_selecting = !is_selecting;
				selected_image = 0;

				return is_selecting ? EventType::BeginSelectImage : EventType::CancelSelectImage;
			}
			if (is_selecting && 0x30 <= key_code && key_code <= 0x39) // '0' to '9' ASCII code
			{
				selected_image *= 10;
				selected_image += key_code - 0x30;

				if (selected_image > 65535)
				{
					selected_image = 65535;
				}

				return selected_image << 16 | EventType::ContinueSelectImage;
			}
			if (is_selecting && key_code == VK_RETURN)
			{
				is_selecting = false;

				return selected_image << 16 | EventType::EndSelectImage;
			}

			if (key_code == 0x61) // 'a' ASCII code
			{
				return EventType::AdjustImage;
			}
			if (key_code == 0x2B) // '+' ASCII code
			{
				return 0x00010000 | EventType::AdjustImage;
			}
			if (key_code == 0x2D) // '-' ASCII code
			{
				return int(0xFFFF0000) | EventType::AdjustImage;
			}
		}
	}

	should_wait = true;
	return -1;
}

void ProcessEvent(bool is_debugging, int const & in_event, std::vector<Image> const & in_img_array, Image & out_img, std::vector<double> & out_params)
{
	static int img_idx = 0;
	static int param_idx = 0;

	auto event = short(in_event);
	auto param = short(in_event >> 16);

	ConsoleClear();

	if (!is_debugging)
	{
		ConsoleOut("Image loaded!", 0);
		event = EventType::ShowImageInfo;
	}

	switch (event)
	{
	case EventType::ShowHelp:
	{
		break;
	}
	case EventType::ShowImageInfo:
	{
		auto const & img = in_img_array[img_idx];
		ConsoleOut("Number of channels: " + std::to_string(img.Data().channels()), 1);
		ConsoleOut("Type of channels: " + std::to_string(img.Data().type()), 2);
		ConsoleOut("Depth of channels: " + std::to_string(img.Data().depth()), 3);
		break;
	}

	case EventType::ListImages:
	{
		ConsoleOut("Listing images: ", 1);
		int line = 1;
		for (auto const & img : in_img_array)
		{
			ConsoleOut(std::to_string(line) + " -> " + img.Name(), line + 1);
			++line;
		}
		break;
	}
	case EventType::PrevImage:
	{	
		img_idx = PositiveModulo(img_idx - 1, int(in_img_array.size()));
		break;
	}
	case EventType::NextImage:
	{
		img_idx = PositiveModulo(img_idx + 1, int(in_img_array.size()));
		break;
	}

	case EventType::BeginSelectImage:
	{
		ConsoleOut("Selecting image: 0", 1);
		break;
	}
	case EventType::ContinueSelectImage:
	{
		ConsoleOut("Selecting image: " + std::to_string(param), 1);
		break;
	}
	case EventType::CancelSelectImage:
	{
		break;
	}
	case EventType::EndSelectImage:
	{
		img_idx = event - 1;
		img_idx *= int(img_idx < int(in_img_array.size()));
		break;
	}

	case EventType::AdjustImage:
	{
		int val = param;
		if (val == 0)
		{
			param_idx = PositiveModulo(param_idx + 1, int(out_params.size()));
		}
		else
		{
			out_params[param_idx] += val;
		}
		ConsoleOut("Adjusting image parameter " + std::to_string(param_idx) + ": " + std::to_string(out_params[param_idx]), 1);
		break;
	}
	default:
	{
		img_idx = 0;
		break;
	}
	}

	out_img = in_img_array[img_idx];

	if (is_debugging)
	{
		ConsoleOut(std::to_string(img_idx + 1) + " -> " + out_img.Name());
	}
}

void ConsoleClear()
{
	HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO buf_info;
	GetConsoleScreenBufferInfo(output_handle, &buf_info);

	DWORD _n_chars;
	FillConsoleOutputCharacterA(output_handle, ' ', buf_info.dwSize.X * buf_info.dwSize.Y, { 0, 0 }, &_n_chars);
	
	SetConsoleCursorPosition(output_handle, { 0,0 });
}

void ConsoleOut(std::string const & in_string, int const & in_line)
{
	HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO buf_info;
	GetConsoleScreenBufferInfo(output_handle, &buf_info);

	DWORD _n_chars;
	WriteConsoleOutputCharacterA(output_handle, in_string.c_str(), DWORD(in_string.size()), { 0,SHORT(in_line) }, &_n_chars);
	
	SetConsoleCursorPosition(output_handle, { 0, std::max(buf_info.dwCursorPosition.Y, SHORT(in_line + 1)) });
}
