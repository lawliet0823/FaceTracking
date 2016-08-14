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
  int timeName;
  int fileCounter;
  int remainFrame;
};

// Global Variables
// Face model
const String face_cascade_name = "haarcascade_frontalface_alt.xml";

void read_options(int, char **, VideoCapture &, int &, string &);
// Initialize TLD tracker, Information, File Directory
void createDirectory(VideoCapture &, FileStorage &, Mat &, vector<Rect> &,
                     vector<TLD_Info> &, double &, string);
void setTLD_Info(TLD_Info &, TLD *, int, int, int);
void freeTLD_Info(vector<TLD_Info> &);
vector<Rect> faceDetection(Mat);

int main(int argc, char **argv) {
  // parameters setting
  VideoCapture capture;
  vector<TLD_Info> vtld_info;
  FileStorage fs;
  fs.open("parameters.yml", FileStorage::READ);
  int skip_msecs = 0;
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
  createDirectory(capture, fs, last_gray, faces, vtld_info, frame_num, dirName);

  Mat current_gray;
  BoundingBox pbox;
  vector<Point2f> pts1;
  vector<Point2f> pts2;
  bool status = true;
  bool tl = true;

  int remainFrame = 3;

  while (capture.read(frame)) {
    cout << static_cast<int>(capture.get(CV_CAP_PROP_POS_MSEC));
    frame_num += 2;
    capture.set(CV_CAP_PROP_POS_FRAMES, frame_num);
    cvtColor(frame, current_gray, CV_RGB2GRAY);
    faces = faceDetection(frame);
    if (faces.size() != vtld_info.size()) {
      remainFrame--;
    }

    if (remainFrame > 0) {
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
          Mat face_image = frame(pbox);

          if (tempTLD_Info.fileCounter % 5 == 0) {
            faces = faceDetection(face_image);
            if (faces.size() == 0) {
              --correct_num; // if (correct_num != vtld_info.size()), stop
                             // tracking
              remainFrame = 0;
              break;
            }
          }

          sprintf(filePath, "./%s/%03d/%03d.jpg", dirName.c_str(),
                  tempTLD_Info.timeName, tempTLD_Info.fileCounter);
          imwrite(filePath, face_image);
          ++tempTLD_Info.fileCounter;
          vtld_info[i] = tempTLD_Info;
          // show video tracking state
          /*
          drawPoints(frame, pts1);
          drawPoints(frame, pts2, Scalar(0, 255, 0));
          drawBox(frame, pbox);

          imshow("Tracking", frame);
          if (waitKey(10) >= 0) {
            break;
          }
          */
          pts1.clear();
          pts2.clear();
          ++correct_num;
        }
      }

      // if program can't detect
      if (correct_num == vtld_info.size()) {
        remainFrame = 3;
      } else {
        --remainFrame;
      }

      swap(last_gray, current_gray);
    } else {
      // erase vector
      vector<Point2f>().swap(pts1);
      vector<Point2f>().swap(pts2);
      freeTLD_Info(vtld_info);
      vector<TLD_Info>().swap(vtld_info);
      vector<Rect>().swap(faces);
      createDirectory(capture, fs, last_gray, faces, vtld_info, frame_num,
                      dirName);
      remainFrame = 3;
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
                     double &frame_num, string dirName) {
  Mat frame;
  while (faces.size() == 0) {
    capture >> frame;
    frame_num += 2;
    capture.set(CV_CAP_PROP_POS_FRAMES, frame_num);
    cvtColor(frame, last_gray, CV_RGB2GRAY);
    faces = faceDetection(last_gray);
  }

  // create directory && initialize TLD && set up parameters
  char dirPath[30];
  char filePath[30];
  memset(dirPath, '\0', 30);
  memset(filePath, '\0', 30);
  int prevsecond = 0;
  for (size_t i = 0; i < faces.size(); i++) {
    // increase face size to avoid detection fail
    faces[i].x = faces[i].x - 70;
    faces[i].y = faces[i].y - 70;
    faces[i].width = faces[i].width + 140;
    faces[i].height = faces[i].height + 140;
    if (faces[i].x < 0 || faces[i].y < 0 ||
        (faces[i].x + faces[i].width) > frame.cols ||
        (faces[i].y + faces[i].height) > frame.rows) {
      continue;
    }

    TLD_Info tempTLD_Info;
    TLD *tempTLD = new TLD();
    tempTLD->read(fs.getFirstTopLevelNode());
    tempTLD->init(last_gray, faces[i]);
    // set TLD information: TLD dirCounter fileCounter remainFrame
    int msecond = static_cast<int>(capture.get(CV_CAP_PROP_POS_MSEC));
    if (msecond == prevsecond) {
      msecond += 1;
      prevsecond = msecond;
    } else {
      prevsecond = msecond;
    }

    setTLD_Info(tempTLD_Info, tempTLD, msecond, 2, 3);
    sprintf(dirPath, "./%s/%d/", dirName.c_str(), msecond);
    mkdir(dirPath, 0700);

    Mat face_image = frame(faces[i]);
    sprintf(filePath, "./%s/%03d/%03d.jpg", dirName.c_str(), msecond, 1);
    imwrite(filePath, face_image);

    vtld_info.push_back(tempTLD_Info);
    // vTLD.push_back(tld);
  }
}

void setTLD_Info(TLD_Info &tld_info, TLD *tld, int timeName, int fileCounter,
                 int remainFrame) {
  tld_info.tld = tld;
  tld_info.timeName = timeName;
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
