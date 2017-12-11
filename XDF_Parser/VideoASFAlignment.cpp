#include "VideoASFAlignment.h"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "XDFParser.h"

namespace videopruning {
using namespace cv;


VideoAligment::VideoAligment()
{
	cout << "Enter path to input video file:" << endl;
	cin >> _inputVideoFile;
	//_inputVideoFile = "C:\\Shared\\Pilot_16_11_16\\01.asf";
	cout << "Enter path to output video file:" << endl;
	//_outputVideoFile = "C:\\Shared\\Pilot_16_11_16\\01_out.asf";
	cin >> _outputVideoFile;
	_frameRate = 0;
}

VideoAligment::~VideoAligment()
{
}

void VideoAligment::VideoPruningByFramesNumber(vector <unsigned int> framesNumber)
{
	VideoCapture inputVideo(_inputVideoFile);
	ENSURE_EX(inputVideo.isOpened(), "Video can not be opened");
	
	Size frameSize(inputVideo.get(CV_CAP_PROP_FRAME_WIDTH), inputVideo.get(CV_CAP_PROP_FRAME_HEIGHT));
	double fps = 30;//inputVideo.get(CV_CAP_PROP_FPS);

	VideoWriter outputVideo;
	double codec = inputVideo.get(CV_CAP_PROP_FOURCC);
	outputVideo.open(_outputVideoFile, CV_FOURCC('M', 'P', '4', '2'), fps, frameSize);  //MPEG-4 codec
	
	double framePosition = static_cast<double>(framesNumber[0]);
	ENSURE_EX(inputVideo.set(CV_CAP_PROP_POS_FRAMES, framePosition), "Frame doesn't exist");
	cv::Mat currentFrame;
	for (int i = 0; i < framesNumber.size(); i++)
	{
		/*
		double framePosition = static_cast<double>(framesNumber[i]);
		ENSURE_EX(inputVideo.set(CV_CAP_PROP_POS_FRAMES, framePosition), "Frame doesn't exist");
		inputVideo >> currentFrame;
		outputVideo.write(currentFrame);
		*/

		inputVideo >> currentFrame;
		outputVideo << currentFrame;
		
	}
	inputVideo.release();
	outputVideo.release();
}

}