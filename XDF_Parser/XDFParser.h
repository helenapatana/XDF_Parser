#pragma once
#include <stdio.h>
#include <vector>
#include <string>
#include <map>

namespace xdfparser {

using namespace std;

////

#define ENSURE_EX(condition, msg) \
    if (condition) ; else throw string(msg)

#define ENSURE(condition) \
    ENSURE_EX(condition, __FILE__ "(" STRINGIZE(__LINE__) "): " STRINGIZE(condition) " failed")

#define STRINGIZE(x) \
    STRINGIZE2(x)

#define STRINGIZE2(x) \
    #x


struct streamHeadersInfo
{
	unsigned int idStream;
	string name;
	string type;
	int channelCount;
	int nominalSrate;
	string dataFormat;
	int sizeElement;
};

struct streamData
{
	streamHeadersInfo streamInfo;
	vector <double> timeStamps;
	vector <vector<float>> fValues;
	vector <vector<unsigned int>> uiValues;
};

namespace streamNames 
{
	const string video  = "VideoStream_";
	const string audio  = "AudioCaptureWin";
	const string kinect = "KinectMocap0";
}

extern const map <string, int> dataTypeMap;

class XDFParser
{

public:
	XDFParser();
	void ExtractValuesAndTimes();
	int GetStreamNumber();
	int GetChannelNumberInStream(int streamID);
	void GetSynchronyzedVideoFrames(string nameStream, vector<unsigned int>& values, int indexChannel = 0);

private:
	string _pfileName;
	streamHeadersInfo streamHeader;
	vector <streamHeadersInfo> channelsHeader;
	vector <streamData> allStreamData;
	int GetVectorIndexStream(int streamID);
	string GetNameStream(int streamID);

};

}

using xdfparser::XDFParser;
