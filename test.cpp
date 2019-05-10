#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>

using namespace cv;
using namespace std;

// rectangle comparator
static bool compareRects(Rect, Rect);

// detects the colors and assigns a char to that color
static char findCharsForColors(Scalar);

// detects the different between the rgb values of 2 pixels
static float euclideanDist(float, float, float, float, float, float);


// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
static double angle(Point pt1, Point pt2, Point pt0) {

    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1 * dx2 + dy1 * dy2) / sqrt((dx1 * dx1 + dy1 * dy1) * (dx2 * dx2 + dy2 * dy2) + 1e-10);

}

// displays the identified squares onto the screen
static void drawSquares(Mat &image, const vector<vector<Point>> &squares) {


    for (size_t i = 0; i < squares.size(); i++) {

        const Point *p = &squares[i][0];
        int n = (int) squares[i].size();
        int shift = 1;
        Rect r = boundingRect(Mat(squares[i]));

        if (r.width > 30 && r.height > 30) {
          r.x = r.x + r.width / 4;
          r.y = r.y + r.height / 4;
          r.width = r.width / 2;
          r.height = r.height / 2;

          Mat roi = image(r);
          Scalar color = mean(roi);
          polylines(image, &p, &n, 1, true, color, 2, LINE_AA, shift);

        }

      }

}

// returns sequence of squares detected on the image.
// the sequence is stored in the specified memory storage
static void findSquares(const Mat &image, vector<vector<Point>> &squares, bool inv = false) {

    squares.clear();
    Mat gray,gray0,hsv;
    vector<vector<Point>> contours;

    cvtColor(image,hsv,COLOR_BGR2HSV);
    Mat hsv_channels[3];
    cv::split(hsv, hsv_channels); // splits the hsv values into it's own array
    gray0 = hsv_channels[2]; // the 3rd channel is a grayscale value
    GaussianBlur(gray0, gray0, Size(7,7), 1.5, 1.5);
    Canny(gray0, gray, 0, 30, 3);
    int kernaltemp[3][3] = {{1, 1, 1}, 
                      {1, 1, 1}, 
                      {1, 1, 1}};
    
    
                      
    dilate(gray0, gray0, InputArrayOfArrays(kernaltemp), Point(-1, -1), 2);

    // find contours and store them all as a list
    findContours(gray, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);
    vector<Point> approx;

    // test each contour
    for( size_t i = 0; i < contours.size(); i++) {
        // approximate contour with accuracy proportional
        // to the contour perimeter
        approxPolyDP(Mat(contours[i]), approx, 5, true);

        // square contours should have 4 vertices after approximation
        // relatively large area (to filter out noisy contours)
        // and be convex.
        // Note: absolute value of an area is used because
        // area may be positive or negative - in accordance with the
        // contour orientation
        if( approx.size() == 4 &&
                fabs(contourArea(Mat(approx))) > 10 &&
                isContourConvex(Mat(approx))) {
            double maxCosine = 0;

            for( int j = 2; j < 5; j++ ) {

                // find the maximum cosine of the angle between joint edges
                double cosine = fabs(angle(approx[j % 4], approx[j - 2], approx[j - 1]));
                maxCosine = MAX(maxCosine, cosine);

            }

            // if cosines of all angles are small
            // (all angles are ~90 degree) then write quandrange
            // vertices to resultant sequence
            if( maxCosine < 0.3 /*&& squares.size() < 18*/) squares.push_back(approx);
        }

    }

}


static void detectColors(const Mat &image, const vector<vector<Point>> &squares) {
  vector<Rect> rectangles;
  rectangles.clear();

  for (size_t i = 0; i < squares.size(); i++) {
    rectangles.push_back(Rect(boundingRect(squares[i])));

  }

  // the magical line of code that gets rid of the duplicate squares!!!
  groupRectangles(rectangles, 1, .1);

  for (size_t i = 0; i < rectangles.size(); i++) {

    Rect rect = rectangles[i];
    Mat roi = image(rect);
    // Mat hsv;
    // cvtColor(roi, hsv, COLOR_BGR2HSV);
    Scalar colorRect = mean(roi);

    cout << findCharsForColors(colorRect);

    // cout << "Color: " << colorRect << ", Rect: " << rect << endl;


  }
  // puts(" ");

  // cout << rectangles.size() << endl;

}

static char findCharsForColors(Scalar colorOfRect) {

  // scalar representations of the BGR values
  vector<Scalar> colors = {

    Scalar(35, 10, 105)/*red*/, Scalar(50, 75, 150)/*orange*/, Scalar(50, 150, 140) /*yellow*/,
    Scalar(60, 110, 25) /*green*/, Scalar(140, 30, 10)/*blue*/, Scalar(200, 185, 170)/*white*/

  };

  vector<Scalar> lower_colors = {
    Scalar(0, 100, 100)/*red lower bound*/, Scalar(5, 50, 50)/*orange lower bound*/

  };

  vector<Scalar> upper_colors = {
    Scalar(20, 255, 255)/*red upper bound*/, Scalar(15, 255, 255)/*orange upper bound*/

  };

  char colorChars[] = {
    'R', 'O', 'Y',
    'G', 'B', 'W'

  };

  float dist = INT_MAX;
  float minDist = INT_MAX;
  char color = ' ';
  for (size_t i = 0; i < colors.size(); i++) {
    
    float b1 = colorOfRect[0];
    float g1 = colorOfRect[1];
    float r1 = colorOfRect[2];

    float b2 = colors[i][0];
    float g2 = colors[i][1];
    float r2 = colors[i][2];

    dist = euclideanDist(b1, b2, g1, g2, r1, r2);
    if (dist < minDist) {
      minDist = dist;
      color = colorChars[i];

    }

  }

  return color;
}

// finds the euclidean distance of colors between 2 pixels
static float euclideanDist(float b1, float b2, float g1, float g2, float r1, float r2) {
  return ((b2 - b1) * (b2 - b1)) + ((g2 - g1) * (g2 - g1)) + ((r2 - r1) * (r2 - r1));

}

static void delayAndReset(VideoCapture &cap, vector<vector<Point>> &squares) {
    
    waitKey(0);
    squares.clear();
    cap.release();
    printf(" ");
    destroyAllWindows();
    cap.open(0);

}

int main() {

    Mat frame;
    vector<vector<Point>> squares;
    VideoCapture cap(0);

    vector<string> windowNames = {

    "Red Side", "Orange Side", "Yellow Side",
    "Green Side", "Blue Side", "White Side"

    };
    vector<string> colorNames = {

    "red.png", "orange.png", "yellow.png", 
    "green.png", "blue.png", "white.png"

    };

    for (size_t i = 0; i < 6; i++) {

      while ((int) squares.size() < 18) {

        cap >> frame;
        if (frame.empty()) return -1;

        findSquares(frame, squares);
        drawSquares(frame, squares);
        imshow(windowNames[i], frame);
        imwrite(colorNames[i], frame);
        waitKey(1);

      }
    detectColors(frame, squares);
    delayAndReset(cap, squares);

    }
    puts(" ");

    return 0;

}
