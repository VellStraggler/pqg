#include <iostream>
#include <string>
#include <vector>
#include <io.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#include <bitset>
#include <map>

using namespace std;
// shapes:
// diagonal rise, diagonal fall, vertical split, horizontal split
const uint8_t r = 256 / 2;
const string photo_number = "36";

/// range of 0 to 255
int clampRGB(int a) {
    return max(min(a,255),0);
}
bool fileExists(const string &filePath) {
    return (_access(filePath.c_str(), 0) != -1);
}
int getArea(int x, int y) {
    // Ensure x and y are within bounds (0-7)
    if (x < 0 || x > 7 || y < 0 || y > 7) {
        throw out_of_range("Coordinates are out of bounds");
    }

    // Determine the section index based on the provided layout
    if (y < 3) { // First row (0, 1, 2)
        if (x < 3) {
            return 0; // 3x3 section
        } else if (x < 5) {
            return 1; // 2x3 section
        } else {
            return 2; // 3x3 section
        }
    } else if (y < 5) { // Second row (3, 4)
        if (x < 3) {
            return 3; // 3x2 section
        } else if (x < 5) {
            return 4; // 2x2 section
        } else {
            return 5; // 3x2 section
        }
    } else { // Third row (5, 6, 7)
        if (x < 3) {
            return 6; // 3x3 section
        } else if (x < 5) {
            return 7; // 2x3 section
        } else {
            return 8; // 3x3 section
        }
    }
}

unsigned int RGBtoInt(tuple<uint8_t, uint8_t, uint8_t> rgb) {
    uint8_t r = get<0>(rgb);
    uint8_t g = get<1>(rgb);
    uint8_t b = get<2>(rgb);
    return (r << 16) | (g << 8) | b;
}
vector<uint8_t> IntToRGB(int color) {
    bitset<24> bits(color);
    uint8_t r = (bits >> 16).to_ulong() & 0xFF;
    uint8_t g = (bits >> 8).to_ulong() & 0xFF;
    uint8_t b = bits.to_ulong() & 0xFF;
    return {r,g,b};
}
float RGBspread(uint8_t r, uint8_t g, uint8_t b) {
    float max_val = max(r, max(g, b));
    float min_val = min(r, min(g, b));
    float delta = max_val - min_val;

    float spread = static_cast<float>(max_val - min_val) / 255.0f;

    return (1- spread) * (1- spread);
}
// Function to increase saturation without converting to HSV
tuple<uint8_t, uint8_t, uint8_t> increaseSaturation(vector<uint8_t> rgb, float factor = 1) {
    uint8_t r = rgb[0];
    uint8_t g = rgb[1];
    uint8_t b = rgb[2];

    // Calculate the average (gray component)
    int avg = (r + g + b) / 3;

    // Amplify each component by pushing it away from the average
    int new_r = static_cast<int>(avg + (r - avg) * factor);
    int new_g = static_cast<int>(avg + (g - avg) * factor);
    int new_b = static_cast<int>(avg + (b - avg) * factor);

    // Clamp values to the 0–255 range
    new_r = clampRGB(new_r);
    new_g = clampRGB(new_g);
    new_b = clampRGB(new_b);

    return {static_cast<uint8_t>(new_r), static_cast<uint8_t>(new_g), static_cast<uint8_t>(new_b)};
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
                rgb_small[i] = (rgb_image[index + i]);
            }
            tuple<uint8_t, uint8_t, uint8_t> rgb_tuple = make_tuple(rgb_small[0],rgb_small[1],rgb_small[2]);
            int rgb = RGBtoInt(rgb_tuple);
            // count up each rgb number
            auto it = rgbCount.find(rgb);
            if (it == rgbCount.end()) { // create new key
                rgbCount[rgb] = 1;
            } else {
                rgbCount[rgb] = rgbCount[rgb] + 1;
            }
        }
    }
    // Get the total so we can find 8ths
    long total = 0;
    for (const auto &pair : rgbCount) {
        total += pair.second;
    }
    long portion = total / 8;
    // Find the top colors
    int top_keys[8];
    for (int i = 0; i < 8; ++i) {
        int max = 0;
        int max_key = 0;
        long local_median = (portion / 2) + (portion * i);
        int j = 0;
        long running_total = 0;
        for (const auto &pair : rgbCount) {
            running_total += pair.second;
            if (running_total >= local_median) {
                max_key = pair.first;
                uint8_t r, g, b;
                vector<uint8_t> rgb;
                rgb = IntToRGB(max_key);
                r = rgb[0];
                g = rgb[1];
                b = rgb[2];
                cout << rgb[0] << endl;
                cout << static_cast<int>(r) <<","<< static_cast<int>(g) <<","<< static_cast<int>(b) << endl;
                auto saturatedRGB = increaseSaturation(rgb);
                max_key = RGBtoInt(saturatedRGB);
                break;
            }
            j++;
        }
        cout<<max_key<<endl;
        top_keys[i] = max_key;
    }
    // round each pixel to one of the 8 provided colors
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate index in rgb_image
            int index = (y * width + x) * 3;
            // Get the RGB values of this pixel
            uint8_t rgb_small[3];
            for (int i = 0; i < 3; ++i) {
                rgb_small[i] = (rgb_image[index + i]);
            }
            tuple<uint8_t, uint8_t, uint8_t> rgb_tuple = make_tuple(rgb_small[0],rgb_small[1],rgb_small[2]);
            int rgb = RGBtoInt(rgb_tuple);

            // find which of the colors is closest
            int min_dist = top_keys[7];
            int color_pick = 0;
            for (int i = 0; i < 8; ++i) {
                if (abs(rgb - top_keys[i]) < min_dist) {
                    min_dist = abs(rgb - top_keys[i]);
                    color_pick = i;
                }
            }
            // assign the new color
            uint8_t r, g, b;
            vector<uint8_t> new_rgb = IntToRGB(top_keys[color_pick]);
            r = new_rgb[0];
            g = new_rgb[1];
            b = new_rgb[2];
            for (int i = 0; i < 3; ++i) {
                rgb_image[index + i] = new_rgb[i];
            }
        }
    }

    // loop through the array one more time in 8x8 segments
    //   count how many of each number appears, then get the two most frequent 
    for (int global_y = 0; global_y < height-8; global_y+=8) {
        for (int global_x = 0; global_x < width-8; global_x+=8) {
            map<int,int> count; 
            for (int y = 0; y < 8; ++y) {
                for(int x = 0; x < 8; ++x) {
                    // Calculate index in rgb_image
                    int index = ((global_y + y) * width + (global_x + x)) * 3;
                    // Get the RGB values of this pixel
                    uint8_t rgb_small[3];
                    for (int i = 0; i < 3; ++i) {
                        rgb_small[i] = (rgb_image[index + i]);
                    }
                    tuple<uint8_t, uint8_t, uint8_t> rgb_tuple = make_tuple(rgb_small[0],rgb_small[1],rgb_small[2]);
                    int rgb = RGBtoInt(rgb_tuple);
                    // count up each rgb number
                    auto it = count.find(rgb);
                    if (it == count.end()) { // create new key
                        count[rgb] = 1;
                    } else {
                        count[rgb] = count[rgb] + 1;
                    }
                }
            }
            // find the 2 most common colors
            int two_pop[2] = {0,0};
            for (int i = 0; i < 2; ++i) {
                int max = 0;
                int choice = 0;
                for (const auto &pair : count) {
                    if (pair.second > max) {
                        max = pair.second;
                        choice = pair.first;
                    }
                }
                count.erase(choice);
                two_pop[i] = choice;
            }
            // round each pixel to one of the 2 provided colors
            // also collect information on the spread of those colors
            map<int, int> areaAndCount;

            vector<vector<uint8_t>> newShapeColors(8, vector<uint8_t>(3));

            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    int area = getArea(x,y);
                    // Calculate index in rgb_image
                    int index = ((global_y + y) * width + (global_x + x)) * 3;
                    if (index >= width * height * 3 || global_x + x >= width) {
                        break;
                    }
                    // Get the RGB values of this pixel
                    uint8_t rgb_small[3];
                    for (int i = 0; i < 3; ++i) {
                        rgb_small[i] = (rgb_image[index + i]);
                    }
                    tuple<uint8_t, uint8_t, uint8_t> rgb_tuple = make_tuple(rgb_small[0],rgb_small[1],rgb_small[2]);
                    int rgb = RGBtoInt(rgb_tuple);

                    auto it = areaAndCount.find(area);
                    if (it == areaAndCount.end()) { // create new key as needed
                        areaAndCount[area] = 0;
                    }

                    // find which of the colors is closest
                    int color_pick;
                    if (abs(rgb - two_pop[0]) < abs(rgb - two_pop[1])) {
                        color_pick = two_pop[0];
                        areaAndCount[area] += 1;
                    } else {
                        color_pick = two_pop[1];
                        areaAndCount[area] -= 1;
                    }
                    // assign the new color
                    uint8_t r, g, b;
                    vector<uint8_t> new_rgb = IntToRGB(color_pick);
                    for (int i = 0; i < 3; ++i) {
                        rgb_image[index + i] = new_rgb[i];
                    }
                }
            }
            // Now assign colors based on area counts for the specific shapes
            for (auto &pair : areaAndCount) {
                int area = pair.first;
                int count = pair.second;
                int color_pick = (count > 0) ? two_pop[0] : two_pop[1];

                // Assign colors based on defined shapes
                for (int y = 0; y < 8; ++y) {
                    for (int x = 0; x < 8; ++x) {
                        int index = ((global_y + y) * width + (global_x + x)) * 3;
                        if (index >= width * height * 3 || global_x + x >= width) {
                            break;
                        }

                        // Get the RGB values of this pixel
                        uint8_t rgb_small[3];
                        for (int i = 0; i < 3; ++i) {
                            rgb_small[i] = (rgb_image[index + i]);
                        }
                        tuple<uint8_t, uint8_t, uint8_t> rgb_tuple = make_tuple(rgb_small[0], rgb_small[1], rgb_small[2]);
                        int rgb = RGBtoInt(rgb_tuple);

                        // Assign colors based on rectangular areas
                        if ((area == 0 && y < 3 && x < 3) || (area == 1 && y < 3 && x >= 3 && x < 6)) {
                            vector<uint8_t> new_rgb = IntToRGB(color_pick);
                            for (int i = 0; i < 3; ++i) {
                                rgb_image[index + i] = new_rgb[i];
                            }
                        }

                        // Crisp Top-Left Triangle
                        if (y < 4 && x < 4 && (x + y < 4)) {
                            vector<uint8_t> new_rgb = IntToRGB(color_pick);
                            for (int i = 0; i < 3; ++i) {
                                rgb_image[index + i] = new_rgb[i];
                            }
                        }

                        // Crisp Bottom-Left Triangle
                        if (y >= 4 && x < 4 && (x + (8 - y) < 4)) {
                            vector<uint8_t> new_rgb = IntToRGB(color_pick);
                            for (int i = 0; i < 3; ++i) {
                                rgb_image[index + i] = new_rgb[i];
                            }
                        }

                        // Crisp Top-Right Triangle
                        if (y < 4 && x >= 4 && ((8 - x) + y < 4)) {
                            vector<uint8_t> new_rgb = IntToRGB(color_pick);
                            for (int i = 0; i < 3; ++i) {
                                rgb_image[index + i] = new_rgb[i];
                            }
                        }

                        // Crisp Bottom-Right Triangle
                        if (y >= 4 && x >= 4 && ((8 - x) + (8 - y) < 4)) {
                            vector<uint8_t> new_rgb = IntToRGB(color_pick);
                            for (int i = 0; i < 3; ++i) {
                                rgb_image[index + i] = new_rgb[i];
                            }
                        }
                    }
                }
            }
        }
    }

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