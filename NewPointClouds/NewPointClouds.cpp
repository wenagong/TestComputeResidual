// NewPointClouds.cpp: 定义控制台应用程序的入口点。
//
// MatchExperiment.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include"iostream"
#include"opencv.hpp"
#include"highgui.hpp"
#include"xfeatures2d.hpp"

#define La1 -0.0765583
#define La2 1.12963
#define La3 -0.000478566
#define La4 -0.00934149   //左相机的四个畸变参数
#define Ra1 -0.0564752
#define Ra2 0.598481
#define Ra3 0.00219212
#define Ra4 -0.000801994  //右相机的四个畸变参数
#define Lcx 563.737           
#define Lcy 449.899    //左相机的光心位置
#define Rcx 638.157
#define Rcy 478.782   //右相机的光心位置
#define fx1 3317.41
#define fy1 3318.89   //左相机的焦距
#define fx2 3346.03
#define fy2 3347.61   //右相机的焦距

using namespace cv;
using namespace std;

int main()
{
	Mat imageL0 = imread("Pattenimage_L.bmp");
	Mat imageR0 = imread("Pattenimage_R.bmp");
	//Mat imageL1, imageR1;
	//GaussianBlur(imageL0, imageL1, Size(3, 3), 0.5);
	//GaussianBlur(imageR0, imageR1, Size(3, 3), 0.5);
	ofstream file,debug;
	file.open("PointClouds.txt");
	debug.open("debug.txt");

	Mat dist1 = (Mat_<float>(5, 1) << La1, La2 ,La3 , La4 , 0); //左相机畸变参数(k1,k2,p1,p2,k3)
	Mat dist2 = (Mat_<float>(5, 1) << Ra1, Ra2, Ra3, Ra4, 0); //右相机畸变参数

	double m1[3][3] = { { fx1,0,Lcx },{ 0,fy1,Lcy },{ 0,0,1 } };
	Mat m1_matrix(Size(3, 3), CV_64F, m1);  //左相机的内参矩阵
								
	//Mat r1_matrix = Mat::eye(3, 3, CV_32FC1);   // 左相机旋转矩阵(对角矩阵)
	//Mat t1_matrix = (Mat_<float>(3, 1) << 0, 0, 0);   //左相机平移矩阵
	//Mat p1_matrix(Size(4, 3), CV_32F);
	//Mat n1_matrix(Size(4, 3), CV_32F);
	//hconcat(r1_matrix, t1_matrix, n1_matrix);
	// vconcat（B, C，A）;  等同于A=[B ; C] 按列合并   
	// hconcat（B, C，A）;  等同于A=[B  C] 按行合并
    //p1_matrix = m1_matrix * n1_matrix;//左相机投影矩阵

	double m2[3][3] = { { fx2,0,Rcx },{ 0,fy2,Rcy },{ 0,0,1 } };
	Mat m2_matrix(Size(3, 3), CV_64F, m2);  //右相机的内参矩阵
	float r2[3][1] = { { 0.00778951 },{ -0.121633 },{ 0.0150494 } }; 
	Mat r2_src2(Size(1, 3), CV_32F, r2);
	Mat r2_matrix(Size(3, 3), CV_64F); //右相机旋转向量
	Rodrigues(r2_src2, r2_matrix);   //旋转向量转化为旋转矩阵
	Mat t2_matrix = (Mat_<double>(3, 1) << 105.017, 2.22392, 19.288);   //右相机平移向量
	//Mat p2_matrix(Size(4, 3), CV_32F);
	//Mat n2_matrix(Size(4, 3), CV_32F);
	//hconcat(r2_matrix, t2_matrix, n2_matrix);//右相机外参矩阵3X4
	//p2_matrix = m2_matrix * n2_matrix;//右相机投影矩阵
	 
	float M1[3][3] = { { fx1,0,Lcx },{ 0,fy1,Lcy },{ 0,0,1 } };
	Mat M1_matrix(Size(3, 3), CV_32F, M1);  //左相机的内参矩阵

	float M2[3][3] = { { fx2,0,Rcx },{ 0,fy2,Rcy },{ 0,0,1 } };
	Mat M2_matrix(Size(3, 3), CV_32F, M2);  //右相机的内参矩阵

	Mat R1 = Mat::eye(3, 3, CV_32F);     //左相机图像畸变矫正
	Mat map1x = Mat(imageL0.size(), CV_32FC1);
	Mat map1y = Mat(imageL0.size(), CV_32FC1);
	initUndistortRectifyMap(M1_matrix, dist1, R1, M1_matrix, imageL0.size(), CV_32FC1, map1x, map1y);
	Mat picture1 = imageL0.clone();
	remap(imageL0, picture1, map1x, map1y, INTER_LINEAR);

	Mat R2 = Mat::eye(3, 3, CV_32F);     //右相机图像畸变矫正
	Mat map2x = Mat(imageR0.size(), CV_32FC1);
	Mat map2y = Mat(imageR0.size(), CV_32FC1);
	initUndistortRectifyMap(M2_matrix, dist2, R2, M2_matrix, imageR0.size(), CV_32FC1, map2x, map2y);
	Mat picture2 = imageR0.clone();
	remap(imageR0, picture2, map2x, map2y, INTER_LINEAR);

	//imshow("矫正后左图", picture1);
	//imshow("矫正后右图", picture2);

	//SIFT找出特征点
	Ptr<Feature2D>f2d = xfeatures2d::SIFT::create(0, 3, 0.04, 10); 
	vector<KeyPoint>keyPoint1, keyPoint2;
	Mat descriptors1, descriptors2;

	f2d->detectAndCompute(picture1, noArray(), keyPoint1, descriptors1);
	f2d->detectAndCompute(picture2, noArray(), keyPoint2, descriptors2);

	drawKeypoints(picture1, keyPoint1, picture1, Scalar::all(-1), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
	drawKeypoints(picture2, keyPoint2, picture2, Scalar::all(-1), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

	vector<DMatch>matches1;
	BFMatcher matcher1(NORM_L2);
	matcher1.match(descriptors1, descriptors2, matches1);

	Mat img_before;
	drawMatches(picture1, keyPoint1, picture2, keyPoint2, matches1, img_before);
	imshow("误匹配消除前", img_before);


	//用knn算法获取与该特征点最匹配的两个特征点
	vector<DMatch>matches;

	vector<vector<DMatch>>knn_matches;
	BFMatcher matcher(NORM_L2);
	
	//设置K = 2 ,即对每个匹配返回两个最近邻描述符,仅当第一个匹配与第二个匹配之间的距离足够小时，才认为这是一个匹配
	matcher.knnMatch(descriptors1, descriptors2, knn_matches, 2);
	
	//获取满足Ration Test的最小匹配距离
	float min_dist = FLT_MAX;
	for (int r = 0; r < knn_matches.size(); ++r) {
		//Ratio Test
		if (knn_matches[r][0].distance > 0.6*knn_matches[r][1].distance)
			continue;

		float dist = knn_matches[r][0].distance;
		if (dist < min_dist)
			min_dist = dist;
	}

	for (size_t r = 0; r < knn_matches.size(); r++) {
		//排除不满足Ratio Test的点和匹配距离过大的点
		if (
			knn_matches[r][0].distance > 0.6*knn_matches[r][1].distance ||
			knn_matches[r][0].distance > 5 * max(min_dist, 10.0f)
			)
			continue;
		matches.push_back(knn_matches[r][0]);
	}
	Mat img_mid;
	drawMatches(picture1, keyPoint1, picture2, keyPoint2, matches, img_mid);
	imshow("误匹配排除后", img_mid);

	vector<Point2f>points1, points2;
	for (vector<DMatch>::const_iterator it = matches.begin(); it != matches.end(); ++it) {     //容纳两个关键点匹配的结构
		points1.push_back(keyPoint1[it->queryIdx].pt);  //取得左侧关键点的位置
		points2.push_back(keyPoint2[it->trainIdx].pt);  //取得右侧关键点的位置
	}

	//RANSAC计算单应变换矩阵排除错误匹配点对
	double distance = 1;   //点到对极线的最大距离
	double confidence = 0.98;    //矩阵正确的可信度
	vector<uchar>inliers(points1.size(), 0);
	Mat fundaental = findFundamentalMat(points1, points2, inliers, CV_FM_RANSAC, distance, confidence);
	//RANSAC方法，到对极线的距离，可信度,匹配的点，匹配状态（局内项或局外项）

	//提取存留的局内项
	vector<uchar>::const_iterator itIn = inliers.begin();
	vector<DMatch>::const_iterator itM = matches.begin();

	//遍历全部匹配项
	vector<DMatch>Matches;
	for (; itIn != inliers.end(); ++itIn, ++itM) {
		if (*itIn) {  //有效的匹配项
			Matches.push_back(*itM);
		}
	}

	Mat imageOutput;
	drawMatches(picture1, keyPoint1, picture2, keyPoint2, Matches, imageOutput, Scalar::all(-1));

	namedWindow("picture of matching");
	imshow("picture of matching", imageOutput);

	////立体对极线校正，透视投影p1,p2
	////Mat mm = (Mat_<float>(1, 3) << 0, 0, 1);   
	////Mat mm1,mm2;
	////vconcat(m1_matrix,mm,mm1);
	////vconcat(m2_matrix, mm, mm2);
	////Mat Rl, Rr, Pl, Pr, Q,mapx,mapy,image1,image2;
	////stereoRectify(mm1, dist1, mm2, dist2, imageL0.size(), r2_matrix, t2_matrix, Rl, Rr, Pl, Pr, Q);
	////
	////initUndistortRectifyMap(Pl(cv::Rect(0, 0, 3, 3)), dist1, Rl, Pl(cv::Rect(0, 0, 3, 3)), image1.size(), CV_32FC1, mapx, mapy);
	////remap(picture1, image1, mapx, mapy, CV_INTER_LINEAR);
	////imwrite("校正左图.png", image1);

	////initUndistortRectifyMap(Pr(cv::Rect(0, 0, 3, 3)), dist2, Rr, Pr(cv::Rect(0, 0, 3, 3)), image2.size(), CV_32FC1, mapx, mapy);
	////remap(picture2, image2, mapx, mapy, CV_INTER_LINEAR);
	////imwrite("校正右图.png", image2);

	////imshow("矫正后左图", image1);
	////imshow("矫正后右图", image2);


	//Mat structure(points1.size(), 4, CV_32F);
	//直接调用triangulatePoints函数求深度信息
	//triangulatePoints(p1_matrix, p2_matrix, points1, points2, structure); //生成点的4XN矩阵
	////file << points1.size() << endl;

	//Mat t;
	//t = structure.t();   //转置

	////file << endl << structure << endl<<endl;

	//Mat pointclouds(Size(3, t.rows), CV_32F);

	//for (int i = 0; i < pointclouds.rows; i++) {
	//	for (int j = 0; j < pointclouds.cols; j++) {
	//		pointclouds.ptr<float>(i)[j] = t.ptr<float>(i)[j] / t.ptr<float>(i)[3];
	//		file<< pointclouds.ptr<float>(i)[j] << ",";    //按照点云格式输出
	//	}
	//	file << endl;
	//}

	//file.close();
	//waitKeyEx(0);
	//return 0;
	//}
	Mat R2_matrix(Size(3, 3), CV_64F); //右相机旋转向量
	Mat R2_src = (Mat_<double>(3, 1) << 0.00778951, -0.121633, 0.0150494);
	Rodrigues(R2_src, R2_matrix);   //旋转向量转化为旋转矩阵
	Mat T2_matrix = (Mat_<double>(3, 1) << 105.017, 2.22392, 19.288);   //右相机平移向量


	Mat _src1(points1.size(), 1, CV_32FC2);
	Mat _src2(points1.size(), 1, CV_32FC2);
	
	Vec2f dd;
	for (int i = 0; i < points1.size(); i++) {
		dd[0] = points1[i].x;
		dd[1] = points1[i].y;
		_src1.at<Vec2f>(i, 0) = dd;
	}
	for (int i = 0; i < points2.size(); i++) {
		dd[0] = points2[i].x;
		dd[1] = points2[i].y;
		_src2.at<Vec2f>(i, 0) = dd;
	}

	Mat _dst1;
	Mat _dst2;

	//畸变矫正(将像素坐标转换为了矫正过的图像坐标)
	undistortPoints(_src1, _dst1, m1_matrix, dist1); //校正后的坐标需要乘以焦距+中心坐标变为矫正后的像素坐标
	undistortPoints(_src2, _dst2, m2_matrix, dist2);
	
	////存储二维像素点
	//ofstream PointsL;
	//PointsL.open("PointsL.txt");
	//ofstream PointsR;
	//PointsR.open("PointsR.txt");

	//Vec2f ee,ef;
	//for (size_t i = 0; i < points1.size(); i++) {
	//	ee = _dst1.at<Vec2f>(i, 0);
	//	PointsL << ee[0] << " " << ee[1] << endl;

	//	ef = _dst2.at<Vec2f>(i, 0);
	//	PointsR << ef[0] << " " << ef[1] << endl;

	//}

	//立体校正，校正后的立体相机光轴平行，且行逐行对齐
	Mat Rl, Rr, Pl, Pr, Q;
	stereoRectify(m1_matrix, dist1, m2_matrix, dist2, picture1.size(), R2_matrix, T2_matrix, Rl, Rr, Pl, Pr, Q, 0, -1, picture1.size());

	debug << m1_matrix << endl << m2_matrix << endl << Pl << endl << Pr << endl;

	//分别投影原立体相机图像坐标到校正后立体相机图像坐标
	Mat iRl(3, 3, CV_64F);
	Mat iRr(3, 3, CV_64F);
	for (int i = 0; i<3; i++)
	{
		for (int j = 0; j<3; j++)
		{
			iRl.at<double>(i, j) = Pl.at<double>(i, j);//取Pl的-2列所构成的*3矩阵与Rl相乘获得从原左相机平面图像到矫正后左相机平面图像的转换矩阵
			iRr.at<double>(i, j) = Pr.at<double>(i, j);
		}
	}

	iRl = iRl * Rl;
	iRr = iRr * Rr;

	//根据Pl,PR,Q矩阵，计算像平面矫正后的特征点对坐标(x,y)和左右视差d
	Mat Prec3D(4, 1, CV_64F);
	Mat Pworld3D(4, 1, CV_64F);
	double fxl, fyl, cxl, cyl;
	double fxr, fyr, cxr, cyr;
	//投影相机中心
	//左相机
	fxl = m1_matrix.at<double>(0, 0);
	fyl = m1_matrix.at<double>(1, 1);
	cxl = m1_matrix.at<double>(0, 2);
	cyl = m1_matrix.at<double>(1, 2);
	//右相机
	fxr = m2_matrix.at<double>(0, 0);
	fyr = m2_matrix.at<double>(1, 1);
	cxr = m2_matrix.at<double>(0, 2);
	cyr = m2_matrix.at<double>(1, 2);

	//file << "Num : "<<points1.size() << endl;

	double rr[3][3] = { { 1,0,0 },{ 0,1,0 },{ 0,0,1 } };
	Mat RR1(3, 3, CV_64F, rr); //旋转矩阵
	Mat RR(3, 1, CV_64F); //旋转向量
	Rodrigues(RR1, RR);
	Mat TT = (Mat_<double>(3, 1) << 0, 0, 0);   //平移向量

    //三维点坐标
	vector<Point3d>coord_points;

	//生成的重投影二维坐标
	vector<Point2d>res_2d;

	ofstream dev_txt;
	dev_txt.open("dev.txt");

	//ofstream of;
	//of.open("trianglePoints.txt");
	//Mat s;
	//triangulatePoints(Pl, Pr, _dst1, _dst2, s);

	//for (int p = 0; p < s.cols; p++) {

		////1.公式
		//Mat_<float>col = s.col(p);
		//
		////像素坐标转换为图像坐标
		//dd = _dst1.at<Vec2f>(p, 0);
  //      //double x1_ = (dd[0]-Lcx)/fx1;
  //      //double y1_ = (dd[1]-Lcy)/fy1;

		//double x1_ = dd[0]*fx1+Lcx;
		//double y1_ = dd[1]*fy1+Lcy;

		//double px1 = col(0) / col(3);
		//double py1 = col(1) / col(3);
		//double pz1 = col(2) / col(3);

		////double p_x = px1 / pz1;
		////double p_y = py1 / pz1;
		//double p_x = px1 * fx1 / pz1 + Lcx;
		//double p_y = py1 * fy1 / pz1 + Lcy;

		////匹配点二维坐标
  //      vector<Point2d>match_2d;
  //      Mat tem1 = (Mat_<double>(2, 1) << x1_, y1_);
  //      match_2d = Mat_<Point2d>(tem1);

		////重投影点二维坐标
		//vector<Point2d>pro_2d;
		//Mat tem2 = (Mat_<double>(2, 1) << p_x, p_y);
		//pro_2d = Mat_<Point2d>(tem2);

		////重投影误差大小
  //      double dev;
  //      dev = norm(pro_2d, match_2d, CV_L2);
		//
		//of << px1 << " " << py1 << " " << pz1  << endl;

		//dev_txt << " pro_2d=" << pro_2d << " ; " << "matcher=" << match_2d << " ; " << "dev=" << dev << endl;  //输出偏差

		////2.projectPoints()
		//Mat_<float>col = s.col(p);
		//of << col(0) / col(3) << " " << col(1) / col(3) << " " << col(2) / col(3)  << endl;

		//dd = _src1.at<Vec2f>(p, 0);

		//double x1_ = dd[0]-Lcx;
		//double y1_ = dd[1]-Lcy;

		////匹配点二维坐标
		//vector<Point2d>match_2d;
		//Mat tem1 = (Mat_<double>(2, 1) << x1_, y1_);
		//match_2d = Mat_<Point2d>(tem1);

		//Mat cord = (Mat_<double>(3, 1) << col(0)/col(3) , col(1)/col(3), col(2)/ col(3));
		//coord_points = Mat_<Point3d>(cord);

		//projectPoints(coord_points, R2_src, T2_matrix, m1_matrix, dist1, res_2d);

		////重投影误差大小
		//double dev;
		//dev = norm(res_2d, match_2d, CV_L1);

		//dev_txt << "coord_3D=" << coord_points << " ; " << " res_2d=" << res_2d << " ; " << "matcher=" << match_2d << " ; " << "dev=" << dev << endl;  //输出偏差
//	}

	for (int i = 0; i < points1.size(); i++) {

		dd = _dst1.at<Vec2f>(i, 0);
		double x1_ = dd[0];
		double y1_ = dd[1];
		Mat xx = (Mat_<double>(3, 1) << x1_, y1_, 1);
		Mat xx1(3, 1, CV_64F);
		xx1 = iRl * xx; //矫正后的坐标！
		double x1 = xx1.at<double>(0, 0) / xx1.at<double>(2, 0);
		double y1 = xx1.at<double>(1, 0) / xx1.at<double>(2, 0);

		dd = _dst2.at<Vec2f>(i, 0);
		double x2_ = dd[0];
		double y2_ = dd[1];
		Mat yy = (Mat_<double>(3, 1) << x2_, y2_, 1);
		Mat yy1(3, 1, CV_64F);
		yy1 = iRr * yy;
		double x2 = yy1.at<double>(0, 0) / yy1.at<double>(2, 0);
		double y2 = yy1.at<double>(1, 0) / yy1.at<double>(2, 0);

		//(x1,y1,d,1)构成视差矢量
		Prec3D.at<double>(0, 0) = x1;
		Prec3D.at<double>(1, 0) = y1;
		Prec3D.at<double>(2, 0) = x1 - x2;
		Prec3D.at<double>(3, 0) = 1;
		Pworld3D = Q * Prec3D;
		x1 = Pworld3D.at<double>(0, 0);
		y1 = Pworld3D.at<double>(1, 0);
		double z1 = Pworld3D.at<double>(2, 0);
		double w1 = Pworld3D.at<double>(3, 0);
		double wx = -x1 / w1;
		double wy = -y1 / w1;
		double wz = -z1 / w1;

		////畸变后的像素坐标
		//double x_a = x1_ * fx1 + Lcx;
		//double y_a = y1_ * fy1 + Lcy;
		
		//图像坐标转为像素坐标(一个标定格子14mm)
		double x_a = x1 / 0.28 + Lcx;
		double y_a = y1 / 0.28 + Lcy;


		//匹配点二维坐标
		vector<Point2d>match_2d;
		Mat tem1 = (Mat_<double>(2, 1) << x_a,y_a);
		match_2d = Mat_<Point2d>(tem1);

		Mat cord = (Mat_<double>(3, 1) <<wx,wy,wz);
		coord_points = Mat_<Point3d>(cord);
		
		//生成的重投影二维坐标
		vector<Point2d>res_2d;

		double u1 = fx1/0.28 * wx / wz +Lcx;
		double v1 = fy1/0.28 * wy / wz +Lcy;
		Mat ppp = (Mat_< double > (2, 1) << u1, v1);
		res_2d = Mat_<Point2d>(ppp);
		
		//projectPoints(coord_points, RR, TT, m1_matrix, dist1, res_2d);

		//res_2d[0].x = (res_2d[0].x - Lcx) / fx1;
		//res_2d[0].y = (res_2d[0].y - Lcy) / fy1;

		//重投影误差大小
		double dev;
		dev = norm(res_2d, match_2d, CV_L2);

		dev_txt << "coord_3D=" << coord_points << " ; " << " res_2d=" << res_2d << " ; " << "matcher=" << match_2d << " ; " << "dev=" << dev << endl;  //输出偏差值

		file << wx << "," << wy << "," << wz << endl;
	}


	file,debug.close();
	waitKey(0);
	return 0;
}

