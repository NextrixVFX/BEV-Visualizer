/*
 * @Author: Mandy
 * @Date: 2023-08-13 13:19:50
 * @Last Modified by: Mandy
 * @Last Modified time: 2023-08-13 18:51:21
 */
#include <cuda_runtime.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <numeric>
#include <opencv2/opencv.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <algorithm>

#include "fastbev/fastbev.hpp"
#include "common/check.hpp"
#include "common/tensor.hpp"
#include "common/timer.hpp"
#include "common/visualize.hpp"
#include "common/Types.hpp"
#include "server/udp.hpp"

constexpr int final_height = 256;
constexpr int final_width = 704;

class VideoLoader {
public:
    explicit VideoLoader(const std::string& video_dir) {
        // Camera order matching FastBEV's expected order
        const std::vector<std::string> camera_names = {
            "CAM_FRONT", "CAM_FRONT_RIGHT", "CAM_FRONT_LEFT",
            "CAM_BACK", "CAM_BACK_LEFT", "CAM_BACK_RIGHT"
        };

        const std::vector<std::string> video_extensions = { ".mp4", ".avi", ".mov" };

        for (const auto& cam_name : camera_names) {
            std::string video_path;
            // Try to find the video file
            for (const auto& ext : video_extensions) {
                std::string test_path = video_dir;
                test_path += '/';
                test_path += cam_name;
                test_path += ext;

                if (std::ifstream(test_path).good()) {
                    video_path = test_path;
                    break;
                }
            }

            if (video_path.empty()) {
                std::cerr << "Could not find video file for camera: " << cam_name << std::endl;
                continue;
            }

            cv::VideoCapture cap(video_path);
            if (!cap.isOpened()) {
                std::cerr << "Error opening video: " << video_path << std::endl;
                continue;
            }

            cameras_.push_back(cam_name);
            captures_.push_back(cap);
            std::cout << "Loaded video: " << video_path << std::endl;
        }

        if (cameras_.size() != 6) {
            std::cerr << "Warning: Expected 6 cameras, but loaded " << cameras_.size() << std::endl;
        }
    }

    ~VideoLoader() {
        for (auto& cap : captures_) {
            cap.release();
        }
    }

    std::vector<cv::Mat> get_next_frames() {
        std::vector<cv::Mat> frames;

        for (auto & capture : captures_) {
            cv::Mat frame;
            if (!capture.read(frame)) {
                // End of video or error
                return {};
            }
            frames.push_back(frame);
        }

        return frames;
    }

    void reset() {
        for (auto& cap : captures_) {
            cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        }
    }

    size_t get_camera_count() const { return cameras_.size(); }

private:
    std::vector<std::string> cameras_;
    std::vector<cv::VideoCapture> captures_;
};

static std::vector<unsigned char*> cv_mat_to_stb_images(const std::vector<cv::Mat>& frames) {
    std::vector<unsigned char*> images;

    for (const auto& frame : frames) {
        if (frame.empty()) {
            images.push_back(nullptr);
            continue;
        }

        // Convert BGR to RGB if needed
        cv::Mat rgb_frame;
        if (frame.channels() == 3) {
            cv::cvtColor(frame, rgb_frame, cv::COLOR_BGR2RGB);
        }
        else {
            frame.copyTo(rgb_frame);
        }

        // Allocate memory and copy data
        auto* image_data = new unsigned char[rgb_frame.total() * rgb_frame.channels()];
        memcpy(image_data, rgb_frame.data, rgb_frame.total() * rgb_frame.channels());
        images.push_back(image_data);
        //delete[] image_data;
    }

    return images;
}

static void free_images(std::vector<unsigned char*>& images) {
    for (const auto & image : images) {
         delete[] image;
    }
    images.clear();
}

std::shared_ptr<fastbev::Core> create_core(const std::string& model, const std::string& precision) {
    printf("Create by %s, %s\n", model.c_str(), precision.c_str());
    fastbev::pre::NormalizationParameter normalization;
    normalization.image_width = 1600;
    normalization.image_height = 900;
    normalization.output_width = 704;
    normalization.output_height = 256;
    normalization.num_camera = 6;
    normalization.resize_lim = 0.44f;
    normalization.interpolation = fastbev::pre::Interpolation::Nearest;

    float mean[3] = { 123.675, 116.28, 103.53 };
    float std[3] = { 58.395, 57.12, 57.375 };
    normalization.method = fastbev::pre::NormMethod::mean_std(mean, std, 1.0f, 0.0f);
    fastbev::pre::GeometryParameter geo_param{};
    geo_param.feat_height = 64;
    geo_param.feat_width = 176;
    geo_param.num_camera = 6;
    geo_param.valid_points = 160000;
    geo_param.volum_x = 200;
    geo_param.volum_y = 200;
    geo_param.volum_z = 4;

    fastbev::CoreParameter param;
    param.pre_model = nv::format("model/%s/build/fastbev_pre_trt.plan", model.c_str());
    param.normalize = normalization;
    param.post_model = nv::format("model/%s/build/fastbev_post_trt_decode.plan", model.c_str());
    param.geo_param = geo_param;
    return fastbev::create_core(param);
}

void SaveBoxPred(const std::vector<fastbev::post::transbbox::BoundingBox>& boxes, const std::string& file_name) {
    std::ofstream ofs;
    ofs.open(file_name, std::ios::out);
    if (ofs.is_open()) {
        for (const auto& box : boxes) {
            ofs << box.position.x << " ";
            ofs << box.position.y << " ";
            ofs << box.position.z << " ";
            ofs << box.size.w << " ";
            ofs << box.size.l << " ";
            ofs << box.size.h << " ";
            ofs << box.z_rotation << " ";
            ofs << box.id << " ";
            ofs << box.score << " ";
            ofs << " ";
        }
    }
    else {
        std::cerr << "Output file cannot be opened!" << std::endl;
    }
    ofs.close();
    std::cout << "Saved prediction in: " << file_name << std::endl;
}

void process_video_mode(const std::string& video_dir, const std::string& model, const std::string& precision) {
    fastbev::UDPServer udpServer(8080);
    if (!udpServer.initialize()) {
        std::cerr << "Failed to initialize UDP server" << std::endl;
        return;
    }

    std::cout << "Loading videos from: " << video_dir << std::endl;

    VideoLoader video_loader(video_dir);
    if (video_loader.get_camera_count() == 0) {
        std::cerr << "No video files loaded!" << std::endl;
        return;
    }

    auto core = create_core(model, precision);
    if (core == nullptr) {
        printf("Core initialization failed.\n");
        return;
    }

    cudaStream_t stream;
    cudaStreamCreate(&stream);

    core->print();
    core->set_timer(true);

    // Load validation tensors (you might want to adjust these paths)
    auto valid_c_idx = nv::Tensor::load("example-data/valid_c_idx.tensor", false);
    auto valid_x = nv::Tensor::load("example-data/x.tensor", false);
    auto valid_y = nv::Tensor::load("example-data/y.tensor", false);
    core->update(valid_c_idx.ptr<float>(), valid_x.ptr<int64_t>(), valid_y.ptr<int64_t>(), stream);

    int frame_count = 0;
    std::vector<double> inference_times;

    // Create output directory for frame results
    std::string output_dir = nv::format("model/%s/video_results", model.c_str());
    system(nv::format("mkdir -p %s", output_dir.c_str()).c_str());

    while (true) {
        auto frames = video_loader.get_next_frames();
        if (frames.empty()) {
            std::cout << "End of video reached." << std::endl;
            break;
        }

        // Convert OpenCV frames to STB image format
        auto images = cv_mat_to_stb_images(frames);

        if (images.size() != 6) {
            std::cerr << "Error: Expected 6 frames, got " << images.size() << std::endl;
            free_images(images);
            continue;
        }

        // Run inference
        int64 start_time = cv::getTickCount();
        std::vector<fastbev::post::transbbox::BoundingBox> bboxes = core->forward(const_cast<const unsigned char**>(images.data()), stream);
        int64 end_time = cv::getTickCount();

        double inference_time = (end_time - start_time) / cv::getTickFrequency() * 1000.0;
        inference_times.push_back(inference_time);

        // Save results for this frame
        //std::string frame_result_file = nv::format("%s/frame_%04d.txt", output_dir.c_str(), frame_count);
        //SaveBoxPred(bboxes, frame_result_file);

        udpServer.sendDetections(reinterpret_cast<std::vector<BoundingBox> &>(bboxes), "172.23.112.1", 8081);


        // Clean up
        free_images(images);

        frame_count++;
    }

    // Print statistics
    if (!inference_times.empty()) {
        double avg_time = std::accumulate(inference_times.begin(), inference_times.end(), 0.0) / inference_times.size();
        double max_time = *std::max_element(inference_times.begin(), inference_times.end());
        double min_time = *std::min_element(inference_times.begin(), inference_times.end());

        std::cout << "\n=== Inference Statistics ===" << std::endl;
        std::cout << "Total frames processed: " << frame_count << std::endl;
        std::cout << "Average inference time: " << avg_time << " ms" << std::endl;
        std::cout << "Min inference time: " << min_time << " ms" << std::endl;
        std::cout << "Max inference time: " << max_time << " ms" << std::endl;
        std::cout << "Average FPS: " << 1000.0 / avg_time << std::endl;
    }

    checkRuntime(cudaStreamDestroy(stream));
}

void process_image_mode(const std::string& image_dir, const std::string& model, const std::string& precision) {
    // Original image processing code (for backward compatibility)
    const std::string Save_Dir = nv::format("model/%s/result", model.c_str());
    const auto core = create_core(model, precision);
    if (core == nullptr) {
        printf("Core has been failed.\n");
        return;
    }

    cudaStream_t stream;
    cudaStreamCreate(&stream);

    core->print();
    core->set_timer(true);

    // Load static images (original functionality)
    const char* file_names[] = { "0-FRONT.jpg", "1-FRONT_RIGHT.jpg", "2-FRONT_LEFT.jpg",
                                "3-BACK.jpg",  "4-BACK_LEFT.jpg",   "5-BACK_RIGHT.jpg" };

    std::vector<unsigned char*> images;
    for (const auto & file_name : file_names) {
        char path[200];
        sprintf(path, "%s/%s", image_dir.c_str(), file_name);
        int width, height, channels;
        images.push_back(stbi_load(path, &width, &height, &channels, 0));
    }

    const nv::Tensor valid_c_idx = nv::Tensor::load(nv::format("%s/valid_c_idx.tensor", image_dir.c_str()), false);
    const nv::Tensor valid_x = nv::Tensor::load(nv::format("%s/x.tensor", image_dir.c_str()), false);
    const nv::Tensor valid_y = nv::Tensor::load(nv::format("%s/y.tensor", image_dir.c_str()), false);
    core->update(valid_c_idx.ptr<float>(), valid_x.ptr<int64_t>(), valid_y.ptr<int64_t>(), stream);

    // warmup
    std::vector<fastbev::post::transbbox::BoundingBox> bboxes = core->forward(const_cast<const unsigned char**>(images.data()), stream);

    // evaluate inference time
    for (int i = 0; i < 5; ++i) {
        core->forward(const_cast<const unsigned char**>(images.data()), stream);
    }

    const std::string save_file_name = Save_Dir + ".txt";
    SaveBoxPred(bboxes, save_file_name);

    // destroy memory
    for (const auto & image : images) {
        stbi_image_free(image);
    }
    checkRuntime(cudaStreamDestroy(stream));
}

int main(const int argc, char** argv) {
    // Default parameters
    auto input_path = "example-data";
    auto model = "resnet18";
    auto precision = "fp16";
    bool video_mode = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--video" || arg == "-v") {
            video_mode = true;
        }
        else if (arg == "--model" || arg == "-m") {
            if (i + 1 < argc) model = argv[++i];
        }
        else if (arg == "--precision" || arg == "-p") {
            if (i + 1 < argc) precision = argv[++i];
        }
        else {
            // Assume it's the input path
            input_path = argv[i];
        }
    }

    std::cout << "Input path: " << input_path << std::endl;
    std::cout << "Model: " << model << std::endl;
    std::cout << "Precision: " << precision << std::endl;
    std::cout << "Mode: " << (video_mode ? "Video" : "Image") << std::endl;

    if (video_mode) {
        process_video_mode(input_path, model, precision);
    }
    else {
        process_image_mode(input_path, model, precision);
    }

    return 0;
}