#include "MillikanTracker.h"

#include <iostream>
#include <fstream>

MillikanTracker::MillikanTracker(const std::string & videoPath, const std::string & outputPath) :
    flags_(SHOW_PROCESSED),
    backSub_(cv::createBackgroundSubtractorMOG2()),
    activeDropletInd_(NO_DROPLET),
    outputPath_(outputPath)
{
    cv::namedWindow("millikan");
    load_video(videoPath);
}

void MillikanTracker::load_video(const std::string & videoPath) {
    video_.open(videoPath);

    if (!video_.isOpened()) {
        throw std::runtime_error("Failed to open video: " + videoPath);
    }

    cv::VideoWriter processedWriter;
    processedWriter.open("./temp/processed1.mp4", cv::VideoWriter::fourcc('a', 'v', 'c', '1'), video_.get(cv::CAP_PROP_FPS), cv::Size(video_.get(cv::CAP_PROP_FRAME_WIDTH), video_.get(cv::CAP_PROP_FRAME_HEIGHT)));
    while (video_.read(currentFrame_)) {
        process_frame_();
        processedWriter.write(processedFrame_);
    }
    processedWriter.release();

    video_.set(cv::CAP_PROP_POS_FRAMES, 0);
    video_.read(currentFrame_);

    processedVideo_.open("./temp/processed1.mp4");
    if (!video_.isOpened()) {
        throw std::runtime_error("Failed to open processed video: ./temp/processed1.mp4");
    }
    processedVideo_.read(processedFrame_);
}

void MillikanTracker::show() {
    cv::Mat overlayed_ = (flags_ & SHOW_PROCESSED) ? processedFrame_.clone() : currentFrame_.clone();

    draw_overlay_(overlayed_);

    cv::imshow("millikan", overlayed_);
}

void MillikanTracker::finish() {
    bool dropDataToWrite = false;
    for (size_t i = 0; (i < trackedDroplets_.size()) && !dropDataToWrite; i++) {
        dropDataToWrite = !trackedDroplets_[i].bbox.empty();
    }

    if (dropDataToWrite) {
        std::ofstream out(outputPath_ + ".txt");
        out << "drop#\tframe\tx\ty\tS_x\tS_y" << std::endl;
        for (size_t i = 0; i < trackedDroplets_.size(); i++) {
            for (auto & [frame, bbox] : trackedDroplets_[i].bbox) {
                double S_x = (double)(bbox.width) / 2.0;
                double x = bbox.x + S_x;
                double S_y = (double)(bbox.height) / 2.0;
                double y = bbox.y + S_x;
                out << i << '\t' << frame << '\t' << x << '\t' << y << '\t' << S_x << '\t' << S_y << std::endl;
            }
        }

        out.close();
    }

    if (!keyframes_.empty()) {
        std::ofstream keyframeFile(outputPath_ + ".kfr");
        keyframeFile << "keyframe" << std::endl;
        for (int keyframe : keyframes_) {
            keyframeFile << keyframe << std::endl;
        }
        keyframeFile.close();
    }
}

void MillikanTracker::prev_doplet() {
    if ((activeDropletInd_ != 0) && (activeDropletInd_ != -1)) {
        activeDropletInd_--;
    }
}
void MillikanTracker::next_droplet() {
    if (activeDropletInd_ < trackedDroplets_.size() - 1) {
        activeDropletInd_++;
    }
}

void MillikanTracker::disable_tracker() {
    Droplet & activeDrop = trackedDroplets_[activeDropletInd_];

    activeDrop.active = false;
    activeDrop.bbox.erase(get_frame());
    activeDrop.frameLastUpdated = get_frame();
}
void MillikanTracker::reset_tracker() {
    if (activeDropletInd_ != NO_DROPLET) {
        cv::Mat overlayed = processedFrame_.clone();
        draw_overlay_(overlayed);
        cv::Rect rect = cv::selectROI("millikan", overlayed, false, true);

        if (!rect.empty()) {
            auto & activeDrop = trackedDroplets_[activeDropletInd_];
            activeDrop.active = true;
            activeDrop.tracker = cv::TrackerCSRT::create();
            activeDrop.tracker->init(processedFrame_, rect);
            activeDrop.bbox[get_frame()] = rect;
            activeDrop.frameLastUpdated = get_frame();
        }
    }
}

bool MillikanTracker::next_frame() {
    if (video_.read(currentFrame_)) {
        processedVideo_.read(processedFrame_);
        update_trackers_();

        return true;
    }
    else {
        return false;
    }
}
bool MillikanTracker::prev_frame() {
    int frame = get_frame() - 2;

    if (frame >= 0) {
        video_.set(cv::CAP_PROP_POS_FRAMES, frame);
        processedVideo_.set(cv::CAP_PROP_POS_FRAMES, frame);
        return next_frame();
    }

    return false;
}
void MillikanTracker::beginning() {
    video_.set(cv::CAP_PROP_POS_FRAMES, 0);
    processedVideo_.set(cv::CAP_PROP_POS_FRAMES, 0);
    next_frame();
}

void MillikanTracker::load_data(const std::string & dataFilepath) {
    std::ifstream dataFile(dataFilepath);
    std::string line;
    std::getline(dataFile, line);
    while (getline(dataFile, line)) {
        std::stringstream linestream(line);
        std::string token;

        getline(linestream, token, '\t');
        size_t droplet = std::stoull(token);

        getline(linestream, token, '\t');
        size_t frame = std::stoull(token);

        getline(linestream, token, '\t');
        double x = std::stold(token);

        getline(linestream, token, '\t');
        double y = std::stold(token);

        getline(linestream, token, '\t');
        double S_x = std::stold(token);

        getline(linestream, token, '\t');
        double S_y = std::stold(token);

        if (trackedDroplets_.size() < droplet + 1) {
            trackedDroplets_.resize(droplet + 1);
        }
        trackedDroplets_[droplet].active = false;
        trackedDroplets_[droplet].frameLastUpdated = std::numeric_limits<size_t>::max();
        trackedDroplets_[droplet].bbox[frame] = cv::Rect(x - S_x, y - S_x, 2 * S_x, 2 * S_y);

        activeDropletInd_ = 0;
    }
    dataFile.close();
}

void MillikanTracker::load_keyframes(const std::string & keyframeFilepath) {
    std::ifstream keyframeFile(keyframeFilepath);
    std::string line;
    getline(keyframeFile, line);
    while (getline(keyframeFile, line)) {
        keyframes_.insert(std::stol(line));
    }
    keyframeFile.close();
}
void MillikanTracker::mark_keyframe() {
    keyframes_.insert(get_frame());
}
void MillikanTracker::unmark_keyframe() {
    keyframes_.erase(get_frame());
}
bool MillikanTracker::is_keyframe() {
    return keyframes_.contains(get_frame());
}

bool MillikanTracker::get_flag(unsigned char flag) {
    return flags_ & flag;
}
void MillikanTracker::set_flag(unsigned char flag, bool value) {
    if (value) {
        flags_ |= flag;
    }
    else {
        flags_ &= ~flag;
    }
}

size_t MillikanTracker::get_frame() {
    return video_.get(cv::CAP_PROP_POS_FRAMES);
}

MillikanTracker::~MillikanTracker() {
    if (video_.isOpened()) {
        video_.release();
    }
    if (processedVideo_.isOpened()) {
        processedVideo_.release();
    }

    cv::destroyWindow("millikan");
}

void MillikanTracker::update_trackers_() {
    for (size_t i = 0; i < trackedDroplets_.size(); i++) {
        auto & activeDrop = trackedDroplets_[i];

        if (activeDrop.frameLastUpdated < get_frame()) {
            if (activeDrop.active) {
                cv::Rect rect;
                if (activeDrop.tracker->update(processedFrame_, rect)) {
                    trackedDroplets_[i].bbox[get_frame()] = rect;
                }
            }
            else {
                activeDrop.bbox.erase(get_frame());
            }
            activeDrop.frameLastUpdated = get_frame();
        }
    }
}

void MillikanTracker::draw_overlay_(cv::Mat & image) {
    cv::putText(image, "frame: " + std::to_string(get_frame()), cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 255));
    cv::putText(image, "active droplet: " + ((activeDropletInd_ == -1) ? "NONE" : std::to_string(activeDropletInd_)), cv::Point(10, 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 255));
    if (keyframes_.contains(get_frame())) {
        cv::putText(image, "KEYFRAME", cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255));
    }

    for (size_t i = 0; i < trackedDroplets_.size(); i++) {
        auto & droplet = trackedDroplets_[i];
        auto it = droplet.bbox.find(get_frame());
        if (it != droplet.bbox.end()) {
            cv::putText(image, std::to_string(i), cv::Point(it->second.x - 7, it->second.y + 2), cv::FONT_HERSHEY_SIMPLEX, 0.25, cv::Scalar(255, 0, 255));
            if (i != activeDropletInd_) {
                cv::rectangle(image, it->second, cv::Scalar(255, 0, 0), 1, cv::LINE_AA);
            }
            else {
                cv::rectangle(image, it->second, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
            }
        }
    }
}

void MillikanTracker::process_frame_() {
    cv::Mat fgMask_;
    backSub_->apply(currentFrame_, fgMask_);
    cv::medianBlur(fgMask_, fgMask_, 5);

    processedFrame_.release();
    cv::bitwise_and(currentFrame_, currentFrame_, processedFrame_, fgMask_);
}