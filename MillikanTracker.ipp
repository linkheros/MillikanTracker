#ifndef MILLIKAN_TRACKER_IPP
#define MILLIKAN_TRACKER_IPP

#include "MillikanTracker.h"

template <typename TrackerType>
void MillikanTracker::new_droplet() {
	size_t prevActiveDroplet = activeDropletInd_;
	activeDropletInd_ = NO_DROPLET;

	cv::Mat overlayed = processedFrame_.clone();
	draw_overlay_(overlayed);
	cv::Rect rect = cv::selectROI("millikan", overlayed, false, true);

	if (!rect.empty()) {
		activeDropletInd_ = trackedDroplets_.size();
		trackedDroplets_.emplace_back();
		auto & activeDrop = trackedDroplets_.back();
		activeDrop.active = true;
		activeDrop.tracker = TrackerType::create();
		activeDrop.tracker->init(processedFrame_, rect);
		activeDrop.frameLastUpdated = get_frame();
		activeDrop.bbox[get_frame()] = rect;
	}
	else {
		activeDropletInd_ = prevActiveDroplet;
	}
}

#endif