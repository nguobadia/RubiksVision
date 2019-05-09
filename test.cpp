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


static void detectColors(const Mat &image/*, const vector<vector<Point>> squares*/) {
  Mat hsv;
  cvtColor(image, hsv, COLOR_BGR2HSV);
  cv::inRange(hsv, Scalar(100, 50, 50), Scalar(130, 255, 255), hsv);

  // imshow("Detecting Red", hsv);
  // waitKey(1);

}

static void detectColors(const Mat &image, const vector<vector<Point>> &squares) {
  vector<Rect> rects;
  rects.clear();

  // red
  Scalar lower_red;
  Scalar upper_red;

  // orange
  Scalar lower_orange;
  Scalar upper_orange;

  // yellow
  Scalar lower_yellow;
  Scalar upper_yellow;

  // green
  Scalar lower_green;
  Scalar upper_green;

  // blue
  Scalar lower_blue = Scalar(100, 50, 50, 0);
  Scalar upper_blue = Scalar(130, 255, 255, 0);

  // white
  Scalar lower_white;
  Scalar upper_white;

  vector<Rect> rectangles;

  for (size_t i = 0; i < squares.size(); i++) {
    rectangles.push_back(Rect(boundingRect(squares[i])));

  }
  groupRectangles(rectangles, 1, .1);

  for (size_t i = 0; i < rectangles.size(); i++) {

    Rect rect = rectangles[i];
    Mat roi = image(rect);
    Mat hsv;
    cvtColor(roi, hsv, COLOR_BGR2HSV);
    Scalar colorRect = mean(hsv);

    // cout << rect << endl;


  }

  cout << rectangles.size() << endl;

	/*Scalar colors[6] = { // BGR
		{70, 25, 130, 0}, // red
		{90, 90, 180, 0}, // orange
		{100, 185, 185, 0}, // yellow
		{65, 95, 35, 0}, // green
		{140, 25, 0, 0}, // blue
		{200, 175, 160, 0} // white

	};

  char colorchars[6] = {
    'R', 'O', 'Y',
    'G', 'B', 'W'

  };

  vector<char> chars;
  int count = 0;
  for (size_t i = 0; i < squares.size(); i+= 2) {

    Rect rect = boundingRect(Mat(squares[i]));
    if (rect.height <= 30 || rect.width <= 30 ) {
      continue;

    }
    rects.push_back(rect);
    // count++;
    Mat roi = image(rect);
    Scalar color = mean(roi);
    Scalar tempcolor = {0, 0, 0, 0};
    char c = ' ';
    int min = 1000;
    for (size_t r = 0; r < 6; r++) { // 6 colors to check
      tempcolor = colors[r]; // gets the current colors
      int delta = fabs(tempcolor[0] - color[0])
        + fabs(tempcolor[1] - color[1])
        + fabs(tempcolor[2] - color[2]);



      if (delta < min) {
        min = delta;
        c = colorchars[r];
        cout << "Color: " << color << " " << rect << endl;
        // printf("%c", c);

      }

    }

  }

  bool (*compareFn)(Rect, Rect) = compareRects;
  stable_sort(rects.begin(), rects.end(), compareFn);
  int delta = 25;
  for (size_t k = 0; k < 9; k += 2) {
    // Rect current = rects[k];
    printf("%c", chars[k]);

  }


}

// compares the coordinates of a rectangle
bool compareRects(Rect r1, Rect r2) {
  int delta = 25; // allowed difference between the 2 numbers

  if (r1.y + delta > r2.y && r1.y - delta < r2.y) { // y's are equal so sort by the y's
    return (r1.x + delta < r2.x && r1.x - delta > r2.x);

  } else if (r1.x + delta > r2.x && r1.x - delta < r2.x) { // x's are equal so sort by x's
    return (r1.y + delta < r2.y && r1.y - delta > r2.y);


  } else { // it's a duplicate
    return 0;

  }*/

}

// finds the euclidean distance of colors between 2 pixels
int euclideanDist(int b1, int b2, int g1, int g2, int r1, int r2) {
  return ((b2 - b1) * (b2 - b1)) + ((g2 - g1) * (g2 - g1)) + ((r2 - r1) * (r2 - r1));

}

int main() {

    Mat frame;
    vector<vector<Point>> squares;
    VideoCapture cap(0);

    /*for(;;) {
      cap >> frame;
      if (frame.empty()) return -1;
      detectColors(frame);


    }*/

// THE MOST GHETTO LOOPING STRUCTURE I'VE EVER DONE PLEASE DON'T JUDGE ME //

   ofstream outfile;
   outfile.open("Cube.txt");
    // RED
    while ((int) squares.size() < 18) {

      cap >> frame;
      if (frame.empty()) return -1;

      findSquares(frame, squares);
      drawSquares(frame, squares);
      imshow("Red", frame);
      imwrite("red.png", frame);
      waitKey(1);

    }
    detectColors(frame, squares);
    squares.clear();
    /*printf(" ");
    
    string dummy;
    cout << "Testing : DUMMY\n";
    getline(cin, dummy, '\n');//  >> dummy;

    // ORANGE
    while ((int) squares.size() < 18) {

      cap >> frame;
      if (frame.empty()) return -1;

      findSquares(frame, squares);
      drawSquares(frame, squares);
      imshow("Orange", frame);
      imwrite("orange.png", frame);
      waitKey(1);

    }
    detectColors(frame, squares);
    squares.clear();
    printf(" ");

    //YELLOW
    while ((int) squares.size() < 18) {

      cap >> frame;
      if (frame.empty()) return -1;

      findSquares(frame, squares);
      drawSquares(frame, squares);
      imshow("Yellow", frame);
      imwrite("yellow.png", frame);
      waitKey(1);

    }
    detectColors(frame, squares);
    squares.clear();
    printf(" ");

    // GREEN
    while ((int) squares.size() < 18) {

      cap >> frame;
      if (frame.empty()) return -1;

      findSquares(frame, squares);
      drawSquares(frame, squares);
      imshow("Green", frame);
      imwrite("green.png", frame);
      waitKey(1);

    }
    detectColors(frame, squares);
    squares.clear();
    printf(" ");

    // BLUE
    while ((int) squares.size() < 18) {

      cap >> frame;
      if (frame.empty()) return -1;

      findSquares(frame, squares);
      drawSquares(frame, squares);
      imshow("Blue", frame);
      imwrite("blue.png", frame);
      waitKey(1);

    }
    detectColors(frame, squares);
    squares.clear();
    printf(" ");

    // WHITE
    while ((int) squares.size() < 18) {

      cap >> frame;
      if (frame.empty()) return -1;

      findSquares(frame, squares);
      drawSquares(frame, squares);
      imshow("White", frame);
      imwrite("white.png", frame);
      waitKey(1);

    }
    detectColors(frame, squares);
    squares.clear();
    puts("");*/

    return 0;

}
