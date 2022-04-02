#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>
#include <set>

#include <nlohmann/json.hpp>

#include "opencv2/highgui.hpp"

#include "MillikanTracker.h"

static const std::unordered_map<std::string, std::string> controlInfo = {
    {"finish", "Finish"},
    {"pause", "Play / Pause"},
    {"keyframe", "Mark Keyframe"},
    {"remKeyframe", "Unmark Keyframe"},
    {"restart", "Restart Video"},
    {"forward", "Next Frame"},
    {"back", "Previous Frame"},
    {"view", "Toggle Processed View"},
    {"new", "New Droplet"},
    {"nextDrop", "Next Droplet"},
    {"prevDrop", "Previous Droplet"},
    {"resTracker", "Reset Tracker"},
    {"disTracker", "Disable Tracker"}
};

std::unordered_map<std::string, int> mappings;
template <typename It>
void get_mappings(It nameBeg, It nameEnd);

void calibration();
void data_collection();

int main() {
    bool running = true;
    while (running) {
        std::cout << "Please select mode:" << std::endl;
        std::cout << "\t1: Calibration" << std::endl;
        std::cout << "\t2: Data Collection" << std::endl;

        int inp;
        std::cin >> inp;
        while (std::cin.fail() || (inp < 1) || (inp > 2)) {
            std::cout << "Please enter a number between 1 and 2." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin >> inp;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        
        switch (inp) {
        case 1:
            calibration();
            break;
        case 2:
            data_collection();
            break;
        }

        char answer;
        std::cout << "Would you like to analyse another video? (y/n)" << std::endl;
        std::cin >> answer;
        while ((answer != 'y') && (answer != 'n')) {
            std::cout << "y/n only." << std::endl;
            std::cout << "Would you like to analyse another video? (y/n)" << std::endl;
            std::cin >> answer;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        running = (answer == 'y');
    }
    
    return 0;
}

template <typename It>
void get_mappings(It nameBeg, It nameEnd) {
    using nlohmann::json;
    static bool mapped = false;
    if (!mapped) {
        if (std::filesystem::exists("./config.JSON")) {
            std::ifstream configFile("./config.JSON");
            json configJSON = json::parse(configFile);
            configFile.close();

            for (auto & [mapping, value] : configJSON.items()) {
                mappings[mapping] = value;
            }
        }

        mapped = true;
    }

    std::vector<std::string> neededMappings;
    for (auto it = nameBeg; it != nameEnd; it++) {
        if (!mappings.contains(*it)) {
            neededMappings.push_back(*it);
        }
    }

    if (!neededMappings.empty()) {
        cv::namedWindow("Control Mapping");

        std::cout << "Either the config files does not exist or some controls were not mapped in the config file. The following controls must be set manually:" << std::endl;
        for (const std::string & mapping : neededMappings) {
            std::cout << "Press the \"" << controlInfo.at(mapping) << "\" Key:" << std::endl;
            mappings[mapping] = cv::waitKey();
        }
        std::cout << "Controls set!" << std::endl;

        cv::destroyWindow("Control Mapping");

        json configJSON(mappings);
        std::ofstream configFile("./config.JSON");
        configFile << std::setw(4) << configJSON << std::endl;
    }
}

void calibration() {
    std::cout << "Enter calibration video filename:" << std::endl;
    std::string videoPath;
    std::getline(std::cin, videoPath);
    while (!std::filesystem::exists(videoPath)) {
        std::cout << "Invalid filepath." << std::endl;
        std::cout << "Enter video filename:" << std::endl;
        std::getline(std::cin, videoPath);
    }

    try {
        auto stem = std::filesystem::path(videoPath).stem();

        cv::VideoCapture video;
        video.open(videoPath);

        if (!video.isOpened()) {
            throw std::runtime_error("Failed to open video: " + videoPath);
        }

        cv::Mat frame;
        video.read(frame);

        cv::Mat processedFrame = frame.clone();
        cv::imshow("Calibration", processedFrame);

        std::tuple<bool, int, int> mouseEventInfo;
        auto & [wasClicked, c_x, c_y] = mouseEventInfo;
        cv::setMouseCallback("Calibration",
            [](int event, int x, int y, int flags, void * userdata) {
                auto & [wasClicked, c_x, c_y] = *(std::tuple<bool, int, int>*)userdata;
                wasClicked = (event == cv::EVENT_LBUTTONDOWN);
                c_x = x;
                c_y = y;
            },
            &mouseEventInfo
        );

        std::cout << "Select lower rule marking." << std::endl;
        while (!wasClicked) {
            processedFrame = frame.clone();
            cv::line(processedFrame, cv::Point(0, c_y), cv::Point(processedFrame.size().width, c_y), cv::Scalar(0, 255, 0), 2);
            cv::imshow("Calibration", processedFrame);

            cv::pollKey();
        }
        int lowY = c_y;
        cv::line(frame, cv::Point(0, lowY), cv::Point(frame.size().width, lowY), cv::Scalar(255, 0, 0), 2);

        processedFrame = frame.clone();
        cv::imshow("Calibration", processedFrame);

        while (wasClicked) {
            cv::pollKey();
        }

        std::cout << "Select upper rule marking." << std::endl;
        while (!wasClicked) {
            processedFrame = frame.clone();
            cv::line(processedFrame, cv::Point(0, c_y), cv::Point(processedFrame.size().width, c_y), cv::Scalar(0, 255, 0), 2);
            cv::imshow("Calibration", processedFrame);

            cv::pollKey();
        }
        int highY = c_y;
        cv::line(frame, cv::Point(0, highY), cv::Point(frame.size().width, highY), cv::Scalar(255, 0, 0), 2);

        processedFrame = frame.clone();
        cv::imshow("Calibration", processedFrame);

        while (wasClicked) {
            cv::pollKey();
        }

        std::cout << "Specify distance in tenths of a millimeter between lower and upper marking." << std::endl;
        double scale;
        std::cin >> scale;
        while (std::cin.fail()) {
            std::cout << "Please enter valid number." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin >> scale;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::ofstream calibrationFile("./out/" + stem.string() + ".clb");
        calibrationFile << "pix\ttenths of mm\n" << abs(highY - lowY) << '\t' << fabs(scale) << std::endl;
        calibrationFile.close();
        std::cout << "Calibration info written to " << stem.string() << ".txt" << std::endl;

        cv::destroyWindow("Calibration");
    }
    catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}
void data_collection() {
    static const std::vector<std::string> reqMappings = {
        "finish",
        "pause",
        "keyframe",
        "remKeyframe",
        "restart",
        "forward",
        "back",
        "view",
        "new",
        "nextDrop",
        "prevDrop",
        "resTracker",
        "disTracker"
    };
    get_mappings(reqMappings.begin(), reqMappings.end());

    std::cout << "Enter video filename:" << std::endl;
    std::string videoPath;
    std::getline(std::cin, videoPath);
    while (!std::filesystem::exists(videoPath)) {
        std::cout << "Invalid filepath." << std::endl;
        std::cout << "Enter video filename:" << std::endl;
        std::getline(std::cin, videoPath);
    }

    try {
        auto stem = std::filesystem::path(videoPath).stem();
        MillikanTracker millikanTracker(videoPath, "./out/" + stem.string());

        if (std::filesystem::exists("./out/" + stem.string() + ".txt")) {
            millikanTracker.load_data("./out/" + stem.string() + ".txt");
        }
        if (std::filesystem::exists("./out/" + stem.string() + ".kfr")) {
            millikanTracker.load_keyframes("./out/" + stem.string() + ".kfr");
        }

        millikanTracker.show();

        bool paused = true;
        while (true) {
            millikanTracker.show();

            bool finalFrame = false;
            if (paused) {
                int keyCode = cv::waitKey();
                if (keyCode == mappings["forward"]) {
                    finalFrame = !millikanTracker.next_frame();
                }
                else if (keyCode == mappings.at("back")) {
                    millikanTracker.prev_frame();
                }
                else if (keyCode == mappings.at("view")) {
                    millikanTracker.set_flag(MillikanTracker::SHOW_PROCESSED, !(millikanTracker.get_flag(MillikanTracker::SHOW_PROCESSED)));
                }
                else if (keyCode == mappings.at("new")) {
                    millikanTracker.new_droplet();
                }
                else if (keyCode == mappings.at("nextDrop")) {
                    millikanTracker.next_droplet();
                }
                else if (keyCode == mappings.at("prevDrop")) {
                    millikanTracker.prev_doplet();
                }
                else if (keyCode == mappings.at("resTracker")) {
                    millikanTracker.reset_tracker();
                }
                else if (keyCode == mappings.at("disTracker")) {
                    millikanTracker.disable_tracker();
                }
                else if (keyCode == mappings.at("pause")) {
                    paused = false;
                }
                else if (keyCode == mappings.at("finish")) {
                    millikanTracker.finish();
                    break;
                }
                else if (keyCode == mappings.at("restart")) {
                    millikanTracker.beginning();
                }
                else if (keyCode == mappings.at("keyframe")) {
                    millikanTracker.mark_keyframe();
                }
                else if (keyCode == mappings.at("remKeyframe")) {
                    millikanTracker.unmark_keyframe();
                }
            }
            else {
                int keyCode = cv::waitKey(5);
                if (keyCode == mappings.at("pause")) {
                    paused = true;
                }
                else if (keyCode == mappings.at("view")) {
                    millikanTracker.set_flag(MillikanTracker::SHOW_PROCESSED, !(millikanTracker.get_flag(MillikanTracker::SHOW_PROCESSED)));
                }
                else if (keyCode == mappings.at("nextDrop")) {
                    millikanTracker.next_droplet();
                }
                else if (keyCode == mappings.at("prevDrop")) {
                    millikanTracker.prev_doplet();
                }
                else if (keyCode == mappings.at("restart")) {
                    millikanTracker.beginning();
                }

                finalFrame = !millikanTracker.next_frame();

                if (millikanTracker.is_keyframe()) {
                    paused = true;
                }
            }
            
            if (finalFrame) {
                paused = true;
                millikanTracker.prev_frame();
                millikanTracker.next_frame();
            }
        }
    }
    catch (const std::exception & e) {
        std::cerr << e.what() << std::endl;
        throw;
    }
}