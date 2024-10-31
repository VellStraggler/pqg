#include <iostream>
#include <string>
#include <io.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

using namespace std;
// shapes:
// diagonal rise, diagonal fall, vertical split, horizontal split

bool fileExists(const string &filePath) {
    return (_access(filePath.c_str(), 0) != -1);
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


    // loop through the whole array once to get 8 average colors (using 6 color values):

    // Store the average red, blue, and green values, and the min and max values
    long rgb_sums[3] = {0,0,0};
    uint8_t rgb_mins[3] = {255,255,255};
    uint8_t rgb_maxes[3] = {0,0,0};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate index in rgb_image
            int index = (y * width + x) * 3;
            // Get the RGB values
            for (int i = 0; i < 3; ++i) {
                uint8_t v = rgb_image[index + i];
                // add to a sum for an eventual average
                rgb_sums[i] += v;
                // get the min
                if (v < rgb_mins[i]) {
                    rgb_mins[i] = v;
                }
                // get the max
                if (v > rgb_maxes[i]) {
                    rgb_maxes[i] = v;
                }
            }
        }
    }
    // Get the median value of min and avg and of the max and the avg, these are the two numbers we'll use
    for (int i = 0; i < 3; ++i) {
        // compute the average
        rgb_sums[i] /= (width * height);
        cout << "RGB: " << i << ", average: " << rgb_sums[i] << ", min: " << static_cast<int>(rgb_mins[i]) << ", max: " << static_cast<int>(rgb_maxes[i]) << endl;
        rgb_mins[i] = (rgb_mins[i] + rgb_sums[i]) / 2;
        rgb_maxes[i]= (rgb_maxes[i]+ rgb_sums[i]) / 2;
        cout << "RGB: " << i << ", average: " << rgb_sums[i] << ", min: " << static_cast<int>(rgb_mins[i]) << ", max: " << static_cast<int>(rgb_maxes[i]) << endl;
        
    }
    // loop through the array again to apply the the closest colors using the numbers stored
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 3;
            for (int i = 0; i < 3; ++i) {
                uint8_t v = rgb_image[index + i];
                // static cast because uint8_t is unsigned (cannot hold negative value)
                uint8_t dist_to_min = abs(static_cast<int>(rgb_mins[i]) - static_cast<int>(v));
                uint8_t dist_to_max = abs(static_cast<int>(rgb_maxes[i])- static_cast<int>(v));
                uint8_t dist_to_avg = abs(static_cast<int>(rgb_sums[i])- static_cast<int>(v));
                dist_to_max = abs(255- static_cast<int>(v));
                dist_to_avg = abs(0- static_cast<int>(v));
                // use whichever one is closest and "round" to that number
                if (dist_to_max < dist_to_min) {
                    rgb_image[index + i] = rgb_maxes[i];
                } else {//if (dist_to_min < dist_to_max) {
                    rgb_image[index + i] = rgb_mins[i];
                }
            }
        }
    }
    // loop through the array one more time in 8x8 segments
    //   count how many of each number appears, then get the two most frequent 
    
    // Save the processed image
    string output_file_name = "output/4" + file_name;
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
    return 0;
}