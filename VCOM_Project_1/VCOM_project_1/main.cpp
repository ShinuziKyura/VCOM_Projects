#include <iostream>
#include <filesystem>
#include <vector>
#include <atomic>

#include <opencv2/opencv.hpp>

#include "args_processing.hpp"
#include "event_handling.hpp"
#include "opencv_utility.hpp"

int main(int argc, char ** argv)
{
	//////////////////////////
	/// Argument processing

	std::string filename;

	bool debug = false;
	bool version = false;
	bool help = false;

	std::unordered_map<std::string, std::string> options;

	if (!process_args(argc, argv, filename, debug, version, help, options))
	{
		print_help();
		return 1;
	}

	/////////////////////////
	/// Options processing

	std::vector<double> params = { 
		(debug ? 2.0 : 5.0), 
		(debug ? 1.0 : 3.0), 
		(debug ? 8.0 : 0.8), 
		(debug ? 16.0 : 1.6), 
		20.0, 
		8.0, 
		2.0, 
		2.0, 
		60.0, 
		0.0, // Only used in debug-mode
		0.0, // Only used in debug-mode
	};

	// If the 'help' option was specified, or if more than one exclusive option was specified, or if no image file was specified in non-debug mode
	if (help || debug && version || !debug && filename.empty())
	{
		print_help();
		return 1;
	}

	// If the 'version' option was specified
	if (version)
	{
		print_version();
		return 1;
	}

	// If in non-debug mode, check for parameter options and override default values
	if (!(debug ||
		process_option(options, ProgramOptions::GKW, ProgramOptions::GKW_Ex, params[0]) &&
		process_option(options, ProgramOptions::GKH, ProgramOptions::GKH_Ex, params[1]) &&
		process_option(options, ProgramOptions::GSX, ProgramOptions::GSX_Ex, params[2]) &&
		process_option(options, ProgramOptions::GSY, ProgramOptions::GSY_Ex, params[3]) &&
		process_option(options, ProgramOptions::BTV, ProgramOptions::BTV_Ex, params[4]) &&
		process_option(options, ProgramOptions::MKW, ProgramOptions::MKW_Ex, params[5]) &&
		process_option(options, ProgramOptions::MKH, ProgramOptions::MKH_Ex, params[6]) &&
		process_option(options, ProgramOptions::MNI, ProgramOptions::MNI_Ex, params[7]) &&
		process_option(options, ProgramOptions::RMS, ProgramOptions::RMS_Ex, params[8])))
	{
		print_help();
		return 1;
	}

	////////////////////
	/// Load image(s)

	std::vector<Image> img_array;

	// If an image file was specified
	if (!filename.empty())
	{
		if (auto path = fs::path{ filename };
			fs::exists(path) && fs::is_regular_file(path))
		{
			img_array.emplace_back(path);
		}
	}

	// If debugging, try to automatically load images from 'debug/images/' (relative to where the program is executing)
	if (debug)
	{
		if (auto path = fs::path{ "debug/images/" };
			fs::exists(path) && fs::is_directory(path))
		{
			auto const & img_dir = fs::directory_iterator{ path };
			for (auto const & img_file : img_dir)
			{
				if (img_file.is_regular_file())
				{
					img_array.emplace_back(img_file.path());
				}
			}
		}
	}

	// If no images were specified or found
	if (img_array.empty())
	{
		print_help();
		return 1;
	}

	///////////////////////
	/// Create window(s)

	NamedWindow wnd0{ "image" };
	NamedWindow wnd1{ "ROIs" };
	NamedWindow wnd2{ "step" };
	NamedWindow wnd3{ "barcode" };

	// Destroy superfluous windows if in non-debug mode
	if (!debug)
	{
		cv::destroyWindow(wnd1);
		cv::destroyWindow(wnd2);
		cv::destroyWindow(wnd3);
	}

	///////////////////////////
	/// Execute program loop

	// Wait for keyboard event (only accepts 'Escape' key while in non-debug mode)
	while (int event = WaitEvent(debug))
	{
		Image img;

		// Process keyboard event
		ProcessEvent(debug, event, img_array, img, params); 

		// Additional processing of debug mode only parameters
		params[9] = double(PositiveModulo(int(params[9]), 8));
		params[10] = double(PositiveModulo(int(params[10]), 2));

		cv::Mat img_data = img.Data().clone();

		///////////////////////////////////////////////////
		/// First step: Find potential barcodes in image

		ImageSnapshot src_data{ img_data, size_t(params[9]) };
		cv::Mat dst_data;

		std::vector<ImageROI> image_ROIs;

		try
		{
			// Convert to grayscale

			cv::cvtColor(src_data, dst_data, cv::COLOR_BGR2GRAY);
			src_data = dst_data; // 1

			// Apply Sobel operator: second derivative in x with a kernel size of 3

			cv::Sobel(src_data, dst_data, cv::FILTER_SCHARR, 2, 0, 3);
			src_data = dst_data; // 2

			int const gauss_kernel_width = int(debug ? params[0] * 2.0 + 1.0 : params[0]);
			int const gauss_kernel_height = int(debug ? params[1] * 2.0 + 1.0 : params[1]);
			double const gauss_sigma_x = debug ? params[2] * 0.1 : params[2];
			double const gauss_sigma_y = debug ? params[3] * 0.1 : params[3];

			// Apply Gaussian blur

			cv::GaussianBlur(src_data, dst_data, cv::Size(gauss_kernel_width, gauss_kernel_height), gauss_sigma_x, gauss_sigma_y);
			src_data = dst_data; // 3

			// Convert to binary image using a simple thresholding function with specified thresholding value

			cv::threshold(src_data, dst_data, params[4], 255.0, cv::THRESH_BINARY);
			src_data = dst_data; // 4

			// Apply morphological operator: close operation with specified kernel and iterations

			cv::morphologyEx(src_data, dst_data, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT,cv::Size(int(params[5]), int(params[6]))), cv::Point(-1, -1), int(params[7]));
			src_data = dst_data; // 5

			std::vector<std::vector<cv::Point>> contours;

			// Find contours using border following algorithm

			cv::findContours(src_data, contours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

			// Convert back to BGR color space (interesting only for debug mode)

			cv::cvtColor(src_data, dst_data, cv::COLOR_GRAY2BGR);
			src_data = dst_data; // 6

			image_ROIs.reserve(contours.size());

			size_t unique_id = 0;

			for (auto const & contour : contours)
			{
				// Find bounding rectangles for the contours, and save them as ROIs

				if (auto rect = cv::boundingRect(contour); rect.height > params[8] && rect.width > rect.height)
				{
					// Trace ROIs with rectangles (interesting only for debug mode)

					cv::rectangle(dst_data, rect, cv::Scalar(0.0, 0.0, 255.0), 3);
					
					image_ROIs.emplace_back(rect, unique_id++);
				}
			}

			src_data = dst_data; // 7
		}
		catch (...) // This may happen if an invalid value was specified in some option
		{
			// We'll do a clean exit only in non-debug mode
			if (!debug)
			{
				print_help();
				return 1;
			}
		}

		bool const detected_ROIs = !image_ROIs.empty();

		if (!debug || bool(params[10]))
		{
			if (detected_ROIs)
			{
				std::cout << "ROIs detected! Analyzing regions for best response...\n";
			}
			else
			{
				std::cout << "No ROI could be detected! Try running with different parameters.\n";
			}
		}

		///////////////////////////////////////////////////////
		/// Second step: Find most likely barcode among ROIs

		cv::Mat barcode_region;

		// If no ROI was obtained, then no barcode could be detected
		if (detected_ROIs)
		{
			std::vector<ImageROI> image_ROIs_x_responses;
			std::vector<ImageROI> image_ROIs_y_responses;

			image_ROIs_x_responses.reserve(image_ROIs.size());
			image_ROIs_y_responses.reserve(image_ROIs.size());

			for (auto & ROI : image_ROIs)
			{
				cv::Mat img_ROI;

				// Convert ROI to grayscale

				cv::cvtColor(img_data(ROI.region), img_ROI, cv::COLOR_BGR2GRAY);

				cv::Mat x_gradient;
				cv::Mat y_gradient;

				// Apply Sobel operator: second derivatives both in x and y with a kernel size of 3

				cv::Sobel(img_ROI, x_gradient, cv::FILTER_SCHARR, 2, 0, 3);
				cv::Sobel(img_ROI, y_gradient, cv::FILTER_SCHARR, 0, 2, 3);

				// Accumulate response for each gradient

				std::atomic_int x_response = 0;
				std::atomic_int y_response = 0;

				x_gradient.forEach<uchar>([&x_response](auto & elem, int const *)
					{
						x_response.fetch_add(elem, std::memory_order_release);
					}
				);
				y_gradient.forEach<uchar>([&y_response](auto & elem, int const *)
					{
						y_response.fetch_add(elem, std::memory_order_release);
					}
				);

				// Normalize and save response for each gradient

				int ROI_size = ROI.region.area();

				ROI.x_response = x_response.load(std::memory_order_acquire) / ROI_size;
				ROI.y_response = y_response.load(std::memory_order_acquire) / ROI_size;

				image_ROIs_x_responses.emplace_back(ROI);
				image_ROIs_y_responses.emplace_back(ROI);
			}

			// Sort responses both in x and y, in maximizing order for x and minimizing order for y

			std::sort(std::begin(image_ROIs_x_responses), std::end(image_ROIs_x_responses), [](auto const & Elem1, auto const & Elem2) { return Elem1.x_response > Elem2.x_response; });
			std::sort(std::begin(image_ROIs_y_responses), std::end(image_ROIs_y_responses), [](auto const & Elem1, auto const & Elem2) { return Elem1.y_response < Elem2.y_response; });

			// Search for better overall response among ROIs

			auto & max_x_ROI = image_ROIs_x_responses[0];
			auto & min_y_ROI = image_ROIs_y_responses[0];

			for (size_t idx = 0; idx < image_ROIs.size(); ++idx)
			{
				if (max_x_ROI.idx == image_ROIs_y_responses[idx].idx) // Preference for a maximizing x response
				{
					barcode_region = img_data(max_x_ROI.region);
					break;
				}
				if (min_y_ROI.idx == image_ROIs_x_responses[idx].idx)
				{
					barcode_region = img_data(min_y_ROI.region);
					break;
				}
			}
		}

		bool const detected_barcode = detected_ROIs; // TODO potentially better analysis of regions in the future

		if (!debug || bool(params[10]))
		{
			if (detected_ROIs)
			{
				std::cout << "Barcode detected! Analyzing barcode characteristics...\n";
			}
			else
			{
				std::cout << "No barcode could be detected! Try running with different parameters.\n";
			}
		}

		////////////////////////////////////////////////////////////
		/// Third step: Analyze barcode in ROI with best response

		cv::Mat scan_region;

		if (detected_barcode)
		{
			constexpr int ROI_width = 2560;
			constexpr int ROI_height = 1440;

			// Adjust barcode region for processing

			// Convert region to grayscale

			cv::cvtColor(barcode_region, scan_region, cv::COLOR_BGR2GRAY);

			// Resize it to 2560x1440

			cv::resize(scan_region, scan_region, cv::Size(ROI_width, ROI_height));

			// Equalize pixel intensity

			cv::equalizeHist(scan_region, scan_region);

			// Apply morphological operator: close operation with a kernel size of 8 by 8 and 1 iteration

			cv::morphologyEx(scan_region, scan_region, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(8, 8)));

			// Convert region to binary image using a simple thresholding function with a thresholding value of 96.0

			cv::threshold(scan_region, scan_region, 96.0, 255.0, cv::THRESH_BINARY);

			// Iterate through region through a line at half-height, designated scanline

			int const barcode_scanline = barcode_region.rows / 2;
			int const ROI_scanline = scan_region.rows / 2;

			float const pixel_ratio = float(barcode_region.cols) / float(scan_region.cols);

			size_t scan_step = 0;

			int longest_bar = 0;
			int current_bar = 0;
			int current_space = 0;

			std::vector<std::pair<int, bool>> barcode_segments;

			for (int pixel_idx = 0; pixel_idx < scan_region.cols; ++pixel_idx)
			{
				// Determine if pixel is likely to belong to a bar or space based on intensity
				bool on_bar = scan_region.at<uchar>(ROI_scanline, pixel_idx) < 128;

				// Determine first pixel to paint on based on whether we switched from segment (bar or space)
				int start_paint = pixel_idx * int(on_bar != (!barcode_segments.empty() && barcode_segments.back().second));

				// Determine whether we should paint based on likely position along the region
				bool should_paint = false;
				switch (scan_step)
				{
				case 0: // Pre-start
					scan_step += int(!on_bar);
					should_paint = false;
					break;
				case 1: // Pre-delim bar
					scan_step += int(on_bar);
					should_paint = false;
					break;
				case 2: // First delim bar
					scan_step += int(!on_bar);
					should_paint = true;
					break;
				case 3: // Second delim bar
					scan_step += int(on_bar);
					should_paint = true;
					break;
				case 4: // On barcode
					longest_bar = std::max(current_bar, longest_bar);

					current_bar = on_bar ? current_bar + 1 : 0;
					current_space = !on_bar ? current_space + 1 : 0;

					// Rule to determine whether we should look for where to stop painting
					scan_step += int(pixel_idx >= scan_region.cols / 2 && current_space > longest_bar * 2);
					should_paint = true;
					break;
				case 5:
				default:
					should_paint = false;
				}

				// If it should paint and found a new segment to paint, mark it

				if (should_paint && start_paint)
				{
					barcode_segments.emplace_back(start_paint, on_bar);
				}
			}

			cv::cvtColor(scan_region, scan_region, cv::COLOR_GRAY2BGR);

			// Paint descriptive line in barcode region and report data about the barcode

			if (!debug || bool(params[10]))
			{
				std::cout << std::setprecision(2) << "Barcode description:\n";
			}

			auto const barcode_length = double(barcode_segments.back().first) - double(barcode_segments.front().first);
			for (size_t idx = 1; idx < barcode_segments.size(); ++idx)
			{
				auto const & segment_start = barcode_segments[idx - 1].first;
				auto const & segment_type = barcode_segments[idx - 1].second;
				auto const & segment_end = barcode_segments[idx].first;

				// Report segment type and width as percentage of the whole barcode
				if (!debug || bool(params[10]))
				{
					auto const percentage = (double(segment_end) - double(segment_start)) * 100.0 / barcode_length;
					std::cout << (segment_type ? "Bar:\t" : "Space:\t") << percentage << "%\n";
				}

				// Paint segment with appropriate color depending on type
				for (int pixel_idx = segment_start; pixel_idx < segment_end; ++pixel_idx)
				{
					auto & scan_top_pixel = scan_region.at<cv::Vec3b>(ROI_scanline - 1, pixel_idx);
					auto & scan_mid_pixel = scan_region.at<cv::Vec3b>(ROI_scanline, pixel_idx);
					auto & scan_bot_pixel = scan_region.at<cv::Vec3b>(ROI_scanline + 1, pixel_idx);

					auto & pixel = barcode_region.at<cv::Vec3b>(barcode_scanline, int(float(pixel_idx) * pixel_ratio));

					scan_top_pixel = scan_mid_pixel = scan_bot_pixel = pixel = segment_type ? cv::Vec3b(0, 0, 255) : cv::Vec3b(255, 0, 0);
				}
			}
		}
			
		cv::imshow(wnd0, img_data);

		if (debug)
		{
			cv::imshow(wnd1, src_data.Image());
			cv::imshow(wnd2, src_data.Snapshot());
			cv::imshow(wnd3, scan_region);
		}
		else
		{
			std::cout << "Press 'ESC' to exit.";
		}
	}
	
	return 0;
}
