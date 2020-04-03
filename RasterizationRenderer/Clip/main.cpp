#include "Canvas.h"
#include "Point2.h"
#include "Point3.h"
#include "Point4.h"
#include "Vector3.h"
#include <cstdio>
#include <vector>
#include <array>
#include <list>
#include <chrono>

// ����εı�
struct Edge
{
	double x;			// �ߵ����x����
	double dx;			// ɨ����ÿ�����ƶ�һ��x������
	double ymax;		// �ߵ��յ�y����
	Edge(double x1, double dx1, double ymax1)
		: x(x1), dx(dx1), ymax(ymax1) {}
};
// ��������
bool sortEdge(Edge const& edge1, Edge const& edge2)
{
	if (edge1.x != edge2.x)
		return edge1.x < edge2.x;
	else					
		return edge1.dx < edge2.dx;	// ���x������ȣ���ô����x������������Ƚϣ�����Խ��������һ��x����ҲԽ��
}
// ����2x2��С��ͼƬ
COLORREF picture[2][2] = {
	{RGB(255, 0, 0), RGB(255, 255, 255)},
	{RGB(0, 255, 0), RGB(0, 0, 255)}
};
// ��ȡͼƬ��ɫ
COLORREF getPixel(int x, int y)
{
	return picture[x][y];
}
// ����������ת��ΪͼƬ��xy����
void UV2XY(double u, double v, double& x, double& y, double w, double h)
{
	x = u * w;
	y = v * h;
	x = min(max(x, 0), w);	// ��x������0-w
	y = min(max(y, 0), h);	// ��y������0-h
}

void fillTriangle(Canvas& canvas, std::array<Point2, 3>& triangle, std::array<Color, 3>& colors)
{
	std::array<Point2, 3> points;
	for (int i = 0; i < 3; i++)
	{
		points[i].x = (triangle[i].x + 1) * 0.5 * ((double)canvas.mScrWidth - 1);
		points[i].y = (triangle[i].y + 1) * 0.5 * ((double)canvas.mScrHeight - 1);
	}

	Vector3 ab(points[1].x - points[0].x, points[1].y - points[0].y, 0.0);
	Vector3 bc(points[2].x - points[1].x, points[2].y - points[1].y, 0.0);
	Vector3 ac(points[2].x - points[0].x, points[2].y - points[0].y, 0.0);
	Vector3 ca(points[0].x - points[2].x, points[0].y - points[2].y, 0.0);
	double area = cross(ab, ac).length();	// ���������������
	if (area == 0)
		return;

	// ����NET(new edge table)��
	std::list<Edge>* NET = new std::list<Edge>[canvas.mScrHeight];

	int yMin = (int)points[0].y;			// �����yֵ����Сֵ
	int yMax = (int)points[0].y;			// �����yֵ�����ֵ
	// ��ʼ��NET��
	int count = points.size();
	for (int i = 0; i < count; i++)
	{
		if (points[i].y > points[(i + 1) % count].y)
		{
			double x = points[(i + 1) % count].x;
			double dx = (points[(i + 1) % count].x - points[i].x)
				/ (points[(i + 1) % count].y - points[i].y);
			double ymax = points[i].y;
			NET[(int)points[(i + 1) % count].y].push_back(Edge(x, dx, ymax));
		}
		else if (points[i].y < points[(i + 1) % count].y)
		{
			double x = points[i].x;
			double dx = (points[(i + 1) % count].x - points[i].x)
				/ (points[(i + 1) % count].y - points[i].y);
			double ymax = points[(i + 1) % count].y;
			NET[(int)points[i].y].push_back(Edge(x, dx, ymax));
		}
		yMin = (int)min(points[i].y, yMin);
		yMax = (int)max(points[i].y, yMax);
	}

	// ����AEL(active edge list)����
	std::list<Edge> AEL;
	for (int y = yMin; y <= yMax; y++)
	{
		if (!NET[y].empty())								// ���ɨ���д��ڱߣ������AEL��
			AEL.splice(AEL.end(), NET[y]);
		AEL.sort(sortEdge);

		std::list<Edge>::iterator edgeStar, edgeEnd;		// ɨ������Ҫ���Ƶ��߶εĶ˵�ĵ�����
		int countEdge = 0;									// �Ѵ洢�Ķ˵�

		for (auto it = AEL.begin(); it != AEL.end(); )
		{
			// ɨ�����Ѿ�����ĳ���ߵ�y�����ֵ�����Ƴ��ñ�
			if ((int)it->ymax <= y)
			{
				it = AEL.erase(it);							// �����¼�Ƴ�Ԫ�ص���һ��Ԫ�صĵ�������������it++����Ϊit�Ѿ���Ч�ˡ�
			}
			else
			{
				if (countEdge == 0)
				{
					edgeStar = it;
					countEdge++;
				}
				else if (countEdge == 1)
				{
					countEdge = 0;
					edgeEnd = it;
					// �����߶�
					for (int x = (int)edgeStar->x; x <= (int)edgeEnd->x; x++)
					{
						double weightA, weightB, weightC;
						Vector3 ap(x - points[0].x, y - points[0].y, 0.0);
						Vector3 bp(x - points[1].x, y - points[1].y, 0.0);
						Vector3 cp(x - points[2].x, y - points[2].y, 0.0);
						weightA = cross(bc, bp).length() / area;
						weightB = cross(ca, cp).length() / area;
						weightC = cross(ab, ap).length() / area;
						BYTE r, g, b;
						Color target = weightA * colors[0] + weightB * colors[1] + weightC * colors[2];
						r = (BYTE)(target.r);
						g = (BYTE)(target.g);
						b = (BYTE)(target.b);
						canvas.setPixel(x, y, RGB(r, g, b));
					}
					// ��Խ����xֵ����dx
					edgeStar->x += edgeStar->dx;
					edgeEnd->x += edgeEnd->dx;
				}
				it++;
			}
		}
	}
	delete[] NET;
}

// ͶӰ����,�ֱ𴫵�һ�����nearƽ�棬�������ͶӰ�㲢�����������һ�����������
void projection(Point3& point, double l, double r, double b, double t, double n, double f, Point4& result)
{
	result.x = (2 * n * point.x) / (r - l) - point.z * (r + l) / (r - l);
	result.y = (2 * n * point.y) / (t - b) - point.z * (t + b) / (t - b);
	result.z = (f + n) / (f - n) * point.z + 2 * f * n / (n - f);
	result.w = point.z;
}

//��ֱ�߽��вü�,����-1��ʾƽ���ܾ�������0��ʾƽ�����ܣ�����1��ʾ��A����вü�������2��ʾ��B����вü��������������result��t��ʾ�ü��������tֵ
//���в���flag��0��5�ֱ��ʾ�߽�Ϊleft��right��bottom��top��near��far
int clipLine(int flag, Point4& A, Point4& B, Point4& result, double& t)
{
	double aClipCondition = 0;
	double bClipCondition = 0;
	switch (flag)
	{
	case 0:
		aClipCondition = A.x + A.w;
		bClipCondition = B.x + B.w;
		break;
	case 1:
		aClipCondition = A.w - A.x;
		bClipCondition = B.w - B.x;
		break;
	case 2:
		aClipCondition = A.y + A.w;
		bClipCondition = B.y + B.w;
		break;
	case 3:
		aClipCondition = A.w - A.y;
		bClipCondition = B.w - B.y;
		break;
	case 4:
		aClipCondition = A.z + A.w;
		bClipCondition = B.z + B.w;
		break;
	case 5:
		aClipCondition = A.w - A.z;
		bClipCondition = B.z - B.z;
		break;
	default:
		return 0;
	}
	if (aClipCondition < 0 && bClipCondition < 0)
		return -1;
	if (aClipCondition >= 0 && bClipCondition >= 0)
		return 0;

	t = aClipCondition / (aClipCondition - bClipCondition);
	result.x = A.x + (B.x - A.x) * t;
	result.y = A.y + (B.y - A.y) * t;
	result.z = A.z + (B.z - A.z) * t;
	result.w = A.w + (B.w - A.w) * t;
	if (aClipCondition < 0)
		return 1;
	else
		return 2;
}

void clipTriangleAndDraw(std::array<Point4, 3>& triangle, std::vector<double>& attribute, Canvas& canvas)
{
	std::vector<Point4> points(triangle.begin(), triangle.end());	// �ü��󶥵�����
	std::vector<double> attributes(attribute);						// �ü��󶥵�����
	
	// ����׵��������ν��вü����ü�������Suthland-Hodgman�㷨����
	for (int clipflag = 0; clipflag < 6; clipflag++)
	{
		std::vector<Point4> temp;		// ���ڴ��汾�βü�֮��Ķ���
		std::vector<double> tempAttri;	// ���ڴ��汾�βü�֮��Ķ�������
		int size = points.size();
		for (int i = 0; i < size; i++)
		{
			Point4 result;				// �߽��������αߵĽ���
			double t;					// �������Բ�ֵ��ϵ��
			int indexA = i, indexB = (i + 1) % size;
			int ret = clipLine(clipflag, points[indexA], points[indexB], result, t);
			// ƽ������
			if (ret == 0)	
			{
				temp.push_back(points[indexB]);
				tempAttri.push_back(attributes[indexB * 3]);
				tempAttri.push_back(attributes[indexB * 3 + 1]);
				tempAttri.push_back(attributes[indexB * 3 + 2]);
			}
			// �ü�A�㣬��������B��
			else if (ret == 1)
			{
				temp.push_back(result);
				tempAttri.push_back(attributes[indexA * 3] + (attributes[indexB * 3] - attributes[indexA * 3]) * t);
				tempAttri.push_back(attributes[indexA * 3 + 1] + (attributes[indexB * 3 + 1] - attributes[indexA * 3 + 1]) * t);
				tempAttri.push_back(attributes[indexA * 3 + 2] + (attributes[indexB * 3 + 2] - attributes[indexA * 3 + 2]) * t);
				temp.push_back(points[(i + 1) % size]);
				tempAttri.push_back(attributes[indexB * 3]);
				tempAttri.push_back(attributes[indexB * 3 + 1]);
				tempAttri.push_back(attributes[indexB * 3 + 2]);
			}
			// �ü�B�㣬�������
			else if (ret == 2)
			{
				temp.push_back(result);
				tempAttri.push_back(attributes[indexA * 3] + (attributes[indexB * 3] - attributes[indexA * 3]) * t);
				tempAttri.push_back(attributes[indexA * 3 + 1] + (attributes[indexB * 3 + 1] - attributes[indexA * 3 + 1]) * t);
				tempAttri.push_back(attributes[indexA * 3 + 2] + (attributes[indexB * 3 + 2] - attributes[indexA * 3 + 2]) * t);
			}
		}
		points.clear();
		points.assign(temp.begin(), temp.end());
		attributes.clear();
		attributes.assign(tempAttri.begin(), tempAttri.end());
	}
	// ���ü����ͼ�ηֽ�ɶ�������ν��л��ƣ��ǵ�Ҫ�Ƚ���͸�ӳ���������任��CVV�ռ�
	std::array<Point2, 3> out;
	std::array<Color, 3> colors;
	for (int i = 1; i < points.size() - 1; i++)
	{
		out[0].x = points[0].x / points[0].w;
		out[0].y = points[0].y / points[0].w;
		out[1].x = points[i].x / points[i].w;
		out[1].y = points[i].y / points[i].w;
		out[2].x = points[i+1].x / points[i+1].w;
		out[2].y = points[i+1].y / points[i+1].w;

		colors[0].r = attributes[0];
		colors[0].g = attributes[1];
		colors[0].b = attributes[2];
		colors[1].r = attributes[i * 3];
		colors[1].g = attributes[i * 3 + 1];
		colors[1].b = attributes[i * 3 + 2];
		colors[2].r = attributes[(i + 1) * 3];
		colors[2].g = attributes[(i + 1) * 3 + 1];
		colors[2].b = attributes[(i + 1) * 3 + 2];

		fillTriangle(canvas, out, colors);
	}
}

int main()
{
	//������ͼ����
	Canvas canvas(640, 480);

	std::array<Point3, 3> triangle = { Point3(-640, -480, 50), Point3(0, 480, 2), Point3(640, -480, 50) };
	std::vector<double> colors = { 255, 0, 0, 0, 255, 0, 0, 0, 255 };
	// �ü��ռ�����
	std::array<Point4, 3> result;
	for (int i = 0; i < 3; i++)
		projection(triangle[i], -320, 320, -240, 240, 25, 50, result[i]);

	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
	
	// �ü������β�����
	clipTriangleAndDraw(result, colors, canvas);

	std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
	std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	//����ʱ��(��)
	double useTime = double(duration.count()) * 
		std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
	//����������ӡ��Ϣ�õĻ�����
	char msg[256];
	sprintf_s(msg, 256, "Debug�汾�����������ֵ���ƺ�ʱ:%lf ����\n", useTime * 1000);
	//�������������֡����ʱ����
	OutputDebugStringA(msg);

	getchar();
	return 0;
}