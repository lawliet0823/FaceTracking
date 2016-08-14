#include "TLD.h"
#include "tld_utils.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/objdetect/objdetect.hpp>
#include <sstream>
#include <sys/stat.h>

//#include "CompressiveTracker.h"

using namespace cv;
using namespace std;

struct TLD_Info {
  TLD *tld;
  int dirCounter;
  int fileCounter;
  int remainFrame;
};

// Global Variables
// Face model
const String face_cascade_name = "haarcascade_frontalface_alt.xml";

void read_options(int, char **, VideoCapture &, int &, string &);
// Initialize TLD tracker, Information, File Directory
void createDirectory(VideoCapture &, FileStorage &, Mat &, vector<Rect> &,
                     vector<TLD_Info> &, int &, double &, string);
void setTLD_Info(TLD_Info &, TLD *, int, int, int);
void freeTLD_Info(vector<TLD_Info> &);
vector<Rect> faceDetection(Mat);

int main(int argc, char **argv) {
  // parameters setting
  VideoCapture capture;
  // vector<TLD*> vTLD;
  vector<TLD_Info> vtld_info;
  FileStorage fs;
  fs.open("parameters.yml", FileStorage::READ);
  int skip_msecs = 0;
  int dirCounter = 1;
  double frame_num = 0;
  string dirName = "Crop_Image";

  // get program parameters
  read_options(argc, argv, capture, skip_msecs, dirName);
  if (!capture.isOpened()) {
    cout << "VideoCapture Fail !!!" << endl;
    return 1;
  }
  capture.set(CV_CAP_PROP_POS_MSEC, skip_msecs);

  cout << "Directory Name: " << dirName << endl;
  // namedWindow("Test", WINDOW_AUTOSIZE);

  Mat frame;
  Mat last_gray;
  // save detect face
  vector<Rect> faces;

  char mainDir[30];
  memset(mainDir, '\0', 30);
  sprintf(mainDir, "./%s/", dirName.c_str());
  mkdir(mainDir, 0700);
  cout << "Begin Tracking" << endl;
  createDirectory(capture, fs, last_gray, faces, vtld_info, dirCounter,
                  frame_num, dirName);

  Mat current_gray;
  BoundingBox pbox;
  vector<Point2f> pts1;
  vector<Point2f> pts2;
  bool status = true;
  bool tl = true;

  int remainFrame = 5;

  while (capture.read(frame)) {
    // frame_num += 3;
    // capture.set(CV_CAP_PROP_POS_FRAMES, frame_num);
    cvtColor(frame, current_gray, CV_RGB2GRAY);
    if (remainFrame > 0 && vtld_info.size() > 0) {
      // correct_num caculate how many faces should we track
      int correct_num = 0;
      for (size_t i = 0; i < vtld_info.size(); i++) {
        char filePath[30];
        memset(filePath, '\0', 30);
        TLD_Info tempTLD_Info = vtld_info[i];
        tempTLD_Info.tld->processFrame(last_gray, current_gray, pts1, pts2,
                                       pbox, status, tl);

        // success tracking
        if (status) {
          if (pbox.x < 0 || pbox.y < 0 || (pbox.x + pbox.width) > frame.cols ||
              (pbox.y + pbox.height) > frame.rows) {
            delete vtld_info[i].tld;
            vtld_info.erase(vtld_info.begin() + i);
            continue;
          }

          Mat face_image = frame(pbox);

          // Test Code

          if (tempTLD_Info.fileCounter % 5 == 0) {
            vector<Rect>().swap(faces);
            faces = faceDetection(face_image);
            if (faces.size() == 0 && faces.size() != vtld_info.size()) {
              correct_num = -1; // if (correct_num != vtld_info.size()), stop
                                // tracking
              remainFrame = -1;
              pts1.clear();
              pts2.clear();
              break;
            }
          }
          sprintf(filePath, "./%s/%03d/%03d.jpg", dirName.c_str(),
                  tempTLD_Info.dirCounter, tempTLD_Info.fileCounter);
          imwrite(filePath, face_image);
          ++tempTLD_Info.fileCounter;
          vtld_info[i] = tempTLD_Info;
          // show video tracking state
          /*
          drawPoints(frame, pts1);
          drawPoints(frame, pts2, Scalar(0, 255, 0));
          drawBox(frame, pbox);
          imshow("Tracking", frame);
          // char tempChar[20];
          // sprintf(tempChar, "%f.jpg", frame_num);
          // imwrite(tempChar, frame);
          if (waitKey(10) >= 0) {
            break;
          }
          */
          ++correct_num;
        } else {
          --correct_num;
        }
      }

      pts1.clear();
      pts2.clear();

      // if program can't detect
      if (correct_num == vtld_info.size()) {
        remainFrame = 5;
      } else {
        --remainFrame;
      }
      // swap frame
      // cout << "Correct Number: " << correct_num << " Vector size " <<
      // vtld_info.size() << " Remain Frame: " << remainFrame << endl;
      swap(last_gray, current_gray);
    } else {
      // erase vector
      vector<Point2f>().swap(pts1);
      vector<Point2f>().swap(pts2);
      freeTLD_Info(vtld_info);
      vector<TLD_Info>().swap(vtld_info);
      vector<Rect>().swap(faces);
      createDirectory(capture, fs, last_gray, faces, vtld_info, dirCounter,
                      frame_num, dirName);
      remainFrame = 5;
    }
    // imshow("Test", frame);
    // waitKey(1);
  }
  system("pause");
  return 0;
}

void read_options(int argc, char **argv, VideoCapture &capture, int &skip_msecs,
                  string &dirName) {
  for (size_t i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-s") == 0) {
      if (argc > i) {
        string video = string(argv[i + 1]);
        capture.open(video);
      }
    }
    if (strcmp(argv[i], "-t") == 0) {
      if (argc > i) {
        skip_msecs = atoi(argv[i + 1]);
      }
    }
    if (strcmp(argv[i], "-f") == 0) {
      if (argc > i) {
        dirName = string(argv[i + 1]);
      }
    }
  }
}

void createDirectory(VideoCapture &capture, FileStorage &fs, Mat &last_gray,
                     vector<Rect> &faces, vector<TLD_Info> &vtld_info,
                     int &dirCounter, double &frame_num, string dirName) {
  Mat frame;
  while (faces.size() == 0) {
    capture >> frame;
    // frame_num += 3;
    // capture.set(CV_CAP_PROP_POS_FRAMES, frame_num);
    cvtColor(frame, last_gray, CV_RGB2GRAY);
    faces = faceDetection(last_gray);
  }

  // create directory && initialize TLD && set up parameters
  char dirPath[30];
  char filePath[30];
  memset(dirPath, '\0', 30);
  memset(filePath, '\0', 30);

  for (size_t i = 0; i < faces.size(); i++) {
    // increase face size to avoid detection fail
    faces[i].x = faces[i].x - 70;
    faces[i].y = faces[i].y - 70;
    faces[i].width = faces[i].width + 140;
    faces[i].height = faces[i].height + 140;
    if (faces[i].x < 0 || faces[i].y < 0 ||
        (faces[i].x + faces[i].width) > frame.cols ||
        (faces[i].y + faces[i].height) > frame.rows) {
      faces.erase(faces.begin() + i);
      continue;
    }

    TLD_Info tempTLD_Info;
    TLD *tempTLD = new TLD();
    tempTLD->read(fs.getFirstTopLevelNode());
    tempTLD->init(last_gray, faces[i]);
    // set TLD information: TLD dirCounter fileCounter remainFrame
    setTLD_Info(tempTLD_Info, tempTLD, dirCounter, 2, 4);
    sprintf(dirPath, "./%s/%03d/", dirName.c_str(), dirCounter);
    mkdir(dirPath, 0700);

    Mat face_image = frame(faces[i]);
    sprintf(filePath, "./%s/%03d/%03d.jpg", dirName.c_str(), dirCounter, 1);
    imwrite(filePath, face_image);

    ++dirCounter;
    vtld_info.push_back(tempTLD_Info);
    // vTLD.push_back(tld);
  }
}

void setTLD_Info(TLD_Info &tld_info, TLD *tld, int dirCounter, int fileCounter,
                 int remainFrame) {
  tld_info.tld = tld;
  tld_info.dirCounter = dirCounter;
  tld_info.fileCounter = fileCounter;
  tld_info.remainFrame = remainFrame;
}

void freeTLD_Info(vector<TLD_Info> &vtld_info) {
  for (size_t i = 0; i < vtld_info.size(); i++) {
    delete vtld_info[i].tld;
  }
}

vector<Rect> faceDetection(Mat frame) {
  CascadeClassifier face_cascade;
  vector<Rect> faces;

  if (!face_cascade.load(face_cascade_name)) {
    printf("Error Loading");
  }

  face_cascade.detectMultiScale(frame, faces, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE,
                                cvSize(90, 90));
  return faces;
}
