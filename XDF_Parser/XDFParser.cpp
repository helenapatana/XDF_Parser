#include "XDFParser.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "rapidxml.hpp"

namespace xdfparser {

using namespace rapidxml;
using namespace std;

////

class FileFromBuffer
{

public:

	FileFromBuffer(const unsigned char* bufferPtr, size_t bufferSize)
		: bufferPtr(bufferPtr), bufferEnd(bufferPtr + bufferSize) {}

	friend inline size_t fread(void* dest, size_t elemSize, size_t elemCount, FileFromBuffer& self)
	{
		size_t availSize = self.bufferEnd - self.bufferPtr;

		size_t requestSize = elemSize * elemCount;

		if (!(requestSize <= availSize))
			return 0;

		memcpy(dest, self.bufferPtr, requestSize);
		self.bufferPtr += requestSize;
		return elemCount;
	}

	friend inline bool feof(FileFromBuffer& self)
	{
		return self.bufferPtr == self.bufferEnd;
	}

private:

	const unsigned char* bufferPtr;
	const unsigned char* bufferEnd;

};

const map <string, int> dataTypeMap = { { "float32", sizeof(float) },
										{ "int32",   sizeof(int) },
										{ "uint8",   sizeof(unsigned char) },
										{ "double",  sizeof(double) } };

XDFParser::XDFParser()
{
	cout << "Enter path to exf file:" << endl;
	cin >> _pfileName;
	//_pfileName = "C:\\Shared\\Pilot_16_11_16\\01.xdf";
	channelsHeader.reserve(3);
}

int ReadVarlen(FileFromBuffer& f)
{
	unsigned char capacity = 0;
	if (fread(&capacity, sizeof(capacity), 1, f) != 1)
	{
		ENSURE(feof(f));
		return 0;
	}
	
	int num = 0;

	unsigned char ucharValue = 0;
	unsigned int uintValue = 0;
	unsigned long int ulongValue = 0;

	switch (capacity)
	{
	case 1:
		ENSURE(fread(&ucharValue, sizeof(ucharValue), 1, f) == 1);
		num = ucharValue;
		break;
	case 4:
		ENSURE(fread(&uintValue, sizeof(uintValue), 1, f) == 1);
		num = uintValue;
		break;
	case 8:
		ENSURE(fread(&ulongValue, sizeof(ulongValue), 1, f) == 1);
		num = ulongValue;
		break;
	};
	
	return num;
}

int XDFParser::GetVectorIndexStream(int streamID)
{
	for (int i = 0; i < channelsHeader.size(); i++)
	{
		if (channelsHeader[i].idStream == streamID)
		{
			return i;
		}
	}
	return -1;
}

string XDFParser::GetNameStream(int streamID)
{
	string name;
	for (auto &elem : channelsHeader)
	{
		if (elem.idStream == streamID)
		{
			return elem.name;
		}
	}
	return name;
}

void XDFParser::ExtractValuesAndTimes()
{
	FILE *xdfBasicFile  = fopen(_pfileName.c_str(), "rb");
	ENSURE_EX(xdfBasicFile, "File can not be opened: " + _pfileName);

	////

	ENSURE(fseek(xdfBasicFile, 0, SEEK_END) == 0);
	long long fileSize = _ftelli64(xdfBasicFile);
	ENSURE(fileSize >= 0 && fileSize <= PTRDIFF_MAX);
	ENSURE(fseek(xdfBasicFile, 0, SEEK_SET) == 0);

	vector<unsigned char> fileContent(fileSize);
	ENSURE(fread(&fileContent[0], 1, fileSize, xdfBasicFile) == fileSize);

	FileFromBuffer xdfFile(&fileContent[0], fileContent.size());

	////


	char tmpBuff[5];
	ENSURE(fread(&tmpBuff, sizeof(char), 4, xdfFile) == 4);
	tmpBuff[4] = '\0';
	ENSURE_EX((strcmp(tmpBuff, "XDF:") == 0), "This is not a valid XDF file");

    vector <char> fileHeader;
	vector <char> strmHeader;
	
	bool unsupportedBlockEncountered = false;
	
	while (true)
	{
		int len = ReadVarlen(xdfFile);
		
		if (!len)
			break;

		unsigned short int blockType;
		ENSURE(fread(&blockType, sizeof(unsigned short int), 1, xdfFile) == 1);

		switch (blockType)
		{
		case 1:  //file header chunk
		{	
			ENSURE(!unsupportedBlockEncountered);
			fileHeader.resize(len);
			ENSURE(len >= 2);
			ENSURE(fread(&fileHeader[0], sizeof(char), len - 2, xdfFile) == len - 2);
			break;
		}
		case 2:  //stream header chunck
		{
			ENSURE(!unsupportedBlockEncountered);
			ENSURE(fread(&streamHeader.idStream, sizeof(unsigned int), 1, xdfFile) == 1);
			ENSURE(len >= 6);
			strmHeader.resize(len - 6);
			ENSURE(fread(&strmHeader[0], sizeof(char), len - 6, xdfFile) == len - 6);
			strmHeader.push_back('\0');
			xml_document<> doc;    // character type defaults to char
			doc.parse<0>(&strmHeader[0]);    // 0 means default parse flags
			xml_node<> *node = doc.first_node();

			for (xml_node<> *child = node->first_node(); child; child = child->next_sibling())
			{
				string name = child->name();
				string value = child->value();
				if (!strcmp(name.c_str(), "name"))
				{
					streamHeader.name = value;
				}
				if (!strcmp(name.c_str(), "type"))
				{
					streamHeader.type = value;
				}
				if (!strcmp(name.c_str(), "channel_count"))
				{
					streamHeader.channelCount = atoi(value.c_str());
				}
				if (!strcmp(name.c_str(), "nominal_srate"))
				{
					streamHeader.nominalSrate = atoi(value.c_str());
				}
				if (!strcmp(name.c_str(), "channel_format"))
				{
					streamHeader.dataFormat = value;
				}
			}
			streamHeader.sizeElement = dataTypeMap.at(streamHeader.dataFormat);
			channelsHeader.push_back(streamHeader);
			allStreamData.resize(channelsHeader.size());
			strmHeader.clear();
			break;
		}
		case 3:  // sample chunk
		{
			ENSURE(!unsupportedBlockEncountered);

			unsigned int streamID;
			ENSURE(fread(&streamID, sizeof(unsigned int), 1, xdfFile) == 1);
			int chunckSize = ReadVarlen(xdfFile);
			const streamHeadersInfo& currStreamInfo = channelsHeader[GetVectorIndexStream(streamID)];
			
			streamData& strmData = allStreamData.at(streamID - 1);
			strmData.streamInfo = currStreamInfo;

			for (int i = 0; i < chunckSize; i++)
			{
				//read or deduce time stamp
				unsigned char nextChunck = 0;
				ENSURE(fread(&nextChunck, sizeof(unsigned char), 1, xdfFile) == 1);
				if (nextChunck)
				{
					double currTimeStamp = 0;
					ENSURE(fread(&currTimeStamp, sizeof(double), 1, xdfFile) == 1);
					strmData.timeStamps.push_back(currTimeStamp);
				}
				else
				{
					if (strmData.timeStamps.empty())
						strmData.timeStamps.push_back(0);
					else
						strmData.timeStamps.push_back(strmData.timeStamps.back() + (1.0 / currStreamInfo.nominalSrate));
				}
				// values reading
				if (!currStreamInfo.dataFormat.compare("float32"))
				{
					strmData.fValues.emplace_back(currStreamInfo.channelCount);
					auto& dest = strmData.fValues.back();
					ENSURE(fread(&dest[0], strmData.streamInfo.sizeElement, currStreamInfo.channelCount, xdfFile) == currStreamInfo.channelCount);
				}
				else if (!currStreamInfo.dataFormat.compare("int32"))
				{
					strmData.uiValues.emplace_back(currStreamInfo.channelCount);
					auto& dest = strmData.uiValues.back();
					ENSURE(fread(&dest[0], strmData.streamInfo.sizeElement, currStreamInfo.channelCount, xdfFile) == currStreamInfo.channelCount);
				}
				else
				{
					ENSURE_EX(false, "Unsupported data type");
				}
			}
			break;
		}
		case 4:
		{
			uint32_t itmp = 0;
			ENSURE(fread(&itmp, sizeof(uint32_t), 1, xdfFile) == 1);
			double dtmp[2];
			ENSURE(fread(dtmp, sizeof(double), 2, xdfFile) == 2);
			break;
		}
		case 6:
		{
			uint32_t itmp = 0;
			ENSURE(fread(&itmp, sizeof(uint32_t), 1, xdfFile) == 1);
			ENSURE(len >= 6);
			vector <char> tmpVector(len - 6);
			ENSURE(fread(&tmpVector[0], sizeof(char), len - 6, xdfFile) == len - 6);
			break;
		}
		default:
			ENSURE(len >= 2);
			vector <unsigned char> tmpVector(len - 2);
			ENSURE(fread(&tmpVector[0], sizeof(unsigned char), len - 2, xdfFile) == len - 2);
			break;

		}
	}
	fclose(xdfBasicFile);
}

int XDFParser::GetStreamNumber()
{
	ENSURE_EX( allStreamData.size() > 0, "No streams data. Please, load xdf file.");
	return allStreamData.size();
}

int XDFParser::GetChannelNumberInStream(int streamID)
{
	ENSURE_EX(allStreamData.size() > 0, "No streams data. Please, load xdf file.");
	for (int i = 0; i < allStreamData.size(); i++)
	{
		if (allStreamData[i].streamInfo.idStream == streamID)
		{
			if (!allStreamData[i].streamInfo.dataFormat.compare("float32"))
			{
				return allStreamData[i].fValues[0].size();
			}
			if (!allStreamData[i].streamInfo.dataFormat.compare("int32"))
			{
				return allStreamData[i].uiValues[0].size();
			}
		}
	}
	ENSURE_EX(false, "Stream index is not found");
}

void XDFParser::GetSynchronyzedVideoFrames(string nameStream, vector<unsigned int>& values, int indexChannel)
{
	int ind;
	for (ind = 0; ind < allStreamData.size(); ind++)
	{
		if (!allStreamData[ind].streamInfo.name.compare(nameStream))
		{
			for (int i = 0; i < allStreamData[ind].uiValues.size(); i++)
			{
				values.push_back(allStreamData[ind].uiValues[i][indexChannel]);
			}
			return;
		}
	}
	ENSURE_EX(false, "Stream name is not found");
}






}
