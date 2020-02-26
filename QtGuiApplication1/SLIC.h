#pragma once
#include <QImage>

class SLIC
{
public:
	struct Cluster
	{
		float x = 0.0f, y = 0.0f; // center
		float r = 0.0f, g = 0.0f, b = 0.0f; // mean color
		float edgeR = 0.0f, edgeG = 0.0f, edgeB = 0.0f; // mean color
		int numPixels = 0;
		int label = -1;

		void reset();
		void addPixel(int x, int y, int r, int g, int b);
		void addPixel(int x, int y, int rgb);
		void normalize();
	};

	struct SLICResult
	{
		QImage _oversegmentedImage;
		QImage _edgesImage;
	};

	SLICResult execute(float s, float m, float c, const QImage& input, int nloops = 10); // performs the clustering
protected:
	float distance(const Cluster& cluster, int r, int g, int b, int x, int y, float s, float m, float c);
	float distance(const Cluster& cluster, int rgb, int x, int y, float s, float m, float c);
};

