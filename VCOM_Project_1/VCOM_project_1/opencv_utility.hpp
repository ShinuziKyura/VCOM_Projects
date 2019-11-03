#ifndef OPENCV_UTILITY_HEADER
#define OPENCV_UTILITY_HEADER

#include <filesystem>

#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

class Image
{
public:
	Image() = default;
	Image(fs::path const & in_file, int in_flags = cv::IMREAD_COLOR)
		: image_name{ in_file.filename().string() }
	{
		if (!fs::exists(in_file))
		{
			throw std::logic_error("File does not exist!");
		}

		if (!fs::is_regular_file(in_file))
		{
			throw std::logic_error("File is not a regular file!");
		}

		image_data = cv::imread(in_file.string(), in_flags);
	}

	std::string const & Name() const noexcept
	{
		return image_name;
	}
	cv::Mat const & Data() const noexcept
	{
		return image_data;
	}

private:
	std::string image_name;
	cv::Mat image_data;

};

class ImageSnapshot
{
public:
	ImageSnapshot(cv::Mat const & in_image, size_t in_snapshot_idx)
		: image{ in_image }
		, snapshot_idx{ in_snapshot_idx }
		, current_idx{ 0 }
	{
		CheckAndTakeSnapshot();
	}

	ImageSnapshot & operator=(cv::Mat const & in_image) noexcept
	{
		++current_idx;
		image = in_image;

		CheckAndTakeSnapshot();

		return *this;
	}
	operator std::remove_const_t<std::remove_reference_t<cv::InputArray>>() const noexcept
	{
		return image;
	}

	cv::Mat const & Image() const noexcept
	{
		return image;
	}
	cv::Mat const & Snapshot() const noexcept
	{
		return snapshot;
	}

private:
	void CheckAndTakeSnapshot() noexcept
	{
		if (snapshot_idx == current_idx)
		{
			snapshot = image.clone();
		}
	}

	cv::Mat image;
	cv::Mat snapshot;
	size_t snapshot_idx;
	size_t current_idx;

};

struct ImageROI
{
	ImageROI(cv::Rect in_region, size_t in_idx)
		: region{ in_region }
		, idx{ in_idx }
		, x_response{ 0 }
		, y_response{ 0 }
	{
	}

	cv::Rect region;
	size_t idx;

	int x_response;
	int y_response;

};

class NamedWindow
{
public:
	NamedWindow(std::string const & in_window_name)
		: window_name{ in_window_name }
	{
		if (bool(cv::getWindowProperty(window_name, cv::WND_PROP_VISIBLE)))
		{
			throw std::logic_error("A window with this name already exists!");
		}

		cv::namedWindow(window_name, cv::WINDOW_KEEPRATIO);
		cv::setWindowProperty(window_name, cv::WND_PROP_ASPECT_RATIO, 16. / 9.);
	}
	~NamedWindow()
	{
		cv::destroyWindow(window_name);
	}

	operator std::string() const noexcept
	{
		return window_name;
	}

private:
	std::string const window_name;

};

inline int PositiveModulo(int const& a, int const& b)
{
	return (a % b + b) % b;
}

#endif