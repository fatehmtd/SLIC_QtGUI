#include "SLIC.h"
#include <math.h>
#include <QColor>


SLIC::SLICResult SLIC::execute(float s, float m, float c, const QImage& input, int nloops /*= 10*/)
{
	const int width = input.width();
	const int height = input.height();

	const int nClustersX = ceil((float)width / s);
	const int nClustersY = ceil((float)height / s);

	auto clusters = new Cluster[nClustersX*nClustersY]; // contains the clusters
	auto pixelsLabels = new int[width*height]; // contains the labels of all pixels

	// initialize the clusters and labels
	for (int i = 0; i < nClustersY; i++)
	{
		const int clusterBeginY = i * s;
		const int clusterEndY = std::min<int>((i + 1)*s, height);
		for (int j = 0; j < nClustersX; j++)
		{
			const int clusterBeginX = j * s;
			const int clusterEndX = std::min<int>((j + 1)*s, width);

			const int clusterOffset = i * nClustersX + j;

			auto& cluster = clusters[clusterOffset];

			cluster.reset();

			// assign each pixel in the cluster
			for (int u = clusterBeginY; u < clusterEndY; u++)
			{
				for (int v = clusterBeginX; v < clusterEndX; v++)
				{
					const int pixelOffset = u * width + v;
					pixelsLabels[pixelOffset] = clusterOffset;
					const int pixelColor = input.pixel(v, u);

					cluster.addPixel(v, u, pixelColor);
				}
			}

			// compute mean values
			cluster.normalize();
		}
	}

	// loop assign
	bool loopNeeded = true;

	auto pixelsDistances = new float[width*height];
	auto pixelsTempLabels = new int[width*height];
	auto tempClusters = new Cluster[nClustersX*nClustersY];

	while (loopNeeded && nloops > 0) // perform nloops or break when loopNeed==false
	{
		loopNeeded = false;

		// initialize the temp clusters
		for (int i = 0; i < nClustersY*nClustersX; i++)
		{
			tempClusters[i].reset();
		}

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				// for each pixel, check surrounding clusters 
				// assign the new pixel label to the closest cluster

				const int pixelOffset = j + i * width;
				const int currentLabel = pixelsLabels[pixelOffset];

				const int currentClusterV = currentLabel % nClustersX;
				const int currentClusterU = currentLabel / nClustersX;

				const int pixelColor = input.pixel(j, i);
				float currentDist = distance(clusters[currentLabel], pixelColor, j, i, s, m, c);

				int newClusterIndex = currentLabel;

				// check neighboring clusters
				for (int u = std::max<int>(currentClusterU - 1, 0); u < std::min<int>(currentClusterU + 2, nClustersY); u++)
				{
					for (int v = std::max<int>(currentClusterV - 1, 0); v < std::min<int>(currentClusterV + 2, nClustersX); v++)
					{
						const int clusterIndex = v + u * nClustersX;

						if (clusterIndex == currentLabel) continue;

						const Cluster& cluster = clusters[clusterIndex];

						float spatialDistance = sqrtf(powf(cluster.x - j, 2) + powf(cluster.y - i, 2));

						if (spatialDistance > 2.0f * s) continue;

						float dist = distance(cluster, pixelColor, j, i, s, m, c);

						if (dist < currentDist)
						{
							newClusterIndex = clusterIndex;
							currentDist = dist;
						}
					}
				}

				if (newClusterIndex != currentLabel) loopNeeded = true;

				pixelsTempLabels[pixelOffset] = newClusterIndex;
				tempClusters[newClusterIndex].addPixel(j, i, pixelColor);
			}
		}

		for (int i = 0; i < nClustersY*nClustersX; i++)
		{
			tempClusters[i].normalize();
		}

		memcpy(clusters, tempClusters, sizeof(Cluster)*nClustersX*nClustersY); // copy clusters
		memcpy(pixelsLabels, pixelsTempLabels, sizeof(int)*width*height); // copy temp labels to final labels
		nloops--;
	}

	// free temp memory
	delete[] tempClusters;
	delete[] pixelsTempLabels;
	delete[] pixelsDistances;

	// write results
	QImage output(input.size(), QImage::Format::Format_RGB32);
	QImage edges(input.size(), QImage::Format::Format_RGB32);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			const auto& cluster = clusters[pixelsLabels[i*width + j]];
			output.setPixel(j, i, qRgb(cluster.r, cluster.g, cluster.b));
			edges.setPixel(j, i, input.pixel(j, i));
		}
	}

	// performs pseudo edge detection using if statements
	for (int i = 1; i < height - 1; i++)
	{
		for (int j = 1; j < width - 1; j++)
		{
			const int neighborsLabels[] = { ((i - 1)*width + (j - 1)), ((i - 1)*width + (j)), ((i - 1)*width + (j + 1)),
				((i)*width + (j - 1)), ((i)*width + (j + 1)),
			((i + 1)*width + (j - 1)), ((i + 1)*width + (j)), ((i + 1)*width + (j + 1)) };

			int label = pixelsLabels[i*width + j];

			if (clusters[label].numPixels < s*s*0.5f) // skip small clusters
				continue;

			for (int n = 0; n < 8; n++)
			{
				if (label != pixelsLabels[neighborsLabels[n]])
				{
					const auto& cluster = clusters[pixelsLabels[i*width + j]];
					edges.setPixel(j, i, qRgb(cluster.edgeR, cluster.edgeG, cluster.edgeB));
					break;
				}
			}

		}
	}

	delete[] pixelsLabels;
	delete[] clusters;

	return { output, edges };
}

float SLIC::distance(const Cluster& c, int r, int g, int b, int x, int y, float s, float m, float cc)
{
	const float dXY = (c.x - (float)x)*(c.x - (float)x) + (c.y - (float)y)*(c.y - (float)y);
	const float dRGB = (c.r - (float)r)*(c.r - (float)r) + (c.g - (float)g)*(c.g - (float)g) + (c.b - (float)b)*(c.b - (float)b);

	return sqrtf(dRGB * cc / m + dXY *m / s);
}

float SLIC::distance(const Cluster& cluster, int rgb, int x, int y, float s, float m, float c)
{
	return distance(cluster, qRed(rgb), qGreen(rgb), qBlue(rgb), x, y, s, m, c);
}

void SLIC::Cluster::reset()
{
	x = y = 0.0f;
	r = g = b = 0.0f;
	numPixels = 0;
}

void SLIC::Cluster::addPixel(int x, int y, int r, int g, int b)
{
	this->x += x;
	this->y += y;
	this->r += r;
	this->g += g;
	this->b += b;
	numPixels++;
}

void SLIC::Cluster::addPixel(int x, int y, int rgb)
{
	this->x += x;
	this->y += y;
	this->r += qRed(rgb);
	this->g += qGreen(rgb);
	this->b += qBlue(rgb);
	numPixels++;
}

void SLIC::Cluster::normalize()
{
	this->x /= (float)numPixels;
	this->y /= (float)numPixels;
	this->r /= (float)numPixels;
	this->g /= (float)numPixels;
	this->b /= (float)numPixels;

	edgeR = rand() % 256;
	edgeG = rand() % 256;
	edgeB = rand() % 256;
}
