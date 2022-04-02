#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/video.hpp"
#include "opencv2/tracking.hpp"

#include "Droplet.h"

class MillikanTracker {
public:
	enum {
		NONE = 0x0,
		SHOW_PROCESSED = 0x1
	};

	MillikanTracker(const std::string & videoPath, const std::string & outputPath);

	void load_video(const std::string & videoPath); //either private this or have it reinitialize

	void show();

	void finish();

	template <typename TrackerType = cv::TrackerCSRT>
	void new_droplet();
	void prev_doplet();
	void next_droplet();

	void disable_tracker();
	void reset_tracker();

	bool next_frame();
	bool prev_frame();
	void beginning();

	void load_data(const std::string & dataFilepath);

	void load_keyframes(const std::string & keyframeFilepath);
	void mark_keyframe();
	void unmark_keyframe();
	bool is_keyframe();

	bool get_flag(unsigned char flag);
	void set_flag(unsigned char flag, bool value);

	size_t get_frame();

	~MillikanTracker();
private:
	void update_trackers_();
	void draw_overlay_(cv::Mat & image);
	void process_frame_();

	unsigned char flags_;

	cv::VideoCapture video_;
	cv::VideoCapture processedVideo_;
	cv::Mat currentFrame_, processedFrame_;

	cv::Ptr<cv::BackgroundSubtractor> backSub_;

	std::vector<Droplet> trackedDroplets_;
	size_t activeDropletInd_;

	std::unordered_set<int> keyframes_;

	std::string outputPath_;

	static const size_t NO_DROPLET = -1;
};

#include "MillikanTracker.ipp"