#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <sys/stat.h>
#include "tld_utils.h"
#include "TLD.h"

//#include "CompressiveTracker.h"

using namespace cv;
using namespace std;

struct TLD_Info {
	TLD *tld;
	int dirCounter;
	int fileCounter;
};

// Global Variables
// Face model
const String face_cascade_name = "haarcascade_frontalface_alt.xml";

void read_options(int, char**,VideoCapture&,int&);
void createDirectory(VideoCapture&, FileStorage&, Mat&, vector<Rect>&, vector<TLD_Info>&, int&);
void setTLD_Info(TLD_Info&, TLD*, int, int);
void freeTLD_Info(vector<TLD_Info> &);
vector<Rect> faceDetection(Mat);

int main(int argc, char** argv)
{
	// parameters setting
	VideoCapture capture;
	//vector<TLD*> vTLD;
	vector<TLD_Info> vtld_info;
	FileStorage fs;
	fs.open("parameters.yml", FileStorage::READ);
	int skip_msecs = 0;
	int dirCounter = 1;

	// get program parameters
	read_options(argc, argv, capture, skip_msecs);

	if (!capture.isOpened()) {
		cout << "VideoCapture Fail !!!" << endl;
		return 1;
	}

	//namedWindow("Test", WINDOW_AUTOSIZE);
	capture.set(CV_CAP_PROP_POS_MSEC, skip_msecs); // The big bang theory
	//capture.set(CV_CAP_PROP_POS_MSEC, 1400000); // Sherlock

	Mat frame;
	Mat last_gray;
	vector<Rect> faces;

	createDirectory(capture, fs, last_gray, faces, vtld_info, dirCounter);

	Mat current_gray;
	BoundingBox pbox;
	vector<Point2f> pts1;
	vector<Point2f> pts2;
	bool status = true;
	bool tl = true;

	int remainFrame = 3;

	while (capture.read(frame)) {
		cvtColor(frame, current_gray, CV_RGB2GRAY);

		if(remainFrame != 0){
			int correct_num = 0;
			for(size_t i = 0; i < vtld_info.size(); i++) {
				char filePath[30];
				memset(filePath, '\0', 30);
				TLD_Info tempTLD_Info = vtld_info[i];
				tempTLD_Info.tld->processFrame(last_gray, current_gray, pts1, pts2, pbox, status, tl);

				// success tracking
				if (status) {
					pbox.x = pbox.x - 20;
					pbox.y = pbox.y - 20;
					pbox.width = pbox.width + 40;
					pbox.height = pbox.height + 40;
					Mat face_image = frame(pbox);
					sprintf(filePath, "./Crop_Image/%03d/%03d.jpg", tempTLD_Info.dirCounter, tempTLD_Info.fileCounter);
					imwrite(filePath, face_image);
					++tempTLD_Info.fileCounter;
					vtld_info[i] = tempTLD_Info;
					//drawPoints(frame, pts1);
					//drawPoints(frame, pts2, Scalar(0, 255, 0));
					//drawBox(frame, pbox);
					pts1.clear();
					pts2.clear();
					++correct_num;
				}
			}

			if(correct_num == vtld_info.size()){
				remainFrame = 3;
			}
			else{
				--remainFrame;
			}
			swap(last_gray, current_gray);
		}
		else{
			// erase vector
			vector<Point2f>().swap(pts1);
			vector<Point2f>().swap(pts2);
			freeTLD_Info(vtld_info);
			vector<TLD_Info>().swap(vtld_info);
			vector<Rect>().swap(faces);
			createDirectory(capture, fs, last_gray, faces, vtld_info, dirCounter);
			remainFrame = 3;
		}
		//imshow("Test", frame);
		//waitKey(1);
	}
	system("pause");
	return 0;
}

void read_options(int argc, char** argv, VideoCapture &capture, int &skip_msecs){
	for(size_t i = 0; i < argc; i++) {
		if (strcmp(argv[i],"-s") == 0) {
			if (argc > i) {
				string video = string(argv[i+1]);
				capture.open(video);
			}
		}
		if(strcmp(argv[i], "-t") == 0) {
			if(argc > i) {
				skip_msecs = atoi(argv[i+1]);
			}
		}
	}
}

void createDirectory(VideoCapture &capture, FileStorage &fs, Mat &last_gray, vector<Rect> &faces, vector<TLD_Info> &vtld_info, int &dirCounter){
	Mat frame;
	while (faces.size() == 0) {
		capture >> frame;
		cvtColor(frame, last_gray, CV_RGB2GRAY);
		faces = faceDetection(last_gray);
	}

	// create directory && initialize TLD && set up parameters
	char dirPath[30];
	memset(dirPath, '\0', 30);
	for(size_t i = 0; i < faces.size(); i++) {
		TLD_Info tempTLD_Info;
		TLD *tempTLD = new TLD();
		tempTLD->read(fs.getFirstTopLevelNode());
		tempTLD->init(last_gray, faces[i]);
		setTLD_Info(tempTLD_Info, tempTLD, dirCounter, 1);
		sprintf(dirPath, "./Crop_Image/%03d/", dirCounter);
		mkdir(dirPath, 0700);
		++dirCounter;
		vtld_info.push_back(tempTLD_Info);
		//vTLD.push_back(tld);
	}
}

void setTLD_Info(TLD_Info &tld_info, TLD *tld, int dirCounter, int fileCounter){
	tld_info.tld = tld;
	tld_info.dirCounter = dirCounter;
	tld_info.fileCounter = fileCounter;
}

void freeTLD_Info(vector<TLD_Info> &vtld_info){
	for(size_t i = 0; i < vtld_info.size(); i++){
		delete vtld_info[i].tld;
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
