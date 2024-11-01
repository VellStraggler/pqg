#include <iostream>
#include <string>
#include <vector>
#include <io.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <bitset>
#include <unordered_map>
#include <map>

using namespace std;
// shapes:
// diagonal rise, diagonal fall, vertical split, horizontal split
const uint8_t r = 256 / 2;
const string photo_number = "30";

bool fileExists(const string &filePath) {
    return (_access(filePath.c_str(), 0) != -1);
}
unsigned int RGBtoInt(uint8_t rgb[3]) {
    return (rgb[0] << 16) | (rgb[1] << 8) | rgb[2];
}
tuple<uint8_t, uint8_t, uint8_t> IntToRGB(int color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    return make_tuple(r,g,b);
}
float RGBtoSatAndVal(uint8_t r, uint8_t g, uint8_t b) {
    float r_norm = r / 255.0f;
    float g_norm = g / 255.0f;
    float b_norm = b / 255.0f;

    float max_val = max(r_norm, max(g_norm, b_norm));
    float min_val = min(r_norm, min(g_norm, b_norm));
    float delta = max_val - min_val;

    float value = max_val;
    float saturation;

    if (max_val == 0) saturation = 0.1;
    else saturation = delta / max_val;

    return 1 + ((saturation + value));
}


int main() {
    // Pick from reading a text file or converting an image file
    int input;
    cout << "Choose what to do: (0) Read a text file or (1) convert an image file: ";
    cin >> input;
    string message = "You have chosen to ";
    if(input > 0) {
        message += "convert an image";
    } else {
        message += "read a text file";
    }
    message += ". Please enter the file name. Do not include \"input/\": ";
    cout << message;
    // Take a file name as input
    string file_name;
    cin >> file_name;
    string input_file_name = "input/" + file_name;
    if (!fileExists(input_file_name)) {
        message = file_name + " does not exist";
        cout << message;
        return -2;
    }

    // CONVERT IMAGE FILE
    // read the image
    int width, height, bpp;
    // handles declaration of variables via reference-passing
    uint8_t* rgb_image = stbi_load(input_file_name.c_str(), &width, &height, &bpp, 3);
    cout << "Loaded image with width: " << width << ", height: " << height << ", channels: " << bpp << endl;

    // create a 2D array that is 8 times smaller than the image resolution
    const int new_width = width/8;
    const int new_height = height/8;
    int** int_array = new int*[new_width];
    for (int i = 0; i < new_height; ++i) {
        int_array[i] = new int[new_height]; 
    }


    // loop through the whole array once to get the 8 most common colors:
    map<int, int> rgbCount;
    // get the sum total, then go up to half the sum, then half of that
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate index in rgb_image
            int index = (y * width + x) * 3;
            // Get the RGB values of this pixel
            uint8_t rgb_small[3];
            for (int i = 0; i < 3; ++i) {
                rgb_small[i] = (rgb_image[index + i] / 16) * 16; // rounding 
            }
            int rgb = RGBtoInt(rgb_small);
            // count up each rgb number
            auto it = rgbCount.find(rgb);
            if (it == rgbCount.end()) { // create new key
                rgbCount[rgb] = 1;
            } else {
                rgbCount[rgb] = rgbCount[rgb] + 1;
            }
        }
    }
    // Find the top colors
    int top_keys[8];
    for (int i = 0; i < 8; ++i) {
        int max = 0;
        int max_key = 0;
        for (const auto &pair : rgbCount) {
            uint8_t r, g, b;
            tie(r, g, b) = IntToRGB(max_key);
            float saturation = RGBtoSatAndVal(r, g, b);
            cout << saturation << endl;
            int weighted_count = static_cast<int>(pair.second * saturation);
            if (weighted_count > max) {
                max = pair.second;
                max_key = pair.first;
            }
        }
        rgbCount.erase(max_key); // don't want to pick it 8 times
        cout<<max_key<<endl;
        for (int j = max_key - 1500000; j < max_key + 1500000; ++j) {
            rgbCount.erase(j);
        }
        top_keys[i] = max_key;
        // uint8_t rgb_old[3] = {r,g,b};
        // int rgb_check = RGBtoInt(rgb_old);
        // cout << "key: " << max_key << 
        //     ", RGB: " << static_cast<int>(r)  
        //     <<","<<static_cast<int>(g) 
        //     <<","<<static_cast<int>(b) <<
        //     ", oldRGB: " << rgb_check <<endl;
    }
    // round each pixel to one of the 8 provided colors
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate index in rgb_image
            int index = (y * width + x) * 3;
            // Get the RGB values of this pixel
            uint8_t rgb_small[3];
            for (int i = 0; i < 3; ++i) {
                rgb_small[i] = (rgb_image[index + i] / 8) * 8; // rounding 
            }
            int rgb = RGBtoInt(rgb_small);

            // find which of the colors is closest
            int min_dist = 1000000;
            int color_pick = 0;
            for (int i = 0; i < 8; ++i) {
                if (abs(rgb - top_keys[i]) < min_dist) {
                    min_dist = abs(rgb - top_keys[i]);
                    color_pick = i;
                }
            }
            // assign the new color
            uint8_t r, g, b;
            tie(r, g, b) = IntToRGB(top_keys[color_pick]);
            uint8_t new_rgb[3] = {r,g,b};
            for (int i = 0; i < 3; ++i) {
                rgb_image[index + i] = new_rgb[i];
            }
        }
    }

    // loop through the array one more time in 8x8 segments
    //   count how many of each number appears, then get the two most frequent 
    
    // Save the processed image
    string output_file_name = "output/" + photo_number + file_name;
    if (stbi_write_png(output_file_name.c_str(), width, height, 3, rgb_image, width*3)) {
        cout << "Image saved successfully as " << output_file_name << endl;
    } else {
        cout << "Failed to save image. :(" << endl;
    }
    // Clean up memory
    for (int i = 0; i < new_height; ++i) {
        delete[] int_array[i]; // delete each row
    }
    delete[] int_array;
    stbi_image_free(rgb_image); //this frees the memory. Only STBI knows how to do this right.

    // READ TEXT FILE
    // file format: half-min,half-max(for R G and B),width,height,0,4,2,7,9... 
    // if we go off multiples of 16 instead, we can store smaller numbers to represent RGB values while increasing color options
    return 0;
}