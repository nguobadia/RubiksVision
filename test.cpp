#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#include <iostream>

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

    /*for( size_t i = 0; i < squares.size(); i++ )
    {
        const Point* p = &squares[i][0];
        int n = (int)squares[i].size();
        int shift = 1;

        Rect r=boundingRect( Mat(squares[i]));
        r.x = r.x + r.width / 4;
        r.y = r.y + r.height / 4;
        r.width = r.width / 2;
        r.height = r.height / 2;

        Mat roi = image(r);
        Scalar color = mean(roi);
        polylines(image, &p, &n, 1, true, color, 2, LINE_AA, shift);

        Point center( r.x + r.width/2, r.y + r.height/2 );
        ellipse( image, center, Size( r.width/2, r.height/2), 0, 0, 360, color, 2, LINE_AA );
    }*/

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
            if( maxCosine < 0.3 && squares.size() <= 18) squares.push_back(approx);
        }

    }

}

static void detectColors(const Mat &image, const vector<vector<Point>> &squares) {
  vector<Rect> rects;
	Scalar colors[6] = {
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

  for (size_t i = 0; i < squares.size(); i ++) {

    Rect r = boundingRect(Mat(squares[i]));
    if (r.height > 30 && r.width > 30 ) rects.push_back(r);
    Mat roi = image(r);
    Scalar color = mean(roi);
    Scalar tempcolor = {0, 0, 0, 0};
    char c = ' ';
    int min = 1000;
    for (size_t r = 0; r < 6; r++) { // 6 colors to check
      tempcolor = colors[r]; // gets the current colors
      int delta = fabs(tempcolor[0] - color[0])
        + fabs(tempcolor[1] - color[1])
        + fabs(tempcolor[2] - color[2]);

      // cout << delta << endl;
      if (delta < min) {
        min = delta;
        c = colorchars[r];

      }

    }
    // cout << "rect: " << r << ", delta: " << min  << ", "<< c  << ": " << color << endl;

  }
  // cout << "size: " << squares.size() / 2 << endl;
  bool (*compareFn)(Rect, Rect) = compareRects;
  stable_sort(rects.begin(), rects.end(), compareFn);

  for (size_t k = 0; k < rects.size(); k++) {
    Rect current = rects[k];
    // if (k % 2 == 0) rects.erase(rects.begin() + k);
    /*else*/ cout << k + 1 << " : " << current << endl;

  }
  // cout << "size: " << rects.size() << endl;


}

// compares the coordinates of a rectangle
static bool compareRects(Rect r1, Rect r2) {
  int delta = 25; // allowed difference between the 2 numbers

  if (r1.y + delta > r2.y || r1.y - delta < r2.y) return (r1.x + delta <= r2.x && r1.x - delta >= r2.x);
  return (r1.y + delta < r2.y && r1.y - delta > r2.y);

}

int main() {

    Mat frame;
    vector<vector<Point>> squares;
    VideoCapture cap(0);

    while ((int) squares.size() < 18) {

      cap >> frame;
      if (frame.empty()) return -1;

      findSquares(frame, squares);
      drawSquares(frame, squares);
      imshow("Rubic Detection Demo", frame);
      imwrite("test.png", frame);
      waitKey(1);

    }

    detectColors(frame, squares);

    return 0;

}
