#include <memory>
#include <iostream>
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/int32.hpp"
#include "hidapi/hidapi.h"
#include <unistd.h>

using namespace std::chrono_literals;

enum class LED_COLORS {
    OFF, RED, GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE
};

enum class LED_PATTERNS {
    OFF, CONTINUOUS
};

class PatliteNode : public rclcpp::Node
{
public:
    PatliteNode() : Node("patlite_node")
    {
        // 初期化
        patlite_init();
        patlite_device_open();

        // プログラム開始時に緑色で点灯
        patlite_lights(LED_COLORS::GREEN, LED_PATTERNS::CONTINUOUS);

        // サブスクライバの作成: 整数メッセージを購読して色を変更
        subscription_ = this->create_subscription<std_msgs::msg::Int32>(
            "led_color", 10, std::bind(&PatliteNode::color_callback, this, std::placeholders::_1));
    }

    ~PatliteNode() {
        patlite_lights(LED_COLORS::OFF, LED_PATTERNS::CONTINUOUS);
        patlite_device_close();
    }

private:
    void color_callback(const std_msgs::msg::Int32::SharedPtr msg)
    {
        // 受信したメッセージに基づいて色を変更
        switch (msg->data) {
            case 0: patlite_lights(LED_COLORS::OFF, LED_PATTERNS::CONTINUOUS); break;
            case 1: patlite_lights(LED_COLORS::RED, LED_PATTERNS::CONTINUOUS); break;
            case 2: patlite_lights(LED_COLORS::GREEN, LED_PATTERNS::CONTINUOUS); break;
            case 3: patlite_lights(LED_COLORS::YELLOW, LED_PATTERNS::CONTINUOUS); break;
            case 4: patlite_lights(LED_COLORS::BLUE, LED_PATTERNS::CONTINUOUS); break;
            case 5: patlite_lights(LED_COLORS::PURPLE, LED_PATTERNS::CONTINUOUS); break;
            case 6: patlite_lights(LED_COLORS::CYAN, LED_PATTERNS::CONTINUOUS); break;
            case 7: patlite_lights(LED_COLORS::WHITE, LED_PATTERNS::CONTINUOUS); break;
            case 8: 
                // 8を受信したらプログラムを終了
                patlite_lights(LED_COLORS::OFF, LED_PATTERNS::CONTINUOUS);
                rclcpp::shutdown();
                break;
            default:
                std::cerr << "Invalid color code received: " << msg->data << std::endl;
                break;
            
            sleep(2);
        }
    }

    int patlite_lights(LED_COLORS color, LED_PATTERNS ptn) {
        uint8_t buf[9] = {0x00, 0x00, 0x00, 0x08, 0x0f, 0x00, 0x00, 0x00, 0x00};
        int led_pos = 5;

        switch (color) {
            case LED_COLORS::OFF: buf[led_pos] = 0x00; break;
            case LED_COLORS::RED: buf[led_pos] = 0x10; break;
            case LED_COLORS::GREEN: buf[led_pos] = 0x20; break;
            case LED_COLORS::YELLOW: buf[led_pos] = 0x30; break;
            case LED_COLORS::BLUE: buf[led_pos] = 0x40; break;
            case LED_COLORS::PURPLE: buf[led_pos] = 0x50; break;
            case LED_COLORS::CYAN: buf[led_pos] = 0x60; break;
            case LED_COLORS::WHITE: buf[led_pos] = 0x70; break;
            default: buf[led_pos] = 0xF0; break;
        }

        switch (ptn) {
            case LED_PATTERNS::OFF: buf[led_pos] |= 0x00; break;
            case LED_PATTERNS::CONTINUOUS: buf[led_pos] |= 0x01; break;
            default: buf[led_pos] |= 0x0F; break;
        }

        return patlite_set(buf);
    }

    int patlite_set(uint8_t *buf) {
        int result = 0;

        if (patlite_handle == nullptr) {
            std::cerr << "Unable to open device. Please check that it is connected." << std::endl;
            result = -1;
        } else {
            result = hid_write(patlite_handle, buf, 9);

            if (result == -1) {
                std::cerr << "Patlite set failed, return " << result << "." << std::endl;
            }
        }

        return result;
    }

    int patlite_device_open(void) {
        int result = 0;

        patlite_handle = hid_open(0x191A, 0x6001, nullptr); // Vendor ID and Product ID

        if (patlite_handle != nullptr) {
            std::cout << "Succeeded to open device." << std::endl;
        } else {
            std::cerr << "Unable to open device. Please check that it is connected." << std::endl;
            result = -1;
        }

        return result;
    }

    int patlite_device_close(void) {
        if (patlite_handle) {
            hid_close(patlite_handle);
            std::cout << "Succeeded to close device." << std::endl;
        }
        return 0;
    }

    int patlite_init() {
        return hid_init();
    }

    rclcpp::Subscription<std_msgs::msg::Int32>::SharedPtr subscription_;
    static hid_device *patlite_handle;
};

hid_device *PatliteNode::patlite_handle = nullptr;

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<PatliteNode>());
    rclcpp::shutdown();
    return 0;
}