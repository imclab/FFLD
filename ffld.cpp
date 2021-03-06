//--------------------------------------------------------------------------------------------------
// Implementation of the paper "Exact Acceleration of Linear Object Detectors", 12th European
// Conference on Computer Vision, 2012.
//
// Copyright (c) 2012 Idiap Research Institute, <http://www.idiap.ch/>
// Written by Charles Dubout <charles.dubout@idiap.ch>
//
// This file is part of FFLD (the Fast Fourier Linear Detector)
//
// FFLD is free software: you can redistribute it and/or modify it under the terms of the GNU
// General Public License version 3 as published by the Free Software Foundation.
//
// FFLD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
// Public License for more details.
//
// You should have received a copy of the GNU General Public License along with FFLD. If not, see
// <http://www.gnu.org/licenses/>.
//--------------------------------------------------------------------------------------------------


#include "ffld.h"

using namespace FFLD;
using namespace jake;

inline void FFLDObjDetector::start()
{
	gettimeofday(&Start, 0);
}

inline int FFLDObjDetector::stop()
{
	gettimeofday(&Stop, 0);
	
	timeval duration;
	timersub(&Stop, &Start, &duration);
	
	return duration.tv_sec * 1000 + (duration.tv_usec + 500) / 1000;
}

	FFLDObjDetector::Detection::Detection() : score(0), l(0), x(0), y(0)
	{
	}
	
	//rectangles copy constructor is being called
	
	FFLDObjDetector::Detection::Detection(HOGPyramid::Scalar score, int l, int x, int y, FFLD::Rectangle bndbox) :
	FFLD::Rectangle(bndbox), score(score), l(l), x(x), y(y)
	{
	}
	
	bool FFLDObjDetector::Detection::operator<(const Detection & detection) const
	{
		return score > detection.score;
	}


using namespace std;
using namespace cv;


void FFLDObjDetector::showUsage()
{
	cout << "Incorrect usage, just run the executable using \"./\" "
		 << endl;
}

void FFLDObjDetector::draw(JPEGImage & image, const FFLD::Rectangle & rect, uint8_t r, uint8_t g, uint8_t b,
		  int linewidth)
{
	if (image.empty() || rect.empty() || (image.depth() < 3))
		return;
	
	const int width = image.width();
	const int height = image.height();
	const int depth = image.depth();
	uint8_t * bits = image.bits();
	
	// Draw 2 horizontal lines
	const int top = min(max(rect.top(), 1), height - linewidth - 1);
	const int bottom = min(max(rect.bottom(), 1), height - linewidth - 1);
	
	for (int x = max(rect.left() - 1, 0); x <= min(rect.right() + linewidth, width - 1); ++x) {
		if ((x != max(rect.left() - 1, 0)) && (x != min(rect.right() + linewidth, width - 1))) {
			for (int i = 0; i < linewidth; ++i) {
				bits[((top + i) * width + x) * depth    ] = r;
				bits[((top + i) * width + x) * depth + 1] = g;
				bits[((top + i) * width + x) * depth + 2] = b;
				bits[((bottom + i) * width + x) * depth    ] = r;
				bits[((bottom + i) * width + x) * depth + 1] = g;
				bits[((bottom + i) * width + x) * depth + 2] = b;
			}
		}
		
		// Draw a white line below and above the line
		if ((bits[((top - 1) * width + x) * depth    ] != 255) &&
			(bits[((top - 1) * width + x) * depth + 1] != 255) &&
			(bits[((top - 1) * width + x) * depth + 2] != 255))
			for (int i = 0; i < 3; ++i)
				bits[((top - 1) * width + x) * depth + i] = 255;
		
		if ((bits[((top + linewidth) * width + x) * depth    ] != 255) &&
			(bits[((top + linewidth) * width + x) * depth + 1] != 255) &&
			(bits[((top + linewidth) * width + x) * depth + 2] != 255))
			for (int i = 0; i < 3; ++i)
				bits[((top + linewidth) * width + x) * depth + i] = 255;
		
		if ((bits[((bottom - 1) * width + x) * depth    ] != 255) &&
			(bits[((bottom - 1) * width + x) * depth + 1] != 255) &&
			(bits[((bottom - 1) * width + x) * depth + 2] != 255))
			for (int i = 0; i < 3; ++i)
				bits[((bottom - 1) * width + x) * depth + i] = 255;
		
		if ((bits[((bottom + linewidth) * width + x) * depth    ] != 255) &&
			(bits[((bottom + linewidth) * width + x) * depth + 1] != 255) &&
			(bits[((bottom + linewidth) * width + x) * depth + 2] != 255))
			for (int i = 0; i < 3; ++i)
				bits[((bottom + linewidth) * width + x) * depth + i] = 255;
	}
	
	// Draw 2 vertical lines
	const int left = min(max(rect.left(), 1), width - linewidth - 1);
	const int right = min(max(rect.right(), 1), width - linewidth - 1);
	
	for (int y = max(rect.top() - 1, 0); y <= min(rect.bottom() + linewidth, height - 1); ++y) {
		if ((y != max(rect.top() - 1, 0)) && (y != min(rect.bottom() + linewidth, height - 1))) {
			for (int i = 0; i < linewidth; ++i) {
				bits[(y * width + left + i) * depth    ] = r;
				bits[(y * width + left + i) * depth + 1] = g;
				bits[(y * width + left + i) * depth + 2] = b;
				bits[(y * width + right + i) * depth    ] = r;
				bits[(y * width + right + i) * depth + 1] = g;
				bits[(y * width + right + i) * depth + 2] = b;
			}
		}
		
		// Draw a white line left and right the line
		if ((bits[(y * width + left - 1) * depth    ] != 255) &&
			(bits[(y * width + left - 1) * depth + 1] != 255) &&
			(bits[(y * width + left - 1) * depth + 2] != 255))
			for (int i = 0; i < 3; ++i)
				bits[(y * width + left - 1) * depth + i] = 255;
		
		if ((bits[(y * width + left + linewidth) * depth    ] != 255) &&
			(bits[(y * width + left + linewidth) * depth + 1] != 255) &&
			(bits[(y * width + left + linewidth) * depth + 2] != 255))
			for (int i = 0; i < 3; ++i)
				bits[(y * width + left + linewidth) * depth + i] = 255;
		
		if ((bits[(y * width + right - 1) * depth    ] != 255) &&
			(bits[(y * width + right - 1) * depth + 1] != 255) &&
			(bits[(y * width + right - 1) * depth + 2] != 255))
			for (int i = 0; i < 3; ++i)
				bits[(y * width + right - 1) * depth + i] = 255;
		
		if ((bits[(y * width + right + linewidth) * depth    ] != 255) &&
			(bits[(y * width + right + linewidth) * depth + 1] != 255) &&
			(bits[(y * width + right + linewidth) * depth + 2] != 255))
			for (int i = 0; i < 3; ++i)
				bits[(y * width + right + linewidth) * depth + i] = 255;
	}
}
void FFLDObjDetector::detectObjects(const Mixture & mixture, int width, int height, const HOGPyramid & pyramid,
			double threshold, double overlap, Mat image, ostream & out,
			const string & images, vector<FFLDObjDetector::Detection> & detections)
{
	// Compute the scores
	vector<HOGPyramid::Matrix> scores;
	vector<Mixture::Indices> argmaxes;
	vector<vector<vector<Model::Positions> > > positions;
	
	if (!images.empty())
		mixture.convolve(pyramid, scores, argmaxes, &positions);
	else
		mixture.convolve(pyramid, scores, argmaxes);
	
	// Cache the size of the models
	vector<pair<int, int> > sizes(mixture.models().size());
	
	for (int i = 0; i < sizes.size(); ++i)
		sizes[i] = mixture.models()[i].rootSize();
	
	// For each scale
	for (int i = pyramid.interval(); i < scores.size(); ++i) {
		// Scale = 8 / 2^(1 - i / interval)
		const double scale = pow(2.0, static_cast<double>(i) / pyramid.interval() + 2.0);
		
		const int rows = scores[i].rows();
		const int cols = scores[i].cols();
		
		for (int y = 0; y < rows; ++y) {
			for (int x = 0; x < cols; ++x) {
				const float score = scores[i](y, x);
				
				if (score > threshold) {
					if (((y == 0) || (x == 0) || (score > scores[i](y - 1, x - 1))) &&
						((y == 0) || (score > scores[i](y - 1, x))) &&
						((y == 0) || (x == cols - 1) || (score > scores[i](y - 1, x + 1))) &&
						((x == 0) || (score > scores[i](y, x - 1))) &&
						((x == cols - 1) || (score > scores[i](y, x + 1))) &&
						((y == rows - 1) || (x == 0) || (score > scores[i](y + 1, x - 1))) &&
						((y == rows - 1) || (score > scores[i](y + 1, x))) &&
						((y == rows - 1) || (x == cols - 1) || (score > scores[i](y + 1, x + 1)))) {
						FFLD::Rectangle bndbox((x - pyramid.padx()) * scale + 0.5,
											   (y - pyramid.pady()) * scale + 0.5,
											   sizes[argmaxes[i](y, x)].second * scale + 0.5,
											   sizes[argmaxes[i](y, x)].first * scale + 0.5);
						
						// Truncate the object
						bndbox.setX(max(bndbox.x(), 0));
						bndbox.setY(max(bndbox.y(), 0));
						bndbox.setWidth(min(bndbox.width(), width - bndbox.x()));
						bndbox.setHeight(min(bndbox.height(), height - bndbox.y()));
						
						if (!bndbox.empty())
							detections.push_back(FFLDObjDetector::Detection(score, i, x, y, bndbox));
					}
				}
			}
		}
	}
	
	// Non maxima suppression
	sort(detections.begin(), detections.end());
	
	for (int i = 1; i < detections.size(); ++i)
		detections.resize(remove_if(detections.begin() + i, detections.end(),
									Intersector(detections[i - 1], overlap, true)) -
						  detections.begin());
	
}

bool FFLDObjDetector::detect(cv::Mat& image,const jake::jiObjectDetectionParams& params,vector<jake::jiObjectDetection> & detections)
{
	
	Object::Name name = Object::PERSON;
	string results;
	string images_bb;
	int nbNegativeScenes = -1;
	int padding = 12;
	int interval = 10;
	double thresh =0.5;
	double overlap = 0.5;
	
    int id = 0;
	ofstream out;
	
    char buffer[1000];
    
	const string filemod(params.model);
	
	const size_t lastDotmod = filemod.find_last_of('.');
	
    BackgroundSubtractorMOG mog;
    if (filemod.substr(lastDotmod) == ".txt") 
    {		
		if (image.empty()) {
			showUsage();
			cerr << "\nInvalid image "<< endl;
			return false;
		}
		
        //detection
        // Try to open the mixture
        ifstream in(filemod.c_str(), ios::binary);

        if (!in.is_open()) {
            showUsage();
            cerr << "\nInvalid model file " << filemod << endl;
            return false;
        }

        Mixture mixture;
        in >> mixture;

        if (mixture.empty()) {
            showUsage();
            cerr << "\nInvalid model" << params.model << endl;
            return false;
        }
        
//         load image
		JPEGImage img(image);
    
//         Compute the HOG features
		start();
        HOGPyramid pyramid(img, padding, padding, interval);

        if (pyramid.empty()) {
            showUsage();
            return false;
        }

        cout << "Computed HOG features in " << 	stop() << " ms" << endl;
    
        
        //Initialize the Patchwork class
		start();		
		if (!Patchwork::Init((pyramid.levels()[0].rows() - padding + 15) & ~15,
							 (pyramid.levels()[0].cols() - padding + 15) & ~15)) {
			cerr << "\nCould not initialize the Patchwork class" << endl;
			return false;
		}
		
		cout << "Initialized FFTW in " << 	stop() << " ms" << endl;
        
        start();
		mixture.cacheFilters();		
		cout << "Transformed the filters in " << 	stop() << " ms" << endl;
		
		// Compute the detections
		start();
		
        sprintf(buffer,"MatImage.jpg");

		vector<FFLDObjDetector::Detection>  tempDetections;
		
		detectObjects(mixture, img.width(), img.height(), pyramid, thresh, overlap, image, out,
			   buffer, tempDetections);		
		cout << "Computed the convolutions and distance transforms in " << stop() << " ms" << endl;  
		
		for (int j = 0; j < tempDetections.size(); ++j) 
		{
			jake::jiObjectDetection bndbox;
			bndbox.box.x=tempDetections[j].Rectangle::x();
			bndbox.box.y=tempDetections[j].Rectangle::y();
			bndbox.box.width=tempDetections[j].width();
			bndbox.box.height=tempDetections[j].height();
			bndbox.score=tempDetections[j].score;
			detections.push_back(bndbox);
		}
		return true;              
    }
    else
    {
		return false;
	}
}



