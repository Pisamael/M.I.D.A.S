#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include "rknn_api.h"

#define UART_PORT "/dev/ttyS3"
#define FRAME_SIZE 320
#define NUM_ANCHORS 2100 // YOLOv8 320x320 grid size

float pan_angle = 90.0f;
float tilt_angle = 90.0f;
const float KP_PAN = 0.05f;
const float KP_TILT = 0.05f;
const int LOCK_THRESHOLD = 25;

int init_uart(const char* port_name) {
    int fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) return -1;
    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag |= (CLOCAL | CREAD | CS8);
    options.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;
    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

int main() {
    int uart_fd = init_uart(UART_PORT);
    if (uart_fd < 0) std::cerr << "[ERROR] UART init failed\n";

    rknn_context ctx;
    if (rknn_init(&ctx, (void*)"/oem/yolov8n.rknn", 0, 0, NULL) < 0) {
        std::cerr << "[ERROR] RKNN init failed. Ensure yolov8n.rknn is in /oem/\n"; 
        return -1;
    }

    // 2. Open Video Stream (SC3336 via local V4L2 node)
    cv::VideoCapture cap(0); // <-- Pass 0 directly into the constructor here!

    if (!cap.isOpened()) {
        std::cerr << "[ERROR] Failed to open camera. Did you run RkLunch-stop.sh?\n";
        return -1;
    }

    // Now that it is open, set the properties
    cap.set(cv::CAP_PROP_FRAME_WIDTH, FRAME_SIZE);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, FRAME_SIZE);

    cv::Mat frame, resized;
    std::cout << "[M.I.D.A.S.] Embedded C++ Turret Online. Tracking...\n";

    while (true) {
        cap >> frame;
        if (frame.empty()) continue;

        cv::resize(frame, resized, cv::Size(FRAME_SIZE, FRAME_SIZE));
        cv::cvtColor(resized, resized, cv::COLOR_BGR2RGB);

        rknn_input inputs[1];
        memset(inputs, 0, sizeof(inputs));
        inputs[0].index = 0;
        inputs[0].type = RKNN_TENSOR_UINT8;
        inputs[0].size = resized.total() * resized.channels();
        inputs[0].fmt = RKNN_TENSOR_NHWC;
        inputs[0].buf = resized.data;
        rknn_inputs_set(ctx, 1, inputs);

        rknn_run(ctx, NULL);

        rknn_output outputs[1];
        memset(outputs, 0, sizeof(outputs));
        outputs[0].want_float = 1;
        rknn_outputs_get(ctx, 1, outputs, NULL);

        float* out = (float*)outputs[0].buf;
        float max_score = 0.0f;
        int best_anchor = -1;
        
        // Target Class: 15 (Cat) / 16 (Dog) for testing, or 0 for custom monkey model
        int target_class = 15; 

        for (int i = 0; i < NUM_ANCHORS; i++) {
            float score = out[(4 + target_class) * NUM_ANCHORS + i];
            if (score > max_score) {
                max_score = score;
                best_anchor = i;
            }
        }

        int fire = 0;
        if (max_score > 0.50f && best_anchor != -1) {
            float target_x = out[0 * NUM_ANCHORS + best_anchor];
            float target_y = out[1 * NUM_ANCHORS + best_anchor];
            
            float error_x = 160.0f - target_x;
            float error_y = target_y - 160.0f; 

            if (std::abs(error_x) < LOCK_THRESHOLD && std::abs(error_y) < LOCK_THRESHOLD) {
                fire = 1;
            } else {
                pan_angle += error_x * KP_PAN;
                tilt_angle += error_y * KP_TILT;
                pan_angle = std::max(10.0f, std::min(170.0f, pan_angle));
                tilt_angle = std::max(30.0f, std::min(150.0f, tilt_angle));
            }
            std::cout << "[!] TARGET LOCKED - Score: " << max_score << "\n";
        }
        
        char cmd[64];
        sprintf(cmd, "%d,%d,%d\n", (int)pan_angle, (int)tilt_angle, fire);
        if (uart_fd >= 0) write(uart_fd, cmd, strlen(cmd));

        rknn_outputs_release(ctx, 1, outputs);
    }
    return 0;
}
