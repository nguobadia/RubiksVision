#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string>

using namespace cv;
using namespace std;

// detects the colors and assigns a char to that color
static char findCharsForColors(Scalar);


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
    cvtColor(image, hsv, CV_BGR2HSV);
    Mat hsv_channels[3];
    split(hsv, hsv_channels); // splits the hsv values into it's own array
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

static void detectColors(const Mat &image, const vector<vector<Point>> &squares, vector<char> &charRectangles, 
                          vector<Rect> &rectangles) {
  
  rectangles.clear();
  char color;

  for (size_t i = 0; i < squares.size(); i++) {
    rectangles.push_back(Rect(boundingRect(squares[i])));

  }

  // the magical line of code that gets rid of the duplicate squares!!!
  groupRectangles(rectangles, 1, .1);
  // cv::sortIdx(rectangles, rectangles, cv::SORT_EVERY_ROW | cv::SORT_DESCENDING);

  for (size_t i = 0; i < rectangles.size(); i++) {

    Rect rect = rectangles[i];
    Mat roi = image(rect);
    Mat hsv;
    Scalar colorRect = mean(roi);
    cvtColor(roi, hsv, CV_BGR2HSV);
    colorRect = mean(hsv);
    // cout << colorRect << endl;
    color = findCharsForColors(colorRect);
    charRectangles.push_back(color);
    // cout << color;


  }

}

static char findCharsForColors(Scalar colorOfRect) {

  // HSV values
  vector<Scalar> lb = {
    Scalar(155, 100, 50)/*red*/, Scalar(0, 100, 50)/*orange*/, Scalar(23, 100, 50) /*yellow*/,
    Scalar(60, 100, 50)/*green*/, Scalar(76, 100, 50)/*blue*/, Scalar(0, 0, 0)/*white*/

  };

  vector<Scalar> ub = {
    Scalar(180, 225, 225)/*red*/, Scalar(75, 255, 255)/*orange*/, Scalar(33, 255, 255) /*yellow*/,
    Scalar(75, 255, 255)/*green*/, Scalar(135, 255, 255)/*blue*/, Scalar(180, 100, 255)/*white*/

  };
  // cout << colorOfRect << endl;

  char colorChars[] = {
    'R', 'O', 'Y',
    'G', 'B', 'W'

  };

  char color = ' ';
  for (size_t i = 0; i < 6; i++) {

    float h = colorOfRect[0];
    float s = colorOfRect[1];
    float v = colorOfRect[2];

    float h_lower = lb[i][0];
    float s_lower = lb[i][1];
    float v_lower = lb[i][2];

    float h_upper = ub[i][0];
    float s_upper = ub[i][1];
    float v_upper = ub[i][2];

    int remh = (int)h % 156;
    if ((remh < h_upper) && (s < s_upper) && (v < v_upper) && 
        (remh > h_lower) && (s > s_lower) && (v > v_lower)) {

          color = colorChars[i];

    }

  }
  return color;

}

static void delayAndReset(VideoCapture &cap, vector<vector<Point>> &squares) {
    
    waitKey(0);
    squares.clear();
    cap.release();
    printf(" ");
    destroyAllWindows();
    cap.open(0);

}

static void interpreter (vector<char> &face, vector<char> &remap) {

  switch(face[4]) {
    case('G'):
    case('R'):
    case('B'):
    case('O'):
      remap[0] = face[8]; remap[1] = face[5]; remap[2] = face[2];
      remap[3] = face[1]; remap[4] = face[0]; remap[5] = face[3];
      remap[6] = face[6]; remap[7] = face[7];

    case('W'):
      remap[0] = face[5]; remap[1] = face[2]; remap[2] = face[1];
      remap[3] = face[0]; remap[4] = face[3]; remap[5] = face[6];
      remap[6] = face[7]; remap[7] = face[8];
    
    case('Y'):
      remap[0] = face[5]; remap[1] = face[1]; remap[2] = face[7];
      remap[3] = face[6]; remap[4] = face[3]; remap[5] = face[0];
      remap[6] = face[1]; remap[7] = face[2];
  }

  remap[8] = face[4]; // center piece

}

int main() {

    Mat frame;
    vector<vector<Point>> squares;
    vector<Rect> rectangles;
    vector<char> unsortedCharRectangles;
    vector<char> sortedCharRectangles = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    VideoCapture cap(0);
    ofstream cube;
    cube.open("Cube.txt");

    vector<string> windowNames = {

      "Red Side", "Orange Side", "Yellow Side",
      "Green Side", "Blue Side", "White Side"

    };
    vector<string> colorNames = {

      "red.png", "orange.png", "yellow.png", 
      "green.png", "blue.png", "white.png"

    };

    for (size_t i = 0; i < 6; i++) {
      // do whiles are amazing!!
      do {
        cap >> frame;
        if (frame.empty()) return -1;

        findSquares(frame, squares);
        drawSquares(frame, squares);
        detectColors(frame, squares, unsortedCharRectangles, rectangles);
        imshow(windowNames[i], frame);
        imwrite(colorNames[i], frame);
        waitKey(1);

      } while (squares.size() < 18 && rectangles.size() != 9);

      unsortedCharRectangles.clear();
      interpreter(unsortedCharRectangles, sortedCharRectangles);

      for (size_t j = 0; j < sortedCharRectangles.size(); j++) {
        cout << sortedCharRectangles[j];
        cube << sortedCharRectangles[j];

      }
      cube << " ";
      delayAndReset(cap, squares);
    
    }
  puts(" ");
  cube.close();

  return 0;

}
