#include <iostream>
#include <conio.h>
#include "XDFParser.h"
#include "VideoASFAlignment.h"

using namespace std;
using namespace xdfparser;
using namespace videopruning;

void my_main()
{
	
	XDFParser parser = XDFParser();
	parser.ExtractValuesAndTimes();
	vector <unsigned int> synchronyzedVideoFrames;
	parser.GetSynchronyzedVideoFrames(streamNames::video, synchronyzedVideoFrames);
	
	VideoAligment testVideo = VideoAligment();
	testVideo.VideoPruningByFramesNumber(synchronyzedVideoFrames);
	
	cout << "Done" << endl;
	_getch();
}

int main()
{
	try 
	{
		my_main();
	}
	catch (const string &message)
	{
		cout << message << endl;
		return EXIT_FAILURE;
	}
	catch (const exception &message)
	{
		cout << "stl exception: " << message.what() << endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}