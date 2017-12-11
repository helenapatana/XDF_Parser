#include <vector>
#include <string>

namespace videopruning
{
using namespace std;

class VideoAligment
{
public:
	VideoAligment();
	~VideoAligment();
	void VideoPruningByFramesNumber(vector <unsigned int> framesNumber);
private:
	string _inputVideoFile;
	string _outputVideoFile;
	double _frameRate;
	
};
}
