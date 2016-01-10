#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <dirent.h>
#include "tld_utils.h"
#include "TLD.h"

//#include "CompressiveTracker.h"


using namespace cv;
using namespace std;

// Global Variables
// Face model
const String face_cascade_name = "haarcascade_frontalface_alt.xml";

void read_options(int, char**,VideoCapture&);
vector<Rect> faceDetection(Mat);

int main(int argc, char** argv)
{
								// parameters setting
								VideoCapture capture;
								FileStorage fs;
								fs.open("parameters.yml", FileStorage::READ);
								int skip_msecs = 0;

								struct option longopts[] = {
										{"s",required_argument,},
										{}
										{0,0,0,0}

								}

								read_options(argc,argv,capture);

								if (!capture.isOpened()) {
																cout << "VideoCapture Fail !!!" << endl;
																return 1;
								}

								namedWindow("Test", WINDOW_AUTOSIZE);
								capture.set(CV_CAP_PROP_POS_MSEC, skip_msecs); // The big bang theory
								//capture.set(CV_CAP_PROP_POS_MSEC, 1400000); // Sherlock

								Mat frame;
								Mat last_gray;
								vector<Rect> faces;

								while (faces.size() == 0) {
																capture >> frame;
																cvtColor(frame, last_gray, CV_RGB2GRAY);
																faces = faceDetection(last_gray);
								}

								TLD tld;
								tld.read(fs.getFirstTopLevelNode());
								tld.init(last_gray, faces[0]);

								Mat current_gray;
								BoundingBox pbox;
								vector<Point2f> pts1;
								vector<Point2f> pts2;
								bool status = true;
								bool tl = true;

								char dir_stringBuffer[20];
								char image_stringBuffer[20];
								int dir_counter = 1;
								int image_counter = 1;

								while (capture.read(frame)) {
																cvtColor(frame, current_gray, CV_RGB2GRAY);
																tld.processFrame(last_gray, current_gray, pts1, pts2, pbox, status, tl);
																if (status) {
																								Mat face_image = frame(pbox);
																								imwrite("",face_image);

																								//drawPoints(frame, pts1);
																								//drawPoints(frame, pts2, Scalar(0, 255, 0));
																								//drawBox(frame, pbox);
																}
																else{

																}
																imshow("Test", frame);
																waitKey(1);
																swap(last_gray, current_gray);
																pts1.clear();
																pts2.clear();
								}


								/*
								   CompressiveTracker tracker;
								   for (size_t i = 0; i < faces.size(); i++) {
								   tracker.init(last_gray, faces[i]);
								   }

								   Mat current_gray;
								   while (capture.read(frame)) {
								   cvtColor(frame, current_gray, CV_RGB2GRAY);

								   for (size_t i = 0; i < faces.size(); i++) {
								   tracker.processFrame(current_gray, faces[i]);
								   rectangle(frame, faces[i], Scalar(0, 0, 255));
								   }
								   imshow("Test", frame);
								   if (cvWaitKey(33) == 'q') { break; }
								   }
								 */


								system("pause");
								return 0;
}

void read_options(int argc, char** argv,VideoCapture& capture){
								for(size_t i=0; i<argc; i++) {
																if (strcmp(argv[i],"-s")==0) {
																								if (argc>i) {
																																string video = string(argv[i+1]);
																																capture.open(video);
																								}

																}
								}
}

vector<Rect> faceDetection(Mat frame) {
								CascadeClassifier face_cascade;
								vector<Rect> faces;

								if (!face_cascade.load(face_cascade_name)) {
																printf("Error Loading");
								}

								face_cascade.detectMultiScale(frame, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, cvSize(90, 90));
								return faces;
}
