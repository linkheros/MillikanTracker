#pragma once

#include <unordered_map>

#include "opencv2/tracking.hpp"

struct Droplet {
public:
	Droplet() : active(false), frameLastUpdated(0) {}

	std::unordered_map<size_t, cv::Rect> bbox;

	//tracker info
	bool active;
	cv::Ptr<cv::Tracker> tracker;
	size_t frameLastUpdated;
};