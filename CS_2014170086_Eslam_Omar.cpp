#include <tchar.h>
#include <Windows.h>
#include <omp.h>
#include <cstring>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <iostream>
#include<random>
#include<math.h>
#include<vector>
#define data_path "C:/Users/eslamomar/Desktop/DC/IrisDataset.txt" //add path here
using namespace std;
int k;

class point{
public:
	float x, y, z, h;
	point(){
		this->x = 0.0;
		this->y = 0.0;
		this->z = 0.0;
		this->h = 0.0;
	}

};

vector<point> get_data()
{
	vector<point> points;
	ifstream ifile;
	ifile.open(data_path);
	if (!ifile) {
		printf("Unable to open file\n");
		exit(0);
	}
	int DataSize;
	string line;
	getline(ifile, line);
	istringstream buff(line);
	buff >> DataSize >> k;
	char tmp;
	while (getline(ifile, line))
	{
		point p;
		istringstream buff(line);
		buff >> p.x >> tmp >> p.y >> tmp >> p.z >> tmp >> p.h;
		points.push_back(p);
	}

	return points;
}

//sequential part

vector<vector<point>> AssignPointsToClusters_Seq(vector<point> centroids, vector<point>data)
{
	vector<vector<point>> new_clusters(k);
	float sum = 0;
	for (int i = 0; i < k; i++)
	{
		new_clusters[i].push_back(centroids[i]);
	}
	for (int i = 0; i < data.size(); i++)
	{
		float min = 1e12;
		int temp = 0;
		for (int j = 0; j < k; j++)
		{
			sum = 0;
			sum += (pow((data[i].x - centroids[j].x), 2) + pow((data[i].y - centroids[j].y), 2)
				+ pow((data[i].z - centroids[j].z), 2) + pow((data[i].h - centroids[j].h), 2));
			sum = sqrt(sum);
			if (sum < min){
				min = sum;
				temp = j;
			}
		}
		new_clusters[temp].push_back(data[i]);
	}
	return new_clusters;

}

vector<point>update_centroids_Seq(vector<vector<point>>clusters){
	vector<point> new_centroids;
	for (int i = 0; i < k; i++) //loop over clusters
	{
		float sum, x1, y1, z1, h1;
		sum = x1 = y1 = z1 = h1 = 0;
		for (int j = 0; j < clusters[i].size(); j++)//loop over points in each cluster
		{
			x1 += clusters[i][j].x;
			y1 += clusters[i][j].y;
			z1 += clusters[i][j].z;
			h1 += clusters[i][j].h;
		}
		x1 /= clusters[i].size();
		y1 /= clusters[i].size();
		z1 /= clusters[i].size();
		h1 /= clusters[i].size();
		point p;
		p.x = x1; p.y = y1; p.z = z1; p.h = h1;
		new_centroids.push_back(p);
	}
	return new_centroids;
}

vector<point>sequintial(){

	vector<point> data, cent, new_cent, new_cent_temp;
	vector<vector<point>> assigned;
	data = get_data();
	int x = 0;
	for (int i = 0; i < k; i++)
		cent.push_back(data[i]);

	assigned = AssignPointsToClusters_Seq(cent, data);//initial state
	new_cent_temp = cent;
	int flag = 0;

	while (true)
	{
		new_cent.clear();
		new_cent = update_centroids_Seq(assigned);
		flag = 0;
		for (int i = 0; i < k; i++)
		{
			if (abs(new_cent_temp[i].x - new_cent[i].x) <= .0001 && abs(new_cent_temp[i].y - new_cent[i].y) <= .0001 &&
				abs(new_cent_temp[i].z - new_cent[i].z) <= .0001 && abs(new_cent_temp[i].h - new_cent[i].h) <= .0001)
			{
				flag++;
			}
		}
		if (flag == k)
			break;
		new_cent_temp = new_cent;
		assigned.clear();
		assigned = AssignPointsToClusters_Seq(new_cent, data);
	}
	return new_cent;
}


//parallel part

vector<vector<point>> AssignPointsToClusters_par(vector<point> centroids, vector<point>data)
{
	vector<vector<point>> new_clusters(k);
	float sum = 0;
	int i, j;
	for (int f = 0; f < k; f++)
	{
		new_clusters[f].push_back(centroids[f]);
	}
#pragma opm parallel 
	{
		omp_set_num_threads(k);
#pragma omp parallel for private( i,j,sum) 
		
#pragma  collapse(2) shared(new_clusters)
		for (i = 0; i < data.size(); i++)
		{
			float min = 1e12;
			int temp = 0;
			for (j = 0; j < k; j++)
			{
				sum = 0;
				sum += (pow((data[i].x - centroids[j].x), 2) + pow((data[i].y - centroids[j].y), 2)
					+ pow((data[i].z - centroids[j].z), 2) + pow((data[i].h - centroids[j].h), 2));
				sum = sqrt(sum);
				if (sum < min){
					min = sum;
					temp = j;
				}
			}
			//printf("thread id :  %d\n", omp_get_thread_num());
#pragma omp critical
			new_clusters[temp].push_back(data[i]);
		}
	}
		return new_clusters;
}

vector<point>update_centroids_par(vector<vector<point>>clusters){
	vector<point> new_centroids;
	int i, j;
	float sum, x1, y1, z1, h1;
#pragma opm parallel 
	{
#pragma omp parallel for private(i,j,x1,y1,z1,h1) 
#pragma collapse(2)shared(new_centroids)
	for ( i = 0; i < k; i++) //loop over clusters
	{
		sum = x1 = y1 = z1 = h1 = 0;
		for (j = 0; j < clusters[i].size(); j++)//loop over points in each cluster
		{
			x1 += clusters[i][j].x;
			y1 += clusters[i][j].y;
			z1 += clusters[i][j].z;
			h1 += clusters[i][j].h;
		}
		x1 /= clusters[i].size();
		y1 /= clusters[i].size();
		z1 /= clusters[i].size();
		h1 /= clusters[i].size();
		point p;
		p.x = x1; p.y = y1; p.z = z1; p.h = h1;
		//printf("thread id :  %d\n", omp_get_thread_num());

#pragma omp critical
		new_centroids.push_back(p);
	}
	}
	return new_centroids;
}

vector<point>parallel(){

	vector<point> data, cent, new_cent, new_cent_temp;
	vector<vector<point>> assigned;
	data = get_data();
	int x = 0;
	for (int i = 0; i < k; i++)
		cent.push_back(data[i]);

	assigned = AssignPointsToClusters_par(cent, data);//initial state
	new_cent_temp = cent;
	int flag = 0;

	while (true)
	{
		new_cent.clear();
		new_cent = update_centroids_par(assigned);
		flag = 0;
		for (int i = 0; i < k; i++)
		{
			if (abs(new_cent_temp[i].x - new_cent[i].x) <= .0001 && abs(new_cent_temp[i].y - new_cent[i].y) <= .0001 &&
				abs(new_cent_temp[i].z - new_cent[i].z) <= .0001 && abs(new_cent_temp[i].h - new_cent[i].h) <= .0001)
			{
				flag++;
			}
		}
		if (flag == k)
			break;
		new_cent_temp = new_cent;
		assigned.clear();
		assigned = AssignPointsToClusters_par(new_cent, data);
	}
	return new_cent;
}


int main(int argc, char *argv[]){
	vector<point>new_cent;

	/*************************************************************************************
	// please note that the output of the two methods are the same but in different order
	*************************************************************************************/

	new_cent = sequintial();	//method 1
	//new_cent = parallel();	//method 2

	ofstream outfile("IrisDataset_clusters_centers.txt");
	for (int i = 0; i < k; i++)
	{
		outfile << i + 1 << "  " << new_cent[i].x << ' ' << new_cent[i].y << ' ' << ' ' << new_cent[i].z << ' ' << new_cent[i].h << endl;
	}
	outfile.close();
	return 0;
}