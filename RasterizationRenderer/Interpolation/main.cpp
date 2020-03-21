#include "Canvas.h"
#include "Point2.h"
#include "Vector3.h"
#include <cstdio>
#include <vector>
#include <array>
#include <list>
#include <chrono>

// 多边形的边
struct Edge
{
	double x;			// 边的起点x坐标
	double dx;			// 扫描线每往上移动一行x的增量
	double ymax;		// 边的终点y坐标
	Edge(double x1, double dx1, double ymax1)
		: x(x1), dx(dx1), ymax(ymax1) {}
};
// 边排序函数
bool sortEdge(Edge const& edge1, Edge const& edge2)
{
	if (edge1.x != edge2.x)
		return edge1.x < edge2.x;
	else					
		return edge1.dx < edge2.dx;	// 如果x坐标相等，那么根据x坐标的增量来比较，增量越大则在下一行x坐标也越大
}
// 构造2x2大小的图片
COLORREF picture[2][2] = {
	{RGB(255, 0, 0), RGB(255, 255, 255)},
	{RGB(0, 255, 0), RGB(0, 0, 255)}
};
// 获取图片颜色
COLORREF getPixel(int x, int y)
{
	return picture[x][y];
}
// 把纹理坐标转换为图片的xy坐标
void UV2XY(double u, double v, double& x, double& y, double w, double h)
{
	x = u * w;
	y = v * h;
	x = min(max(x, 0), w);	// 把x限制在0-w
	y = min(max(y, 0), h);	// 把y限制在0-h
}

void fillTriangle(Canvas& canvas, std::array<Point2, 3>& points, std::array<Color, 3>& colors)
{
	Vector3 ab(points[1].x - points[0].x, points[1].y - points[0].y, 0.0);
	Vector3 bc(points[2].x - points[1].x, points[2].y - points[1].y, 0.0);
	Vector3 ac(points[2].x - points[0].x, points[2].y - points[0].y, 0.0);
	Vector3 ca(points[0].x - points[2].x, points[0].y - points[2].y, 0.0);
	double area = cross(ab, ac).length();	// 三角形面积的两倍
	if (area == 0)
		return;

	// 创建NET(new edge table)表
	std::list<Edge>* NET = new std::list<Edge>[canvas.mScrHeight];

	int yMin = (int)points[0].y;			// 多边形y值的最小值
	int yMax = (int)points[0].y;			// 多边形y值的最大值
	// 初始化NET表
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

	// 创建AEL(active edge list)链表
	std::list<Edge> AEL;
	for (int y = yMin; y <= yMax; y++)
	{
		if (!NET[y].empty())								// 如果扫描行存在边，则加入AEL中
			AEL.splice(AEL.end(), NET[y]);
		AEL.sort(sortEdge);

		std::list<Edge>::iterator edgeStar, edgeEnd;		// 扫描行需要绘制的线段的端点的迭代器
		int countEdge = 0;									// 已存储的端点

		for (auto it = AEL.begin(); it != AEL.end(); )
		{
			// 扫描线已经到达某条边的y的最大值，则移除该边
			if ((int)it->ymax <= y)
			{
				it = AEL.erase(it);							// 这里记录移除元素的下一个元素的迭代器，不能用it++，因为it已经无效了。
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
					// 绘制线段
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
					// 这对交点的x值增加dx
					edgeStar->x += edgeStar->dx;
					edgeEnd->x += edgeEnd->dx;
				}
				it++;
			}
		}
	}
	delete[] NET;
}
// 传入纹理坐标
void fillTriangle(Canvas& canvas, std::array<Point2, 3>& points, double* coordinate)
{
	Vector3 ab(points[1].x - points[0].x, points[1].y - points[0].y, 0.0);
	Vector3 bc(points[2].x - points[1].x, points[2].y - points[1].y, 0.0);
	Vector3 ac(points[2].x - points[0].x, points[2].y - points[0].y, 0.0);
	Vector3 ca(points[0].x - points[2].x, points[0].y - points[2].y, 0.0);
	double area = cross(ab, ac).length();	// 三角形面积的两倍
	if (area == 0)
		return;

	// 创建NET(new edge table)表
	std::list<Edge>* NET = new std::list<Edge>[canvas.mScrHeight];

	int yMin = (int)points[0].y;			// 多边形y值的最小值
	int yMax = (int)points[0].y;			// 多边形y值的最大值
	// 初始化NET表
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

	// 创建AEL(active edge list)链表
	std::list<Edge> AEL;
	for (int y = yMin; y <= yMax; y++)
	{
		if (!NET[y].empty())								// 如果扫描行存在边，则加入AEL中
			AEL.splice(AEL.end(), NET[y]);
		AEL.sort(sortEdge);

		std::list<Edge>::iterator edgeStar, edgeEnd;		// 扫描行需要绘制的线段的端点的迭代器
		int countEdge = 0;									// 已存储的端点

		for (auto it = AEL.begin(); it != AEL.end(); )
		{
			// 扫描线已经到达某条边的y的最大值，则移除该边
			if ((int)it->ymax <= y)
			{
				it = AEL.erase(it);							// 这里记录移除元素的下一个元素的迭代器，不能用it++，因为it已经无效了。
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
					// 绘制线段
					for (int x = (int)edgeStar->x; x <= (int)edgeEnd->x; x++)
					{
						double weightA, weightB, weightC;
						Vector3 ap(x - points[0].x, y - points[0].y, 0.0);
						Vector3 bp(x - points[1].x, y - points[1].y, 0.0);
						Vector3 cp(x - points[2].x, y - points[2].y, 0.0);
						weightA = cross(bc, bp).length() / area;
						weightB = cross(ca, cp).length() / area;
						weightC = cross(ab, ap).length() / area;
						double u, v;				// 纹理坐标
						u = weightA * coordinate[0] + weightB * coordinate[2] + weightC * coordinate[4];	// 线性插值计算纹理坐标
						v = weightA * coordinate[1] + weightB * coordinate[3] + weightC * coordinate[5];	
						double px, py;				// 图片的坐标
						UV2XY(u, v, px, py, 2, 2);
						COLORREF c = getPixel((int)px, (int)py);
						canvas.setPixel(x, y, c);	// 绘制像素
					}
					// 这对交点的x值增加dx
					edgeStar->x += edgeStar->dx;
					edgeEnd->x += edgeEnd->dx;
				}
				it++;
			}
		}
	}
	delete[] NET;
}

int main()
{
	std::array<Point2, 3> triangle1 = { Point2(0, 0), Point2(400, 0), Point2(400, 300) };
	std::array<Color, 3> colors = {
		Color(255, 0, 0),
		Color(0, 255, 0),
		Color(0, 0, 255)
	};

	std::array<Point2, 3> triangle2 = { Point2(400, 300), Point2(600, 300), Point2(600, 500) };
	//纹理坐标
	double coordinate[] = {
		0,0,1,0,1,1
	};
	//创建绘图窗口
	Canvas canvas(800, 600);

	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

	//绘制三角形，传入颜色坐标
	//fillTriangle(canvas, triangle1, colors);		
	//传入纹理坐标
	fillTriangle(canvas, triangle2, coordinate);	

	std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
	std::chrono::microseconds duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
	//花费时间(秒)
	double useTime = double(duration.count()) * 
		std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;
	//往调试器打印信息用的缓冲区
	char msg[256];
	sprintf_s(msg, 256, "Debug版本，重心坐标插值绘制耗时:%lf 毫秒\n", useTime * 1000);
	//往调试器输出两帧绘制时间间隔
	OutputDebugStringA(msg);

	getchar();
	return 0;
}