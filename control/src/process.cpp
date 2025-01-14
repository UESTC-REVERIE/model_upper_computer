#include <iostream>
#include <stdlib.h>
#include <cmath>
#include <array>
#include <chrono>
#include "process.h"
#include "pid.h"
#include "comm.h"

#define USE_VIDEO 0

bool r_circle_use = true;
bool l_circle_use = true;
bool l_circle_big_circle = true;
bool r_circle_big_circle = true;
bool video_ok = false;

Mat get_frame(VideoCapture cap) {
	Mat frame;
	cap.read(frame);
	return frame;
}

float MainImage::dy_forward_dist_up(float kp, float kd, float coef, float exp, float bias, 
									float speed_thresh, float max)
{
	float speed_error = 0, last_speed_error = 0, new_forward_dist;
	last_speed_error = last_enc_speed - speed_thresh;
	if(enc_speed > speed_thresh)
	{
		speed_error = enc_speed - speed_thresh;
		speed_error = kp * speed_error + kd * (speed_error - last_speed_error);
		if(speed_error < 0)  speed_error = 0;
		new_forward_dist = bias + coef * pow(speed_error,exp) / 1000.0;
		if(new_forward_dist > max)
		{
			new_forward_dist = max;
		}
	}
	else
	{
		new_forward_dist = bias;
	}
	return new_forward_dist;
}

float MainImage::dy_forward_dist_up_and_down(float kp, float kd, float coef_up, float coef_down, float exp_up, 
											 float exp_down, float bias, float speed_thresh, float max, float min)
{
	float speed_error = 0, last_speed_error = 0, new_forward_dist;
	last_speed_error = last_enc_speed - speed_thresh;
	speed_error = enc_speed - speed_thresh;
	speed_error = kp * speed_error + kd * (speed_error - last_speed_error);
	if(speed_error >= 0)
	{
		new_forward_dist = bias + coef_up * pow(speed_error,exp_up) / 1000.0;
		if(new_forward_dist > max)
		{
			new_forward_dist = max;
		}
	}
	else
	{
		new_forward_dist = bias - coef_down * pow(-speed_error,exp_up) / 1000.0;
		if(new_forward_dist < min)
		{
			new_forward_dist = min;
		}
	}
	return new_forward_dist;
}

float MainImage::AngelDeviation(void)
{
	int i, j;
	int center = IMGW / 2;
	// center += 10;
	//int center=(IMGW/2+last_center)/2;
	long long sum;
	
	if (state_out == bomb_find) {
		MainImage::deviation_thresh = IMGH - re.cone.dist;
		sum = 0;
		for (i = MainImage::deviation_thresh; i > MainImage::deviation_thresh - re.cone.up_scope; i--) {
			sum += center_point[i];
		}
		angle_deviation = re.main.forward_coef1 * (center - float(sum) / re.cone.up_scope);
		
		sum = 0;
		for (i = MainImage::deviation_thresh + re.cone.down_scope; i > MainImage::deviation_thresh; i--) {
			sum += center_point[i];
		}
		angle_deviation += re.main.forward_coef2 * (center - float(sum) / re.cone.down_scope);
		cout << "deviation: " << angle_deviation << endl;
	}else if(state_out == right_garage_find || state_out == left_garage_find){
		MainImage::deviation_thresh = IMGH - re.cone.garage_dist;
		sum = 0;
		for (i = MainImage::deviation_thresh; i > MainImage::deviation_thresh - re.cone.up_scope; i--) {
			sum += center_point[i];
		}
		angle_deviation = re.main.forward_coef1 * (center - float(sum) / re.cone.up_scope);
		
		sum = 0;
		for (i = MainImage::deviation_thresh + re.cone.down_scope; i > MainImage::deviation_thresh; i--) {
			sum += center_point[i];
		}
		angle_deviation += re.main.forward_coef2 * (center - float(sum) / re.cone.down_scope);
		cout << "deviation: " << angle_deviation << endl;		
	}
	else {
		//速度大于阈值时对前瞻进行动态调整:forward_dist up_scope forward_coef1
		double speed_error;

		if (state_out == right_circle) 
		{
			if (MI.state_r_circle == right_circle_inside_before ||
				MI.state_r_circle == right_circle_in_circle ||
				MI.state_r_circle == right_circle_in_strai)
			{
				MainImage::angle_new_forward_dist = 
					dy_forward_dist_up_and_down(re.r_circle.angle_forward_dist_kp, re.r_circle.angle_forward_dist_kd,
												re.r_circle.dy_forward_dist_coef_up, re.r_circle.dy_forward_dist_coef_down,
												re.r_circle.dy_forward_dist_exp_up, re.r_circle.dy_forward_dist_exp_down,
												re.r_circle.circle_dist, re.r_circle.speed * ENC_SPEED_SCALE,
												re.r_circle.max_dy_forward_dist, re.r_circle.min_dy_forward_dist);
				cout << "forward_dist is: " << MainImage::angle_new_forward_dist << endl;
				MainImage::deviation_thresh = IMGH - MainImage::angle_new_forward_dist;
			}
			else
			{
				MainImage::deviation_thresh = IMGH - re.r_circle.circle_dist;
			}
		}
		else if (state_out == left_circle) 
		{
			if (MI.state_l_circle == left_circle_inside_before ||
				MI.state_l_circle == left_circle_in_circle ||
				MI.state_l_circle == left_circle_in_strai)
			{
				MainImage::angle_new_forward_dist = 
					dy_forward_dist_up_and_down(re.l_circle.angle_forward_dist_kp, re.l_circle.angle_forward_dist_kd,
												re.l_circle.dy_forward_dist_coef_up, re.l_circle.dy_forward_dist_coef_down,
												re.l_circle.dy_forward_dist_exp_up, re.l_circle.dy_forward_dist_exp_down,
												re.l_circle.circle_dist, re.l_circle.speed * ENC_SPEED_SCALE,
												re.l_circle.max_dy_forward_dist, re.l_circle.min_dy_forward_dist);
				cout << "forward_dist is: " << MainImage::angle_new_forward_dist << endl;
				MainImage::deviation_thresh = IMGH - MainImage::angle_new_forward_dist;
			}
			else
			{
				MainImage::deviation_thresh = IMGH - re.l_circle.circle_dist;
			}
		}
		else
		{
			MainImage::angle_new_forward_dist = 
				dy_forward_dist_up(re.main.angle_dy_forward_dist_kp,re.main.angle_dy_forward_dist_kd,
								   re.main.angle_enc_forward_dist_coef,re.main.angle_enc_forward_dist_exp,
								   re.main.forward_dist, re.main.angle_enc_forward_threshold,
								   re.main.angle_max_enc_forward_dist);
			cout << "forward_dist is: " << MainImage::angle_new_forward_dist << endl;
			MainImage::deviation_thresh = IMGH - MainImage::angle_new_forward_dist;
		}
		while ( MainImage::deviation_thresh < center_lost && MainImage::deviation_thresh < IMGH - 1)
		{
			MainImage::deviation_thresh ++;
		}
		
		sum = 0;
		for (i = MainImage::deviation_thresh; i > MainImage::deviation_thresh - re.main.up_scope; i--) {
			sum += center_point[i];
		}
  		angle_deviation = re.main.forward_coef1 * (center - float(sum) / re.main.up_scope);
		sum = 0;
		for (i = MainImage::deviation_thresh + re.main.down_scope; i > MainImage::deviation_thresh; i--) {
			sum += center_point[i];
		}
		angle_deviation += re.main.forward_coef2 * (center - float(sum) / re.main.down_scope);
		// deviation += 18.0;
		//if (deviation < -15) deviation -= 4;
		//else if (deviation > 15) deviation += 4;
		cout << "deviation: " << angle_deviation << endl;
	
	}
	return angle_deviation;
}
float MainImage::SpeedDeviation(void)
{
	
	int i, j;
	int center = IMGW / 2;
	// center += 10;
	//int center=(IMGW/2+last_center)/2;
	long long sum;

	//速度大于阈值时对前瞻进行动态调整:forward_dist forward_coef1
	double new_forward_dist_far =
		dy_forward_dist_up(re.main.speed_dy_forward_dist_kp, re.main.speed_dy_forward_dist_kd,
						   re.main.speed_enc_forward_dist_coef, re.main.speed_enc_forward_dist_exp,
						   re.main.speed_forward_dist_far, re.main.speed_enc_forward_threshold,
						   re.main.speed_max_enc_forward_dist_far);
	cout << "speed new_forward_dist_far is: " << new_forward_dist_far << endl;
	double new_forward_dist_near =
		dy_forward_dist_up(re.main.speed_dy_forward_dist_kp, re.main.speed_dy_forward_dist_kd,
						   re.main.speed_enc_forward_dist_coef, re.main.speed_enc_forward_dist_exp,
						   re.main.speed_forward_dist_near, re.main.speed_enc_forward_threshold,
						   re.main.speed_max_enc_forward_dist_near);

	cout << "speed new_forward_dist_near is: " << new_forward_dist_near << endl;
	MainImage::speed_deviation_thresh_far = IMGH - new_forward_dist_far;	
	MainImage::speed_deviation_thresh_near = IMGH - new_forward_dist_near;	

	sum = 0;
	// update_forward_dist();
	for (i = MainImage::speed_deviation_thresh_far; i > MainImage::speed_deviation_thresh_far - re.main.up_scope; i--) {
		sum += center_point[i];
	}
	//根据速度动态调整权重
	
	speed_deviation_far = re.main.forward_coef1 * (center - float(sum) / re.main.up_scope);
	sum = 0;
	for (i = MainImage::speed_deviation_thresh_far + re.main.down_scope; i > MainImage::speed_deviation_thresh_far; i--) {
		sum += center_point[i];
	}
	speed_deviation_far += re.main.forward_coef2 * (center - float(sum) / re.main.down_scope);
	// deviation += 18.0;
	//if (deviation < -15) deviation -= 4;
	//else if (deviation > 15) deviation += 4;
	cout << "speed speed_deviation_far: " << speed_deviation_far << endl;

	sum = 0;
	// update_forward_dist();
	for (i = MainImage::speed_deviation_thresh_near; i > MainImage::speed_deviation_thresh_near - re.main.up_scope; i--) {
		sum += center_point[i];
	}
	//根据速度动态调整权重
	
	speed_deviation_near = re.main.forward_coef1 * (center - float(sum) / re.main.up_scope);
	sum = 0;
	for (i = MainImage::speed_deviation_thresh_near + re.main.down_scope; i > MainImage::speed_deviation_thresh_near; i--) {
		sum += center_point[i];
	}
	speed_deviation_near += re.main.forward_coef2 * (center - float(sum) / re.main.down_scope);
	// deviation += 18.0;
	//if (deviation < -15) deviation -= 4;
	//else if (deviation > 15) deviation += 4;
	cout << "speed speed_deviation_near: " << speed_deviation_near << endl;
	
	return speed_deviation_near;
}
ImageStorage::ImageStorage()
{
#if USE_VIDEO == 1
	cap.open("./sample.avi");//这个路径就是当前路径的意思
#else
	int video_index = 0;
	while(!(video_ok || video_index > 10))
	{
		cap.open("/dev/video" + to_string(video_index), cv::CAP_V4L2);
		if (!cap.isOpened()) {
			video_index ++;
		}
		else
		{
			video_ok = true;
		}
	}
#endif
	if (!video_ok) {
		std::cout << "An error occured!!!" << std::endl;
	}
	cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));  //'M', 'J', 'P', 'G'
	cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
	cap.set(cv::CAP_PROP_FPS, 30);

	fut = async(launch::async, get_frame, cap);
}

void ImageStorage::get_image(int x, int y, int w, int h, int ai_x, int ai_y, int ai_w, int ai_h, bool f)
{
	cnt++;
	cone_flag = false;
	bool success;
	static std::chrono::time_point<std::chrono::high_resolution_clock> last_frame_ts = std::chrono::high_resolution_clock::now();
	cv::Mat frame, frame_tmp;
	vector<cv::Mat> image_split;
	auto start = std::chrono::high_resolution_clock::now();
	do {
		success = true;
#if USE_VIDEO == 1
		std::chrono::duration<double, std::milli> elapsed = std::chrono::high_resolution_clock::now() - last_frame_ts;
		if(elapsed.count() > 1000.0 / 60.0)
		{
			std::cout << "Frame Elapsed Time: " << elapsed.count() << " ms" << std::endl;
			frame = fut.get();
			fut = async(launch::async, get_frame, cap);
			if (frame.empty()) {
				success = false;
				cout << "frame is empty" << endl;
				stop = true;
				return;
			}
			else
			{
				last_frame_ts = std::chrono::high_resolution_clock::now();
			}
			if(elapsed.count() > 200)
			{
				stop = true;
				return;
			}
		}
		else
		{
			success = false;
		}
#else
		success = true;
		frame = fut.get();
		fut = async(launch::async, get_frame, cap);
		if (frame.empty()) {
			success = false;
		}
#endif
	} while (!success);
	//cv::Rect roi(50, 180, 540, 280);
	cv::Rect roi(x, y, w, h);
	frame = frame + Scalar(20, 20 ,20);
	image_BGR = frame;
	frame = frame(roi);
	image_BGR_small = frame.clone();
	resize(image_BGR_small, image_BGR_small, cv::Size(IMGW, IMGH));//120*160	
	split(image_BGR_small, image_split);
	image_R = image_split[2];
	image_B = image_split[0];
	threshold(image_R, image_mat, 0, 255, THRESH_BINARY | THRESH_OTSU);//大津法
	threshold(image_B, image_mat_cone, 0, 255, THRESH_BINARY | THRESH_OTSU);//大津法

	cv::Rect roi2(ai_x, ai_y, ai_w, ai_h);
	image_BGR = image_BGR(roi2);
	resize(image_BGR, image_BGR, cv::Size(300, 200));

	// int conerow[3]={0,0,0},conecol[3]={0,0,0};
	// int conenum[3]={0,0,0};
	// int maxrow[3]={0,0,0},minrow[3]={IMGH,IMGH,IMGH},maxcol[3]={0,0,0},mincol[3]={IMGW,IMGW,IMGW};
	// int grey[IMGH][IMGW];
	// int cone[IMGH][IMGW]={0};
	// int nncol = 0;
	// cout << image_mat.size().height << "," << image_mat.size().width << endl;
	// while(nncol < IMGW-1)  
	// {  
	// 	int nnrow = 0;
	// 	while(nnrow < IMGH-1)  
	// 	{  
	// 		cv::Vec3b val = image_BGR_small.at<cv::Vec3b>(nnrow, nncol);
	// 		if(val[0]<=50 && val[1]>=120 && val[2]>=120)
	// 			{
	// 				int row_up=0,row_down=0,col_up=0,col_down=0;
	// 				int white_num=0;
	// 				cv::Vec3b val2 = image_BGR_small.at<cv::Vec3b>(nnrow, nncol);
	// 				while(!(val2[0]<=150 && val2[1]<=100 && val2[2]<=50)&&!(val2[0]>120 && val2[1]>=120 && val2[2]>=120))
	// 				{
	// 					row_up++;
	// 					val2 = image_BGR_small.at<cv::Vec3b>((nnrow+row_up), nncol);
	// 					if(val2[0]>120 && val2[1]>=120 && val2[2]>=120)
	// 					{
	// 						white_num++;
	// 					}
	// 				}
	// 				val2 = image_BGR_small.at<cv::Vec3b>(nnrow, nncol);
	// 				while(!(val2[0]<=150 && val2[1]<=100 && val2[2]<=50)&&!(val2[0]>120 && val2[1]>=120 && val2[2]>=120))
	// 				{
	// 					row_down++;
	// 					val2 = image_BGR_small.at<cv::Vec3b>((nnrow-row_down), nncol);
	// 					if(val2[0]>120 && val2[1]>=120 && val2[2]>=120)
	// 					{
	// 						white_num++;
	// 					}
	// 				}
	// 				val2 = image_BGR_small.at<cv::Vec3b>(nnrow, nncol);
	// 				while(!(val2[0]<=150 && val2[1]<=100 && val2[2]<=50)&&!(val2[0]>120 && val2[1]>=120 && val2[2]>=120))
	// 				{
	// 					col_up++;
	// 					val2 = image_BGR_small.at<cv::Vec3b>(nnrow, (nncol+col_up));
	// 					if(val2[0]>120 && val2[1]>=120 && val2[2]>=120)
	// 					{
	// 						white_num++;
	// 					}
	// 				}
	// 				val2 = image_BGR_small.at<cv::Vec3b>(nnrow, nncol);
	// 				while(!(val2[0]<=150 && val2[1]<=100 && val2[2]<=50)&&!(val2[0]>120 && val2[1]>=120 && val2[2]>=120))
	// 				{
	// 					col_down++;
	// 					val2 = image_BGR_small.at<cv::Vec3b>(nnrow, (nncol-col_down));
	// 					if(val2[0]>120 && val2[1]>=120 && val2[2]>=120)
	// 					{
	// 						white_num++;
	// 					}
	// 				}
	// 				cout<<"white_num"<<white_num<<endl;
	// 				if(white_num>=4)
	// 				{
	// 					grey[nnrow][nncol]=1;
	// 					if(conenum[0]==0 || (pow(conerow[0]-nnrow,2)+pow(conecol[0]-nncol,2))<pow((20+0.15*nnrow),2))
	// 					{
	// 						conenum[0]++;
	// 						// if(nncol>maxcol)
	// 						// 	maxcol=nncol;
	// 						// if(nncol<mincol)
	// 						// 	mincol=nncol;
	// 						if(nnrow>maxrow[0])
	// 							maxrow[0]=nnrow;
	// 						// if(nnrow<minrow)
	// 						// 	minrow=nnrow;
	// 						conerow[0]=(conerow[0]*(conenum[0]-1)+nnrow)/conenum[0];
	// 						conecol[0]=(conecol[0]*(conenum[0]-1)+nncol)/conenum[0];
	// 					}
	// 					else if(conenum[1]==0 || (pow(conerow[1]-nnrow,2)+pow(conecol[1]-nncol,2))<pow((20+0.15*nnrow),2))
	// 					{
	// 						conenum[1]++;
	// 						// if(nncol>maxcol)
	// 						// 	maxcol=nncol;
	// 						// if(nncol<mincol)
	// 						// 	mincol=nncol;
	// 						if(nnrow>maxrow[1])
	// 							maxrow[1]=nnrow;
	// 						// if(nnrow<minrow)
	// 						// 	minrow=nnrow;
	// 						conerow[1]=(conerow[1]*(conenum[1]-1)+nnrow)/conenum[1];
	// 						conecol[1]=(conecol[1]*(conenum[1]-1)+nncol)/conenum[1];
	// 					}
	// 					else if(conenum[2]==0 || (pow(conerow[2]-nnrow,2)+pow(conecol[2]-nncol,2))<pow((20+0.15*nnrow),2))
	// 					{
	// 						conenum[2]++;
	// 						// if(nncol>maxcol)
	// 						// 	maxcol=nncol;
	// 						// if(nncol<mincol)
	// 						// 	mincol=nncol;
	// 						if(nnrow>maxrow[2])
	// 						maxrow[2]=nnrow;
	// 						// if(nnrow<minrow)
	// 						// 	minrow=nnrow;
	// 						conerow[2]=(conerow[2]*(conenum[2]-1)+nnrow)/conenum[2];
	// 						conecol[2]=(conecol[2]*(conenum[2]-1)+nncol)/conenum[2];
	// 					}
	// 				}
	// 			}
	// 		else
	// 			{
	// 				grey[nnrow][nncol]=0;
	// 			}
	// 		nnrow++;
	// 	}  
	// 	nncol++;
	// } 
	// conerow[0]=maxrow[0];
	// // conecol=maxcol;
	// // conecol[0]=(conecol[0]+160)/3;
	// conerow[1]=maxrow[1];
	// // conecol=maxcol;
	// // conecol[1]=(conecol[1]+160)/3;
	// conerow[2]=maxrow[2];
	// // conecol=maxcol;
	// // conecol[2]=(conecol[2]+160)/3;
	// cout<<conerow[0]<<";"<<conecol[0]<<endl;
	// cout<<conerow[1]<<";"<<conecol[1]<<endl;
	// cout<<conerow[2]<<";"<<conecol[2]<<endl;
	// cout<<conenum[0]<<conenum[1]<<conenum[2]<<endl;
	// // int cone_used_center[IMGH]={0};
	// for(int nn=0;nn<3;nn++)
	// {
	// 	if(conenum[nn]>5)
	// 	{
	// 		cone_flag = true;
	// 		if(cnt==1)
	// 		{
	// 			for(int i=0;i<IMGH;i++)
	// 			{
	// 				global_center[i]=80;
	// 			}
	// 		}
	// 		else
	// 		{
	// 			// for(int i=0;i<IMGH;i++)
	// 			// {
	// 			// 	cone_used_center[i]=global_center[i];
	// 			// }
	// 		}
	// 		if(conecol[nn]>global_center[conerow[nn]])
	// 		{
	// 			for(int nnrow=conerow[nn]-120;nnrow<conerow[nn]+120;nnrow++)
	// 			{
	// 				for(int nncol=conecol[nn]-15;nncol<159;nncol++)
	// 				{
					
	// 					int nowrow=nnrow;
	// 					int nowcol=nncol;
	// 					if(nowrow<1)
	// 					{
	// 						nowrow=1;
	// 					}
	// 					if(nowrow>119)
	// 					{
	// 						nowrow=119;
	// 					}
	// 					if(nowcol<1)
	// 					{
	// 						nowrow=1;
	// 					}
	// 					if(nowcol>159)
	// 					{
	// 						nowcol=159;
	// 					}
	// 					if(nowcol>(conecol[nn]-3-0.25*conerow[nn]+3*(abs(nowrow*1.0-conerow[nn]*1.0)/(1+0.05*conerow[nn]))))
	// 					{
	// 						image_mat.at<uchar>(nowrow, nowcol) = uchar(0);
	// 					}
	// 				}
	// 			}
	// 		}
	// 		else
	// 		{
	// 			for(int nnrow=conerow[nn]-120;nnrow<conerow[nn]+120;nnrow++)
	// 			{
	// 				for(int nncol=conecol[nn]+15;nncol>0;nncol--)
	// 				{
					
	// 					int nowrow=nnrow;
	// 					int nowcol=nncol;
	// 					if(nowrow<1)
	// 					{
	// 						nowrow=1;
	// 					}
	// 					if(nowrow>119)
	// 					{
	// 						nowrow=119;
	// 					}
	// 					if(nowcol<1)
	// 					{
	// 						nowrow=1;
	// 					}
	// 					if(nowcol>159)
	// 					{
	// 						nowcol=159;
	// 					}
	// 					 if(nowcol<(conecol[nn]+3+0.25*conerow[nn]-3*(abs(nowrow*1.0-conerow[nn]*1.0)/(1+0.05*conerow[nn]))))
	// 					{
	// 						image_mat.at<uchar>(nowrow, nowcol) = uchar(0);
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
	// }
	if (f)
	{
		Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
		morphologyEx(image_mat, image_mat, MORPH_CLOSE, kernel);//腐蚀膨胀
	}
}
//构造函数，进行初始化操作
MainImage::MainImage() : re("./config.yaml")
{
	init(true);
	left_end_point.reserve(16);
	right_end_point.reserve(16);
	state_out = start_state;//straight;//garage_out;
	state_r_circle = right_circle_in_find;
	state_l_circle = left_circle_in_find;
	state_turn_state = turn_slow_down;
	state_repair = repair_in_find;
	state_farm = farm_in_find;
	state_hump = hump_in_find;
	state_end_line = end_line_find;
	state_hill = hill_on;
	state_right_garage = right_garage_before;
	state_left_garage = left_garage_before;
	state_cone = cone_slowdown;//cone_common;
	last_center = IMGW / 2;
	count_circle = 0;
	zebra_far_find = false;
	zebra_near_find = false;
	
	ai_bridge = false;
	ai_tractor = false;
	ai_corn = false;
	ai_pig = false;

    ai_bomb = false;
	ai_right_garage = false;
	ai_left_garage = false;

}
/**
 * @brief 初始化特征点
*/
void MainImage::init(bool complete = true)
{

	for (int i = 0; i < IMGH; i++)
	{
		left_edge_point[i] = 0;
		right_edge_point[i] = IMGW - 1;
		center_point[i] = 0;

		exist_left_edge_point[i] = false;
		exist_right_edge_point[i] = false;
	}
	if (complete)
	{
		left_end_point.clear();
		right_end_point.clear();
		left_cone.clear();
		right_cone.clear();
		center_cone.clear();
		cone_number.clear();
		left_cone_point.clear();
		right_cone_point.clear();
		lost_left = false;
		lost_right = false;
		left_continue = true;
		right_continue = true;
		left_branch_num = 0;
		right_branch_num = 0;

		center_lost = -1;
	}
}
/**
 * @brief 更新处理后的图像和特征点查询结果
*/
void MainImage::update_image()
{
	if (state_out == straight || state_out == turn_state)
		store.get_image(re.set.cut_point_x, re.set.cut_point_y, re.set.cut_w, re.set.cut_h, re.set.ai_cut_x, re.set.ai_cut_y, re.set.ai_w, re.set.ai_h, false);
	else
		store.get_image(re.set.cut_point_x, re.set.cut_point_y, re.set.cut_w, re.set.cut_h, re.set.ai_cut_x, re.set.ai_cut_y, re.set.ai_w, re.set.ai_h, true);
	init();
	find_edge_point();//找边界点
	find_end_point();//找断点
	if (state_out == straight || state_out == turn_state || (state_out == bomb_find && (state_cone == left_block || state_cone == right_block))) edge_filter(10);
	find_end_point();
	end_filter();
	judge_lost();
	count_branch();
	judge_continuity();
}

void MainImage::state_judge()
{
	if (ai_bridge) {
		ai_bridge = false;
		if (state_hill != hill_out)state_out = hill_find;
	}
	else if (ai_tractor) {
		ai_tractor = false;
		if (state_repair != repair_out_out)state_out = repair_find;
	}
	else if (ai_corn) {
		ai_corn = false;
		if (state_farm != farm_out)state_out = farm_find;
	}
	else if (ai_pig) {
		ai_pig = false;
		if (state_hump != hump_out)state_out = hump_find;
	}
	else if (ai_bomb) {
		ai_bomb = false;
		cerr<<"switch state to bomb_find"<<endl;
		if (state_cone != bomb_out)state_out = bomb_find;
	}
	else if (ai_right_garage){
		cerr<<"right garage!!"<<endl;
		ai_right_garage = false;
		if (state_right_garage != right_garage_out)state_out = right_garage_find;
	}
	else if (ai_left_garage){
		ai_left_garage = false;
		cerr<<"switch state to left_garage_find"<<endl;
		if (state_left_garage != left_garage_out)state_out = left_garage_find;
	}
	// if (store.cone_flag == false){
	// 	state_out = straight;
	// }
	if ((state_out == straight || state_out == turn_state))
	{  
		int b_count=0;
		for(int i = IMGH-1; i > MI.center_lost-5; i--){
			uchar* row_b = MI.store.image_mat_cone.ptr<uchar>(i);
			uchar* row_r = MI.store.image_mat.ptr<uchar>(i);
			if(MI.exist_left_edge_point[i]&& MI.exist_right_edge_point[i]){
				for(int j = MI.left_edge_point[i]+2; j < MI.right_edge_point[i]-2; j++){
					cv::Vec3b val = MI.store.image_BGR_small.at<cv::Vec3b>(i, j);
					
					if(row_b[j] == 0&&row_r[j]!=0){//&&val[0]<140
						Point p;
						p.x = j;
						p.y = i;
						MI.right_cone_point.push_back(p);
						// cv::Vec3b val = MI.store.image_BGR_small.at<cv::Vec3b>(i, j);
						// int r = val[2];
						// int g = val[1];
						// int b = val[0];
						// cerr<<"b"<<b<<"g"<<g<<"r"<<r<<endl;
						b_count++;
					}
					// if(val[0]<=150 && val[1]>=120 && val[2]>=120){
					// 	yellow_count++;
					// }
					// if(val[0]< 150){
					// int r = val[2];
					// int g = val[1];
					// int b = val[0];
					// cout<<"b"<<b<<"g"<<g<<"r"<<r<<endl;
					// }

					// if(b_count > 10){
					// 	cout<<"cone ppint find"<<b_count<<endl;;
					// 	state_out = cone_find;
					// 	// break;
					// }
				}
			}
			// if(state_out == cone_find){
			// 	break;
			// }

			}
			cout<<"bcount"<<b_count<<endl;
			if(b_count > Re.cone.cone_state_count){
				cout<<"cone ppint find"<<b_count<<endl;;
				MI.state_out = bomb_find;
				MI.state_cone = judge_side;
			}		
			


		if(state_out == turn_state)
		{
			if((smoothed_curvature_near < re.turn.turn_curvature_thresh && 
				smoothed_curvature_far < re.turn.turn_curvature_thresh) && 
				abs(speed_deviation_near) <= re.turn.turn_deviation_thresh)
			{
				state_out = straight;
			}
			else if(smoothed_curvature_near > re.turn.turn_curvature_thresh || 
					abs(angle_deviation) > re.turn.turn_deviation_thresh)
			{
				state_out = turn_state;
				state_turn_state = turn_inside;
			}
		}

		if (re.set.zebra_detect) {
			find_far_zebra();
			find_near_zebra();
		}

		if (zebra_far_find) {
			state_out = state_end; //garage_find;
		}
		// else if(store.cone_flag == true){
		// 	state_out = cone_find;
		// }
		else if ((r_circle_use && right_end_point.size() >= 6 && right_branch_num >= 2 && abs(right_end_point[1].y - right_end_point[4].y) > 35)|| (abs(right_end_point[0].y-right_end_point[3].y)>35&& abs(right_end_point[0].y-right_end_point[1].y)>15&&right_end_point[0].y < IMGH-20 && right_end_point.size()==4&&left_end_point.size()==2&&(abs(left_end_point[0].y-left_end_point[1].y)>50)))//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		{
			bool f = true;
			int i;
			for (i = right_end_point[1].y - 10; i > right_end_point[4].y + 10; i--) {
				if (!(exist_left_edge_point[i] && abs(left_edge_point[i] - left_edge_point[i + 1]) < 3)) {
					f = false;
					break;
				}
			}
			if(right_end_point.size()==4){
				for (i = right_end_point[1].y-2; i > right_end_point[1].y - 12; i--) {
					uchar* this_row = store.image_mat.ptr(i); 
					if (this_row[right_end_point[1].x]==0) {
						f = false;
						int jj = f;
						cerr<<"first j"<<jj<<"point"<<endl;;
						break;
					}
				}
				uchar* this_row = store.image_mat.ptr(IMGH-5);
				int w_count=0;
				for(i = 0; i < IMGW-1; i++){
					if(this_row[i]!=0)w_count++;
					
				}
				cerr<<"white_count"<<w_count<<endl;
				if(w_count<IMGW-37)f = false;
				// for (i = (right_end_point[0].y+10>IMGH-1?IMGH-1:right_end_point[0].y+10); i > (right_end_point[0].y+5>IMGH-1?IMGH-1:right_end_point[0].y+5); i--) {
				// 	uchar* this_row = store.image_mat.ptr(i); 
				// 	for(int j = right_end_point[0].x; j > right_end_point[0].x-10; j--){
				// 		if (this_row[j]==0) {
				// 			cerr<<"i is"<<i << "j is"<< j<<endl;
				// 			f = false;
				// 			int jj = f;
				// 			cerr<<"second j"<<jj<<endl;
				// 			break;
				// 		}
				// 	}

				// }
			}

			if (f) {
				state_out = right_circle;
			}
		}
		else if (l_circle_use && left_end_point.size() >= 6 && left_branch_num >= 2 && abs(left_end_point[1].y - left_end_point[4].y) > 35) {
			bool f = true;
			int i;
			for (i = left_end_point[1].y - 10; i > left_end_point[4].y + 10; i--) {
				if (!(exist_right_edge_point[i] && abs(right_edge_point[i] - right_edge_point[i + 1]) < 3)) {
					f = false;
					break;
				}
			}
			if (f) {
				state_out = left_circle;
			}
		}
		else if (((abs(speed_deviation_far) > re.turn.turn_deviation_thresh &&
				   abs(speed_deviation_near) < re.turn.turn_deviation_thresh)) &&
				 !(center_lost > re.turn.inside_centerlost_thresh))
		{
			state_out = turn_state;
			state_turn_state = turn_slow_down;
			cout << "turn_slow_down" << endl;
		}
		else if(abs(speed_deviation_near) > re.turn.turn_deviation_thresh || 
				center_lost > re.turn.inside_centerlost_thresh)
		{
			state_out = turn_state;
			state_turn_state = turn_inside;
			cout << "turn_inside" << endl;
		}
	}
	else if (state_out == state_end) {
		find_far_zebra();
		find_near_zebra();
	}
}

void MainImage::circle_pid_update(uchar cur_status, float& kp, float& kd, float& ki)
{
	switch (cur_status) {
		case left_circle_in_find: {
			kp = re.l_circle.in_find_kp;
			kd = re.l_circle.in_find_kd;
			ki = re.l_circle.in_find_ki;
			break;
		}
		case left_circle_in_strai: {
			kp = re.l_circle.in_strai_kp;
			kd = re.l_circle.in_strai_kd;
			ki = re.l_circle.in_strai_ki;
			break;
		}
		case left_circle_in_circle: {
			kp = re.l_circle.in_circle_kp;
			kd = re.l_circle.in_circle_kd;
			ki = re.l_circle.in_circle_ki;
			break;
		}
		case left_circle_inside_before: {
			kp = re.l_circle.inside_before_kp;
			kd = re.l_circle.inside_before_kd;
			ki = re.l_circle.inside_before_ki;
			break;
		}
		case left_circle_inside: {
			kp = re.l_circle.inside_kp;
			kd = re.l_circle.inside_kd;
			ki = re.l_circle.inside_ki;
			break;
		}
		case left_circle_out_find: {
			kp = re.l_circle.out_find_kp;
			kd = re.l_circle.out_find_kd;
			ki = re.l_circle.out_find_ki;
			break;
		}
		case left_circle_out_strai: {
			kp = re.l_circle.out_strai_kp;
			kd = re.l_circle.out_strai_kd;
			ki = re.l_circle.out_strai_ki;
			break;
		}
		case left_circle_out: {
			kp = re.l_circle.out_kp;
			kd = re.l_circle.out_kd;
			ki = re.l_circle.out_ki;
			break;
		}
		case left_circle_out_out: {
			kp = re.l_circle.out_out_kp;
			kd = re.l_circle.out_out_kd;
			ki = re.l_circle.out_out_ki;
			break;
		}
		case right_circle_in_find: {
			kp = re.r_circle.in_find_kp;
			kd = re.r_circle.in_find_kd;
			ki = re.r_circle.in_find_ki;
			break;
		}
		case right_circle_in_strai: {
			kp = re.r_circle.in_strai_kp;
			kd = re.r_circle.in_strai_kd;
			ki = re.r_circle.in_strai_ki;
			break;
		}
		case right_circle_in_circle: {
			kp = re.r_circle.in_circle_kp;
			kd = re.r_circle.in_circle_kd;
			ki = re.r_circle.in_circle_ki;
			break;
		}
		case right_circle_inside_before: {
			kp = re.r_circle.inside_before_kp;
			kd = re.r_circle.inside_before_kd;
			ki = re.r_circle.inside_before_ki;
			break;
		}
		case right_circle_inside: {
			kp = re.r_circle.inside_kp;
			kd = re.r_circle.inside_kd;
			ki = re.r_circle.inside_ki;
			break;
		}
		case right_circle_out_find: {
			kp = re.r_circle.out_find_kp;
			kd = re.r_circle.out_find_kd;
			ki = re.r_circle.out_find_ki;
			break;
		}
		case right_circle_out_strai: {
			kp = re.r_circle.out_strai_kp;
			kd = re.r_circle.out_strai_kd;
			ki = re.r_circle.out_strai_ki;
			break;
		}
		case right_circle_out: {
			kp = re.r_circle.out_kp;
			kd = re.r_circle.out_kd;
			ki = re.r_circle.out_ki;
			break;
		}
		case right_circle_out_out: {
			kp = re.r_circle.out_out_kp;
			kd = re.r_circle.out_out_kd;
			ki = re.r_circle.out_out_ki;
			break;
		}
	}
}

void MainImage::update_control(float& kp, float& kd, float& ki, int& dv, float& slow_down_kd)
{
	switch (state_out) {
	case left_garage_find:
	case right_garage_find: {
		kp = re.main.kp;
		kd = re.main.kd;
		ki = re.main.ki;
		dv = re.main.dv;
		slow_down_kd = re.garage.slow_down_kd;
		return;		
	}
	case garage_out: {
		kp = re.start.kp;
		kd = re.start.kd;
		ki = re.start.ki;
		dv = re.start.dv;
		slow_down_kd = re.main.slow_down_kd;
		return;
	}
	case straight: {
		kp = re.main.kp;
		kd = re.main.kd;
		ki = re.main.ki;
		dv = re.main.dv;
		slow_down_kd = re.main.slow_down_kd;
		return;
	}
	case turn_state: {
		switch (state_turn_state) {
		case turn_slow_down: {
			kp = re.main.kp;
			kd = re.main.kd;
			ki = re.main.ki;
			dv = re.main.dv;
			slow_down_kd = re.turn.slow_down_kd;
			return;
		}
		case turn_inside: {
			kp = re.main.kp;
			kd = re.main.kd;
			ki = re.main.ki;
			slow_down_kd = re.turn.slow_down_kd;
			return;
		}
		case turn_out: {
			kp = re.main.kp;
			kd = re.main.kd;
			ki = re.main.ki;
			slow_down_kd = re.turn.slow_down_kd;
			return;
		}
		}
		return;
	}
	case bomb_find: {
		switch(state_cone){
			case judge_side:
			case right_block:
			case left_block:{
			kp = re.cone.kp;
			kd = re.cone.kd;
			ki = re.cone.ki;
			slow_down_kd = re.garage.slow_down_kd;
			return;				
			}
			case cone_slowdown:
			{
			kp = re.main.kp;
			kd = re.main.kd;
			ki = re.main.ki;
			slow_down_kd = re.garage.slow_down_kd;				
			}
		}
		return;
	}
	case right_circle: {
		slow_down_kd = re.r_circle.circle_slow_down_kd;
		circle_pid_update(state_r_circle,kp,kd,ki);
		return;
	}
	case left_circle: {
		slow_down_kd = re.l_circle.circle_slow_down_kd;
		circle_pid_update(state_l_circle,kp,kd,ki);
		return;
	}
	case repair_find: {
		kp = re.repair.kp;
		kd = re.repair.kd;
		ki = re.repair.ki;
		dv = re.repair.dv;
		slow_down_kd = re.main.slow_down_kd;
		return;
	}
	case farm_find: {
		kp = re.farm.kp;
		kd = re.farm.kd;
		ki = re.farm.ki;
		dv = re.farm.dv;
		slow_down_kd = re.main.slow_down_kd;
		return;
	}
	case hump_find: {
		kp = re.hump.kp;
		kd = re.hump.kd;
		ki = re.hump.ki;
		dv = re.hump.dv;
		slow_down_kd = re.main.slow_down_kd;
		return;
	}
	case state_end: {
		kp = re.zebra.kp;
		kd = re.zebra.kd;
		ki = re.zebra.ki;
		dv = re.zebra.dv;
		slow_down_kd = re.garage.slow_down_kd;
		return;
	}
	case hill_find: {
		kp = re.hill.kp;
		kd = re.hill.kd;
		ki = re.hill.ki;
		dv = re.hill.dv;
		slow_down_kd = re.main.slow_down_kd;
		return;
	}

	}
}

void MainImage::mend_trunk()
{
	int i;
	if (!lost_left && !lost_right) {
		if (left_end_point.size() > 1) {
			for (i = 1; i < left_end_point.size() - 1; i += 2) {
				line(store.image_mat, left_end_point[i], left_end_point[i + 1]);
			}
			if (left_end_point[0].y < IMGH - 5)
				ray(store.image_mat, left_end_point[0], -1.74f);
		}
		if (right_end_point.size() > 1) {
			for (i = 1; i < right_end_point.size() - 1; i += 2) {
				line(store.image_mat, right_end_point[i], right_end_point[i + 1]);
			}
			if (right_end_point[0].y < IMGH - 5)

				ray(store.image_mat, right_end_point[0], -1.4f);
		}
		// if (right_end_point.size() == 1 && left_end_point.size() == 1){
		// 	if(right_end_point[0].y < left_end_point[0].y){
		// 		ray(store.image_mat, right_end_point[0], re.main.right_ray);
		// 	}
		// 	if(left_end_point[0].y < left_end_point[0].y){
		// 		ray(store.image_mat, right_end_point[0], re.main.left_ray);
		// 	}
		// }
	}
	if (lost_left) {
		for (int k = right_end_point[1].y; k < IMGH - 2; k++) {
			if (right_edge_point[k + 1] > right_edge_point[k]) {
				Point p;
				p.y = k;
				p.x = right_edge_point[k];
				float angle = atan(2.0 / (right_edge_point[k] - right_edge_point[k + 1])) + 3.14f;
				ray(store.image_mat, p, angle);
				break;
			}
		}
	}
	else if (lost_right) {
		for (int k = left_end_point[1].y; k < IMGH - 2; k++) {
			if (left_edge_point[k + 1] < left_edge_point[k]) {
				Point p;
				p.y = k;
				p.x = left_edge_point[k];
				float angle = atan(2.0 / (left_edge_point[k] - left_edge_point[k + 2]));
				ray(store.image_mat, p, angle);
				break;
			}
		}
	}
	if((left_end_point[0].y < 50 && left_end_point[0].x > 40)||(right_end_point[0].y < 50 && right_end_point[0].x < 120)){
		line(store.image_mat, right_end_point[0], left_end_point[0]);
	}
}

void MainImage::mend_right_circle_in_straight()
{
	Point p;
	p.x = IMGW - 1;
	int i;
	for (i = right_end_point[0].y; i > right_end_point[1].y; i--) {
		if (right_edge_point[i] < p.x) {
			p.x = right_edge_point[i];
			p.y = i;
		}
	}
	ray(store.image_mat, p, re.r_circle.in_strai_ray_ag1);
	ray(store.image_mat, right_end_point[2], re.r_circle.in_strai_ray_ag2);
	for (int j = right_end_point[2].y; j > 0; j--){
		exist_left_edge_point[j] = false;
		exist_right_edge_point[j] = false;

	}
}

void MainImage::mend_right_circle_in_circle()
{
	Point p;
	p.x = 0;
	p.y = IMGH - 5;
	if (right_end_point.size() > 0) {
		if (right_end_point[0].x > IMGW - MINX || right_end_point[0].y > IMGH - MINY) {
			if (right_end_point[2].x > IMGW - 26) {
				ray(store.image_mat, p, re.r_circle.in_circle_ray_ag1);
			}
			else {
				Point p1;
				p1.x = 0;
				int i;
				for (i = right_end_point[0].y; i > right_end_point[1].y; i--) {
					if (right_edge_point[i] > p1.x) {
						p1.x = right_edge_point[i];
						p1.y = i;
					}
				}
				ray(store.image_mat, p1, re.r_circle.in_circle_ray_ag2);
				ray(store.image_mat, right_end_point[2], re.r_circle.in_circle_ray_ag3);
				for (int j = right_end_point[2].y; j > 0; j--){
					exist_left_edge_point[j] = false;
					exist_right_edge_point[j] = false;

				}
			}
		}
		else {
			ray(store.image_mat, right_end_point[0], re.r_circle.in_circle_ray_ag4);
			for (int j = right_end_point[0].y; j > 0; j--){
				exist_left_edge_point[j] = false;
				exist_right_edge_point[j] = false;

			}
		}
	}
	else {
		ray(store.image_mat, p, re.r_circle.in_circle_ray_ag5);
	}
}

void MainImage::mend_left_circle_in_straight() {
	Point p;
	p.x = 0;
	int i;
	for (i = left_end_point[0].y; i > left_end_point[1].y; i--) {
		if (left_edge_point[i] > p.x) {
			p.x = left_edge_point[i];
			p.y = i;
		}
	}
	ray(store.image_mat, p, re.l_circle.in_strai_ray_ag1);
	ray(store.image_mat, left_end_point[2], re.l_circle.in_strai_ray_ag2);
}

void MainImage::mend_left_circle_in_circle() {
	Point p;
	p.x = IMGW - 1;
	p.y = IMGH - 5;
	if (left_end_point.size() > 0) {
		if (left_end_point[0].x < MINX || left_end_point[0].y > IMGH - MINY) {
			if (left_end_point[2].x < 26) {
				ray(store.image_mat, p, re.l_circle.in_circle_ray_ag1);
			}
			else {
				Point p1;
				p1.x = 0;
				int i;
				for (i = left_end_point[0].y; i > left_end_point[1].y; i--) {
					if (left_edge_point[i] > p1.x) {
						p1.x = left_edge_point[i];
						p1.y = i;
					}
				}
				ray(store.image_mat, p1, re.l_circle.in_circle_ray_ag2);
				ray(store.image_mat, left_end_point[2], re.l_circle.in_circle_ray_ag3);
			}
		}
		else {
			ray(store.image_mat, left_end_point[0], re.l_circle.in_circle_ray_ag4);
		}
	}
	else {
		ray(store.image_mat, p, re.l_circle.in_circle_ray_ag5);
	}
}

void MainImage::mend_farm_in_find() {
	int i;
	Point p;
	int size = left_cone.size() < right_cone.size() ? left_cone.size() : right_cone.size();
	for (i = 0; i < size; i++)
	{
		p.x = (left_cone[i].x + right_cone[i].x) / 2;
		p.y = (left_cone[i].y + right_cone[i].y) / 2;
		center_cone.push_back(p);
	}
	if (left_cone.size() > 0)ray(store.image_mat, left_cone[0], -1.884, 255);
	if (right_cone.size() > 0)ray(store.image_mat, right_cone[0], -1.256, 255);
	if (left_cone.size() > 0 && right_cone.size() > 0)
	{
		p.x = (left_cone.back().x + right_cone.back().x) / 2;
		p.y = (left_cone.back().y + right_cone.back().y) / 2;
		center_cone.push_back(p);
	}
	if (size > 1)
	{
		for (i = 0; i < size - 1; i++)
		{
			line(store.image_mat, left_cone[i], left_cone[i + 1], 255);
			line(store.image_mat, right_cone[i], right_cone[i + 1], 255);
		}
	}
	if (center_cone.size() > 1)last_center_in_cone = center_cone[1].x;
}

void MainImage::mend_farm_inside() {
	Point p;
	int i;
	int size = left_cone.size() < right_cone.size() ? left_cone.size() : right_cone.size();
	if (left_cone.size() > 0 && right_cone.size() > 0)
	{
		for (i = 0; i < size; i++)
		{
			p.x = (left_cone[i].x + right_cone[i].x) / 2;
			p.y = (left_cone[i].y + right_cone[i].y) / 2;
			center_cone.push_back(p);
		}
	}
	if (left_cone.size() > 0)ray(store.image_mat, left_cone[0], -1.884, 255);
	if (right_cone.size() > 0)ray(store.image_mat, right_cone[0], -1.256, 255);
	if (left_cone.size() > 1)
	{
		for (i = 0; i < left_cone.size() - 1; i++)line(store.image_mat, left_cone[i], left_cone[i + 1], 255);
	}
	if (right_cone.size() > 1)
	{
		for (i = 0; i < right_cone.size() - 1; i++)line(store.image_mat, right_cone[i], right_cone[i + 1], 255);
	}
	if (center_cone.size() > 1)last_center_in_cone = center_cone[1].x;
}

void MainImage::mend_farm_out_find() {
	if (center_cone.size() > 1)
	{
		float x1 = center_cone.back().x;
		float x2 = center_cone[center_cone.size() - 2].x;
		float y1 = center_cone.back().y;
		float y2 = center_cone[center_cone.size() - 2].y;
		float x = center_cone.back().x;
		int m = center_cone.back().y;
		const uchar* r = store.image_mat.ptr<uchar>(m);
		while (r[(int)x] == 0)
		{
			x -= (x2 - x1) / (y2 - y1);
			if (x < 0)x = 0;
			else if (x > IMGW - 1)x = IMGW - 1;
			m--;
			r = store.image_mat.ptr<uchar>(m);
		}
		Point p;
		p.x = x;
		p.y = m;
		center_cone.push_back(p);
		cout << p.x << " " << p.y << endl;
	}
	else if (center_cone.size() == 1 && center_cone[0].y < IMGH - 5)
	{
		float x1 = center_cone[0].x;
		float x2 = IMGW / 2;
		float y1 = center_cone[0].y;
		float y2 = IMGH - 5;
		float x = center_cone[0].x;
		int m = center_cone[0].y;
		const uchar* r = store.image_mat.ptr<uchar>(m);
		while (r[(int)x] == 0)
		{
			x -= (x2 - x1) / (y2 - y1);
			if (x < 0)x = 0;
			else if (x > IMGW - 1)x = IMGW - 1;
			m--;
			r = store.image_mat.ptr<uchar>(m);
		}
		Point p;
		p.x = x;
		p.y = m;
		center_cone.push_back(p);
		cout << p.x << " " << p.y << endl;
	}
}

void MainImage::mend_in_hump_on() {
	int i;
	int right = IMGW - 1, left = 0, center = IMGW / 2;
	int r_up_y = IMGH - re.hump.mend_up_line;
	bool find_up = false;
	Point down, up;
	down.x = IMGW / 2; down.y = IMGH - 1;
	while (!find_up)
	{
		uchar* r_up = store.image_mat.ptr<uchar>(r_up_y);
		r_up_y++;
		for (i = 1; i < IMGW; i++)
		{
			if (r_up[i] != 0 && r_up[i - 1] == 0)
			{
				int k = 0;
				while (r_up[i + k] != 0)k++;
				if (k <= 10)continue;
				else if (k < 80) find_up = true;
				break;
			}
		}
		if (IMGH - 1 - r_up_y < 40)find_up = true;
	}
	uchar* r_up = store.image_mat.ptr<uchar>(r_up_y - 10);
	for (i = IMGW / 2; i > 0; i--)
	{
		if (r_up[i + 1] != 0 && r_up[i] != 0 && r_up[i - 1] == 0)left = i;
	}
	for (i = IMGW / 2; i < IMGW - 1; i++)
	{
		if (r_up[i - 1] != 0 && r_up[i] != 0 && r_up[i + 1] == 0)right = i;
	}
	center = (left + right) / 2;
	up.x = center; up.y = r_up_y - 10;
	float x1 = down.x;
	float y1 = down.y;
	float x2 = up.x;
	float y2 = up.y;
	for (i = down.y; i > up.y; i--)
	{
		int m = x1 + (i - down.y) * (x2 - x1) / (y2 - y1);
		center_point[i] = m;
		exist_left_edge_point[i] = true;
		exist_right_edge_point[i] = true;
	}
}



void MainImage::find_edge_point()
{
	Mat mat = store.image_mat;
	int i, j, center = last_center;
	int last_right = IMGW - 1;
	int last_left = 0;
	int count = 0;
	int m = 1;
	if(state_out==bomb_find&&state_cone==right_block){
		i, j, center = IMGW/10;
		last_right = IMGW/2;
		
	}else if(state_out==bomb_find&&state_cone==left_block){
		i, j, center = IMGW/10*9;
		last_left = IMGW/2;		
	}

	uchar* r = mat.ptr<uchar>(IMGH - 1);           //指向最下面一行
	if(state_out==bomb_find&&state_cone==right_block){
		for (j = IMGW/10 + 1; j < IMGW; j++) {                 //中线向右找寻右边界
			if (j == IMGW - 1) {
				last_right = IMGW - 1;
			}
			if (r[j - 1] != 0 && r[j] == 0 && r[j + 1] == 0) {
				last_right = j;
				break;
			}
		}
		for (j = IMGW/10 - 1; j > -1; j--) {                   //中线向左找寻左边界
			if (j == 0) {
				last_left = 0;
			}
			if (r[j + 1] != 0 && r[j] == 0 && r[j - 1] == 0) {
				last_left = j;
				break;
			}
		}
	}else if(state_out==bomb_find&&state_cone==left_block){
		for (j = IMGW/10*9 + 1; j < IMGW; j++) {                 //中线向右找寻右边界
			if (j == IMGW - 1) {
				last_right = IMGW - 1;
			}
			if (r[j - 1] != 0 && r[j] == 0 && r[j + 1] == 0) {
				last_right = j;
				break;
			}
		}
		for (j = IMGW/10*9 - 1; j > -1; j--) {                   //中线向左找寻左边界
			if (j == 0) {
				last_left = 0;
			}
			if (r[j + 1] != 0 && r[j] == 0 && r[j - 1] == 0) {
				last_left = j;
				break;
			}
		}	
	}else{
		for (j = last_center + 1; j < IMGW; j++) {                 //中线向右找寻右边界
			if (j == IMGW - 1) {
				last_right = IMGW - 1;
			}
			if (r[j - 1] != 0 && r[j] == 0 && r[j + 1] == 0) {
				last_right = j;
				break;
			}
		}
		for (j = last_center - 1; j > -1; j--) {                   //中线向左找寻左边界
			if (j == 0) {
				last_left = 0;
			}
			if (r[j + 1] != 0 && r[j] == 0 && r[j - 1] == 0) {
				last_left = j;
				break;
			}
		}
	}
	for (i = IMGH - 1; i > center_lost; i--) {                     //从下向上开始巡线
		uchar* this_row = mat.ptr(i);  
		if(state_out==bomb_find&&state_cone==right_block){
			center = IMGW/10;

		}else if(state_out==bomb_find&&state_cone==left_block){
			center = IMGW/10*9;	
		}                //指向第i行
		if (this_row[center] == 0) {
			if (count_white(mat, i) <= 8)  //第i行白像素数量不大于8，则第i行中心位置缺失
			{
				exist_left_edge_point[i] = false;
				exist_right_edge_point[i] = false;
				center_lost = i;
				break;
			}
			while (true)
			{
				if(state_out==bomb_find&&state_cone==right_block){
					center = IMGW/10;

				}else if(state_out==bomb_find&&state_cone==left_block){
					center = IMGW/10*9;	
				}
				//将中心点挪到白色位置
				if (this_row[(center + m < IMGW ? center + m : IMGW - 1)] != 0)
				{
					center = (center + m < IMGW ? center + m : IMGW - 1);
					// for(int k = (center + m < IMGW ? center + m : IMGW - 1); k < IMGW-1; k++){
					// 	if(this_row[k-1]==0&&this_row[k]!=0){
					// 		center = k;
					// 	}
					// }

					m = 1;
					break;
				}
				if (this_row[center - m >= 0 ? center - m : 0] != 0)
				{
					center = center - m >= 0 ? center - m : 0;
					// for(int k = (center - m < 0 ? center - m : 0); k > 0; k--){
					// 	if(this_row[k+1]==0&&this_row[k]!=0){
					// 		center = k;
					// 	}
					// }
					m = 1;
					break;
				}
				m++;
			}
		}
		for (j = center + 1; j < IMGW; j++) {                                    //找寻右边界 
			count++;
			if (j == IMGW - 1) {
				exist_right_edge_point[i] = false;
				break;
			}
			if (this_row[j - 1] != 0 && this_row[j] == 0) {
				if (j < last_right + 2)
				{//j <= last_right + 2&&(abs(j-last_right)<20    abs(j-last_right) <=  5
				    // bool right_edge = true;

				    // for(int edge_refine = j; edge_refine < j + 10; j++){
					// 	if(this_row[edge_refine] != 0){
					// 		right_edge = false;
					// 		cerr<<"here"<<endl;
					// 		break;
					// 	}
					// }
					// if(right_edge){
					// Point p;
					// p.x = center + 1;
					// p.y = i;
					// right_cone_point.push_back(p);
					right_edge_point[i] = j;
					// if(state_cone==right_block)cerr<<center<<endl;
					exist_right_edge_point[i] = true;
					last_right = j;
					// }
					// else {
				// 	// exist_left_edge_point[i] = false;
				// }
				}
				else {
					exist_right_edge_point[i] = false;
				}
				break;
			}
		}
		for (j = center - 1; j > -1; j--) {                                            //找寻左边界
			count++;
			if (j == 0) {
				exist_left_edge_point[i] = false;
				break;
			}
			if (this_row[j + 1] != 0 && this_row[j] == 0) {
				if (j > last_left - 2)
				{  //j >= last_left - 2abs(j - last_left) <= 5
				    // bool left_edge = true;

				    // for(int edge_refine = j; edge_refine > j - 10; j--){
					// 	if(this_row[edge_refine] != 0){
					// 		left_edge = false;
					// 		break;
					// 	}
					// }
					// if(left_edge){
					left_edge_point[i] = j;
					exist_left_edge_point[i] = true;
					last_left = j;
				// 	}
				// 	else {
				// 	exist_left_edge_point[i] = false;
				// }
				}
				else {
					exist_left_edge_point[i] = false;
				}
				break;
			}

		}

		
		if (last_left > last_right) {
			center_lost = i;
			exist_left_edge_point[i] = false;
			exist_right_edge_point[i] = false;
			break;
		}
		if (count < 3 * MINX) {    //3 * MINX
			center_lost = i;
			exist_left_edge_point[i] = false;
			exist_right_edge_point[i] = false;
			break;
		}
		if ((exist_left_edge_point[i] && left_edge_point[i] > IMGW - 8) || (exist_right_edge_point[i] && right_edge_point[i] < 8)) {
			center_lost = i;
			exist_left_edge_point[i] = false;
			exist_right_edge_point[i] = false;
			break;
		}
		if (exist_left_edge_point[i]) {
			if (count_white(mat, i,
				(left_edge_point[i] + 5 < IMGW ? left_edge_point[i] + 5 : IMGW), (left_edge_point[i] - 4 > -1 ? left_edge_point[i] - 4 : 0)) == 0) {
				center_lost = i;
				exist_left_edge_point[i] = false;
				exist_right_edge_point[i] = false;
				break;
			}
		}
		if (exist_right_edge_point[i]) {
			if (count_white(mat, i,
				(right_edge_point[i] + 5 < IMGW ? right_edge_point[i] + 5 : IMGW), (right_edge_point[i] - 4 > -1 ? right_edge_point[i] - 4 : 0)) == 0) {
				center_lost = i;
				exist_left_edge_point[i] = false;
				exist_right_edge_point[i] = false;
				break;
			}
		}
		// if(exist_left_edge_point[i] && exist_right_edge_point[i]&& i < IMGH-3){
		// 	int count_zebra = 0;
		// 	for(int zebra_judge = left_edge_point[i]; zebra_judge < right_edge_point[i]; zebra_judge++){
		// 		uchar* zebra_row = mat.ptr(i-5);
		// 		if(zebra_row[zebra_judge]!=zebra_row[zebra_judge+1]){
		// 			count_zebra++;
		// 		}
		// 	}
		// 	if(count_zebra > 8){
		// 		for(int lost = i; lost > i - 5; lost--){
		// 			exist_left_edge_point[lost] = false;
		// 			exist_right_edge_point[lost] = false;	
		// 		center_lost = i - 4;				
		// 		}
		// 	}
		// }
		count = 0;
		center = (last_left + last_right) / 2;
		center_point[i] = center;
	}
	if (exist_left_edge_point[IMGH - MINY] && exist_right_edge_point[IMGH - MINY])
		last_center = (left_edge_point[IMGH - MINY] + right_edge_point[IMGH - MINY]) >> 1;
	else if (exist_left_edge_point[IMGH - MINY])
		last_center = (left_edge_point[IMGH - MINY] + IMGW - 1) >> 1;
	else if (exist_right_edge_point[IMGH - MINY])
		last_center = right_edge_point[IMGH - MINY] >> 1;
	for (i = 0; i < 3; i++)
	{
		exist_left_edge_point[i] = false;
		exist_right_edge_point[i] = false;
	}
	exist_left_edge_point[IMGH - 1] = false;
	exist_right_edge_point[IMGH - 1] = false;
	exist_left_edge_point[IMGH - 2] = false;
	exist_right_edge_point[IMGH - 2] = false;
}

void MainImage::refind_edge_point()
{
	Mat mat = store.image_mat;
	int i, j, center = IMGW / 2;
	int last_right = IMGW - 1;
	int last_left = 0;
	int count = 0;
	int m = 1;
	for (i = IMGH - 1; i > 0; i--) {
		exist_left_edge_point[i] = false;
		exist_right_edge_point[i] = false;
	}
	for (i = IMGH - 1; i > center_lost; i--)
	{                     //从下向上开始巡线
		uchar* this_row = mat.ptr(i);                  //指向第i行
		if (this_row[center] == 0) {
			if (count_white(store.image_mat, i) < 5) {
				exist_left_edge_point[i] = false;
				exist_right_edge_point[i] = false;
				break;
			}
			while (true) {
				if (this_row[(center + m < IMGW ? center + m : IMGW - 1)] != 0) {
					center = (center + m < IMGW ? center + m : IMGW - 1);
					m = 1;
					break;
				}
				if (this_row[center - m >= 0 ? center - m : 0] != 0) {
					center = center - m >= 0 ? center - m : 0;
					m = 1;
					break;
				}
				m++;
			}
		}
		if (this_row[center] != 0)
		{
			for (j = center + 1; j < IMGW - 1; j++) {                                    //找寻右边界 
				count++;
				if (this_row[j - 1] != 0 && this_row[j] == 0) {
					right_edge_point[i] = j;
					exist_right_edge_point[i] = true;
					last_right = j;
					break;
				}
				if (j == IMGW - 1) {
					exist_right_edge_point[i] = false;
				}
			}
			for (j = center - 1; j > 0; j--) {                                            //找寻左边界
				count++;
				if (this_row[j + 1] != 0 && this_row[j] == 0) {
					left_edge_point[i] = j;
					exist_left_edge_point[i] = true;
					last_left = j;
					break;
				}
				if (j == 1)
				{
					exist_left_edge_point[i] = false;
				}
			}
		}
		center = (last_left + last_right) / 2;
		if (last_left > last_right) {
			center_lost = i;
			break;
		}
		if (count < 3 * MINX) {    //3 * MINX
			center_lost = i;
			break;
		}
		if ((exist_left_edge_point[i] && left_edge_point[i] > IMGW - 8) || (exist_right_edge_point[i] && right_edge_point[i] < 8)) {
			center_lost = i;
			break;
		}
		if (exist_left_edge_point[i]) {
			if (count_white(mat, i,
				(left_edge_point[i] + 5 < IMGW ? left_edge_point[i] + 5 : IMGW), (left_edge_point[i] - 4 > -1 ? left_edge_point[i] - 4 : 0)) == 0) {
				center_lost = i;
				break;
			}
		}
		if (exist_right_edge_point[i]) {
			if (count_white(mat, i,
				(right_edge_point[i] + 5 < IMGW ? right_edge_point[i] + 5 : IMGW), (right_edge_point[i] - 4 > -1 ? right_edge_point[i] - 4 : 0)) == 0) {
				center_lost = i;
				break;
			}
		}
		count = 0;
	}
}

float MainImage::calc_curvature(CurvaturePoint p1, CurvaturePoint p2, CurvaturePoint p3)
{

	// cout << "x1: " << p1.x << "  y1: " << p1.y << endl;
	// cout << "x2: " << p2.x << "  y2: " << p2.y << endl;
	// cout << "x3: " << p3.x << "  y3: " << p3.y << endl;

 	CurvaturePoint v1 = {p2.x - p1.x, p2.y - p1.y};
    CurvaturePoint v2 = {p3.x - p2.x, p3.y - p2.y};

    // 计算叉积
    CurvaturePoint cross = {v1.x * v2.y - v1.y * v2.x};

    // 计算叉积的模长
    float cross_length = std::sqrt(cross.x * cross.x + cross.y * cross.y);

    // 计算点积
    float dot = v1.x * v2.x + v1.y * v2.y ;

    // 计算曲率
    float curvature = cross_length / (dot + 1e-10);  // 添加一个小的常数以防止除以零	
	return curvature;
}

float MainImage::get_curvature_far(void)
{
	CurvaturePoint p1, p2, p3;
	CurvaturePoint x1, x2;
	slope_thresh_far = IMGH - re.main.slope_forward_dist_far;
	if(center_lost > (IMGH - 2 - re.main.curvature_up_scope - re.main.curvature_down_scope))
	{
		return curvature_far;
	}
	while((slope_thresh_far - re.main.curvature_up_scope) < center_lost)
	{
		slope_thresh_far++;
	}
	p1.y = slope_thresh_far - re.main.curvature_up_scope;
	// p3.y = slope_thresh_far + re.main.curvature_down_scope;

	x1.y = slope_thresh_far - re.main.curvature_up_scope;
	// p2.y = slope_thresh_far;
	x2.y = slope_thresh_far + re.main.curvature_down_scope;
	while(center_point[x1.y]<4 || center_point[x2.y]>IMGW-5 || x1.y < center_lost)
	{
		x1.y++;
		x2.y++;
	}
	//左转
	if(((center_point[x1.y]-center_point[x2.y])<0 ||lost_left) && !lost_right)
	{
		while(!exist_left_edge_point[p1.y])
		{
			p1.y++;
		}
		p2.y = p1.y + re.main.curvature_up_scope;
		if(p2.y > IMGH -1)
		{
			return curvature_far;
		}
		while(!exist_left_edge_point[p2.y] || p2.y<=p1.y)
		{
			p2.y++;
		}
		p3.y = p2.y + re.main.curvature_down_scope;
		if(p3.y > IMGH -1)
		{
			return curvature_far;
		}
		while(!exist_left_edge_point[p3.y] || p3.y<=p2.y)
		{
			p3.y++;
		}
		if(p3.y > IMGH -1)
		{
			return curvature_far;
		}
		p1.x = left_edge_point[p1.y];
		p2.x = left_edge_point[p2.y];
		p3.x = left_edge_point[p3.y];
		curvature_far = calc_curvature(p1,p2,p3);
	}
//右转
	else if(((center_point[x1.y]-center_point[x2.y])>=0 ||lost_right) && !lost_left)
	{
		while(!exist_right_edge_point[p1.y])
		{
			p1.y++;
		}
		p2.y = p1.y + re.main.curvature_up_scope;
		if(p2.y > IMGH -1)
		{
			return slope;
		}
		while(!exist_right_edge_point[p2.y] || p2.y<=p1.y)
		{
			p2.y++;
		}
		p3.y = p2.y + re.main.curvature_down_scope;
		if(p3.y > IMGH -1)
		{
			return slope;
		}
		while(!exist_right_edge_point[p3.y] || p3.y<=p2.y)
		{
			p3.y++;
		}
		if(p3.y > IMGH -1)
		{
			return slope;
		}
		p1.x = right_edge_point[p1.y];
		p2.x = right_edge_point[p2.y];
		p3.x = right_edge_point[p3.y];
		curvature_far = calc_curvature(p1,p2,p3);
	}
	cout << "curvature_far: " << curvature_far << endl;
	if(smoothed_curvature_far == 0)
	{
		smoothed_curvature_far = curvature_far;
	}
	else
	{
		smoothed_curvature_far = curvature_far * 0.7 + smoothed_curvature_far * 0.3;
	}
	cout << "smoothed_curvature_far: " << smoothed_curvature_far << endl;
	return curvature_far;
}

float MainImage::get_curvature_near(void)
{
	CurvaturePoint p1, p2, p3;
	CurvaturePoint x1, x2;
	slope_thresh_near = IMGH - re.main.slope_forward_dist_near;
	slop_direction_thresh = IMGH - re.main.slope_direction_forward_dist;
	if(center_lost > (IMGH - 2 - re.main.curvature_up_scope - re.main.curvature_down_scope))
	{
		return curvature_near;
	}
	while((slope_thresh_near - re.main.curvature_up_scope) < center_lost)
	{
		slope_thresh_near++;
	}
	p1.y = slope_thresh_near - re.main.curvature_up_scope;
	// p3.y = slope_thresh_near + re.main.curvature_down_scope;

	x1.y = slop_direction_thresh - re.main.curvature_up_scope;
	// p2.y = slope_thresh_near;
	x2.y = slop_direction_thresh + re.main.curvature_down_scope;
	while(center_point[x1.y]<4 || center_point[x2.y]>IMGW-5 || x1.y < center_lost)
	{
		x1.y++;
		x2.y++;
	}
	//左转
	if(((center_point[x1.y]-center_point[x2.y])<0 ||lost_left) && !lost_right)
	{
		while(!exist_left_edge_point[p1.y])
		{
			p1.y++;
		}
		p2.y = p1.y + re.main.curvature_up_scope;
		if(p2.y > IMGH -1)
		{
			return curvature_near;
		}
		while(!exist_left_edge_point[p2.y] || p2.y<=p1.y)
		{
			p2.y++;
		}
		p3.y = p2.y + re.main.curvature_down_scope;
		if(p3.y > IMGH -1)
		{
			return curvature_near;
		}
		while(!exist_left_edge_point[p3.y] || p3.y<=p2.y)
		{
			p3.y++;
		}
		if(p3.y > IMGH -1)
		{
			return curvature_near;
		}
		p1.x = left_edge_point[p1.y];
		p2.x = left_edge_point[p2.y];
		p3.x = left_edge_point[p3.y];
		curvature_near = calc_curvature(p1,p2,p3);
	}
//右转
	else if(((center_point[x1.y]-center_point[x2.y])>=0 ||lost_right) && !lost_left)
	{
		while(!exist_right_edge_point[p1.y])
		{
			p1.y++;
		}
		p2.y = p1.y + re.main.curvature_up_scope;
		if(p2.y > IMGH -1)
		{
			return slope;
		}
		while(!exist_right_edge_point[p2.y] || p2.y<=p1.y)
		{
			p2.y++;
		}
		p3.y = p2.y + re.main.curvature_down_scope;
		if(p3.y > IMGH -1)
		{
			return slope;
		}
		while(!exist_right_edge_point[p3.y] || p3.y<=p2.y)
		{
			p3.y++;
		}
		if(p3.y > IMGH -1)
		{
			return slope;
		}
		p1.x = right_edge_point[p1.y];
		p2.x = right_edge_point[p2.y];
		p3.x = right_edge_point[p3.y];
		curvature_near = calc_curvature(p1,p2,p3);
	}
	cout << "curvature_near: " << curvature_near << endl;
	if(smoothed_curvature_near == 0)
	{
		smoothed_curvature_near = curvature_near;
	}
	else
	{
		smoothed_curvature_near = curvature_near * 0.7 + smoothed_curvature_near * 0.3;
	}
	cout << "smoothed_curvature_near: " << smoothed_curvature_near << endl;
	return curvature_near;
}

float MainImage::get_slope_near(void)
{
	CurvaturePoint p1, p2, p3;
	CurvaturePoint x1, x2;
	slope_thresh_near = IMGH - re.main.slope_forward_dist_near;	
	if(center_lost > (IMGH - 2 - re.main.curvature_up_scope - re.main.curvature_down_scope))
	{
		return slope;
	}
	while((slope_thresh_near - re.main.curvature_up_scope) < center_lost)
	{
		slope_thresh_near++;
	}
	
	p1.y = slope_thresh_near - re.main.curvature_up_scope;
	// p2.y = slope_thresh_near;
	// p3.y = slope_thresh_near + re.main.curvature_down_scope;
	// while(center_point[p1.y]<4||center_point[p1.y]>IMGW-5)
	// {
	// 	p1.y++;
	// 	p3.y++;
	// }
	x1.y = slop_direction_thresh - re.main.curvature_up_scope;
	// p2.y = slope_thresh_near;
	x2.y = slop_direction_thresh + re.main.curvature_down_scope;
	while(center_point[x1.y]<4 || center_point[x2.y]>IMGW-5 || x1.y < center_lost)
	{
		x1.y++;
		x2.y++;
	}
	//左转
	if(((center_point[x1.y]-center_point[x2.y])<0 ||lost_left) && !lost_right)
	{
		while(!exist_left_edge_point[p1.y])
		{
			p1.y++;
		}
		p2.y = p1.y + re.main.curvature_up_scope;
		if(p2.y > IMGH -1)
		{
			return slope;
		}
		while(!exist_left_edge_point[p2.y] || p2.y<=p1.y)
		{
			p2.y++;
		}
		p3.y = p2.y + re.main.curvature_down_scope;
		if(p3.y > IMGH -1)
		{
			return slope;
		}
		while(!exist_left_edge_point[p3.y] || p3.y<=p2.y)
		{
			p3.y++;
		}
		if(p3.y > IMGH -1)
		{
			return slope;
		}
		p1.x = left_edge_point[p1.y];
		p2.x = left_edge_point[p2.y];
		p3.x = left_edge_point[p3.y];
		slope = ((float)(p1.x-p2.x) / (p1.y-p2.y) + (float)(p2.x-p3.x) / (p2.y-p3.y)) / 2.0;
	}
	//右转
	else if(((center_point[x1.y]-center_point[x2.y])>=0 ||lost_right) && !lost_left)
	{
		while(!exist_right_edge_point[p1.y])
		{
			p1.y++;
		}
		p2.y = p1.y + re.main.curvature_up_scope;
		if(p2.y > IMGH -1)
		{
			return slope;
		}
		while(!exist_right_edge_point[p2.y] || p2.y<=p1.y)
		{
			p2.y++;
		}
		p3.y = p2.y + re.main.curvature_down_scope;
		if(p3.y > IMGH -1)
		{
			return slope;
		}
		while(!exist_right_edge_point[p3.y] || p3.y<=p2.y)
		{
			p3.y++;
		}
		if(p3.y > IMGH -1)
		{
			return slope;
		}
		p1.x = right_edge_point[p1.y];
		p2.x = right_edge_point[p2.y];
		p3.x = right_edge_point[p3.y];
		slope = ((float)(p1.x-p2.x) / (p1.y-p2.y) + (float)(p2.x-p3.x) / (p2.y-p3.y)) / 2.0;
		// slope = -sqrt(abs((float)((p1.x-p2.x)*(p2.x-p3.x))/((p1.y-p2.y)*(p2.y-p3.y))));
	}

	cout << "x1: " << p1.x << "  y1: " << p1.y << endl;
	cout << "x2: " << p2.x << "  y2: " << p2.y << endl;
	cout << "x3: " << p3.x << "  y3: " << p3.y << endl;
	cout << "slope: " << slope << endl;
	return slope;
}

void MainImage::find_center()
{
	for (int i = IMGH - 1; i >= 0; i--) {
		if (i < center_lost)break;
		if (lost_left || lost_right) {
			if (exist_left_edge_point[i] && exist_right_edge_point[i])
				center_point[i] = (left_edge_point[i] + right_edge_point[i]) >> 1;
			else if (exist_right_edge_point[i]) {
				int j;
				for (j = i; j >= 0; j--) {
					if (!exist_right_edge_point[j] || exist_left_edge_point[j]) break;
				}
				int mov, devide;
				float wid;
				if (i > IMGH - 5 && right_edge_point[i] < 140) {
					wid = 125;//(2 * right_edge_point[i] - 100)/2;110
				}
				else wid = right_edge_point[i];

				for (int k = i; k > j; k--) {
					wid -= 0.28f;
					//devide = right_edge_point[k] / 2;
					mov = (2 * right_edge_point[k] - round(wid)) / 2;
					center_point[k] = ((2 * mov) / 2 > -1 ? (2 * mov) / 2 : 0);
				}
				i = j + 1;
			}
			else if (exist_left_edge_point[i]) {
				int j;
				for (j = i; j >= 0; j--) {
					if (!exist_left_edge_point[j] || exist_right_edge_point[j]) break;
				}
				int mov, devide;
				float wid;
				if (i > IMGH - 5 && IMGW - left_edge_point[i] < 140) {
					wid = 125;//IMGW - left_edge_point[i];110
				}
				else wid = IMGW - left_edge_point[i];
				for (int k = i; k > j; k--) {
					wid -= 0.28;//0.43
					//devide = (left_edge_point[k] + IMGW) / 2;
					mov = (2 * left_edge_point[k] + round(wid)) / 2;
					center_point[k] = ((2 * mov) / 2 < IMGW ? (2 * mov) / 2 : IMGW - 1);
				}
				i = j + 1;
			}
			else center_point[i] = IMGW / 2;
		}
		else {
			if (exist_left_edge_point[i] && exist_right_edge_point[i])
				center_point[i] = (left_edge_point[i] + right_edge_point[i]) / 2;
			else if (exist_right_edge_point[i]) {
				center_point[i] = right_edge_point[i] / 2;
			}
			else if (exist_left_edge_point[i]) {
				center_point[i] = (left_edge_point[i] + IMGW - 1) / 2;
			}
			else center_point[i] = IMGW / 2;
		    /*else if(exist_right_edge_point[i]) {
				center_point[i] =((2*center_point[i-1]-center_point[i-2])+right_edge_point[i] / 2)/2;
			}
			else if (exist_left_edge_point[i]) {
				center_point[i] =(2*center_point[i-1]-center_point[i-2]+_(left_edge_point[i] + IMGW - 1) / 2)/2;
			}
			else center_point[i] = (center_point[i-1]+IMGW / 2)/2;
			*/
		}
	}


	if (center_lost != -1) {
		if (center_lost < IMGH - 1) {
			for (int i = center_lost; i > -1; i--) {
				center_point[i] = center_point[center_lost + 1];
			}
		}
		else {
			for (int i = center_lost; i > -1; i--) {
				center_point[i] = IMGW / 2;
			}
		}
	}
	for(int i=0;i<IMGH;i++)
	{
		store.global_center[i]=center_point[i];
	}



}

void MainImage::find_end_point() {
	Mat mat = store.image_mat;
	Point p;
	int i;
	left_end_point.clear();
	right_end_point.clear();
	for (i = IMGH - 2; i > 0; i--) {
		if (count_white(mat, i) < 1) break;
		if (exist_left_edge_point[i]) {
			if ((!exist_left_edge_point[i - 1] && exist_left_edge_point[i + 1]) ||
				(exist_left_edge_point[i - 1] && !exist_left_edge_point[i + 1])) {
				p.y = i;
				p.x = left_edge_point[i];
				left_end_point.push_back(p);
			}
		}

		if (exist_right_edge_point[i]) {

			if ((!exist_right_edge_point[i - 1] && exist_right_edge_point[i + 1]) ||
				(exist_right_edge_point[i - 1] && !exist_right_edge_point[i + 1])) {
				p.y = i;
				p.x = right_edge_point[i];
				right_end_point.push_back(p);
			}
		}
	}
}

void MainImage::judge_lost()
{
	if (left_end_point.size() != 0 && (left_end_point[0].y > IMGH - 10 || left_end_point[0].x < 8) && left_end_point[1].y > 20 && left_end_point[1].y < IMGH - 20 && left_end_point[1].x > IMGW / 2 + 5) {
		if ((right_end_point.size() == 0) ||
			(right_end_point.size() != 0 && right_end_point[0].y < IMGH - 50) ||
			(right_end_point.size() != 0 && right_end_point[0].y > IMGH - 20 && right_end_point[1].y > IMGH - 40 && right_end_point[0].x > IMGW - 20)) {
			lost_right = true;
		}
	}
	if (right_end_point.size() != 0 && (right_end_point[0].y > IMGH - 10 || right_end_point[0].x > IMGW - 8) && right_end_point[1].y > 20 && right_end_point[1].y < IMGH - 20 && right_end_point[1].x < IMGW / 2 - 5) {
		if (left_end_point.size() == 0 ||
			(left_end_point.size() != 0 && left_end_point[0].y < IMGH - 50) ||
			(left_end_point.size() != 0 && left_end_point[0].y > IMGH - 20 && left_end_point[1].y > IMGH - 40 && left_end_point[0].x < 20)) {
			lost_left = true;
		}
	}
}

void MainImage::count_branch()
{
	Mat mat = store.image_mat;
	int i, j, k;
	int count = 0, white = 0;
	if (left_end_point.size() > 2) {
		for (i = 1; i < left_end_point.size() - 1; i += 2) {
			if (abs(left_end_point[i].y - left_end_point[i + 1].y) > 5 && left_end_point[i].x > 3) {
				for (j = left_end_point[i].y - 1; j > left_end_point[i + 1].y; j--) {
					uchar* r = mat.ptr<uchar>(j);
					for (k = left_end_point[i].x; k > -1; k--) {
						if (r[k] != 0)white++;
						else break;
					}
					if (white > 4)count++;
					white = 0;
				}
				if (count >= abs(left_end_point[i].y - left_end_point[i + 1].y) - 3)left_branch_num++;
				count = 0;
			}
		}
	}
	if (right_end_point.size() > 2) {
		for (i = 1; i < right_end_point.size() - 1; i += 2) {
			if (abs(right_end_point[i].y - right_end_point[i + 1].y) > 5 && right_end_point[i].x < IMGW - 3) {
				for (j = right_end_point[i].y - 1; j > right_end_point[i + 1].y; j--) {
					uchar* r = mat.ptr<uchar>(j);
					for (k = right_end_point[i].x; k < IMGW; k++) {
						if (r[k] != 0)white++;
						else break;
					}
					if (white > 4)count++;
					white = 0;
				}
				if (count >= abs(right_end_point[i].y - right_end_point[i + 1].y) - 3)right_branch_num++;
				count = 0;
			}
		}
	}
}

void MainImage::judge_continuity()
{
	int i, l_long = 0, r_long = 0;
	for (i = 0; i < left_end_point.size(); i += 2) {
		if (abs(left_end_point[i].y - left_end_point[i + 1].y) > l_long)
			l_long = abs(left_end_point[i].y - left_end_point[i + 1].y);
	}
	for (i = 0; i < right_end_point.size(); i += 2) {
		if (abs(right_end_point[i].y - right_end_point[i + 1].y) > r_long)
			r_long = abs(right_end_point[i].y - right_end_point[i + 1].y);
	}
	if ((left_end_point.size() > 2 && l_long > 0.6 * IMGH) || (left_end_point.size() == 2 && abs(left_end_point[0].y - left_end_point[1].y) > 0.3 * IMGH))
		left_continue = true;
	else left_continue = false;
	if ((right_end_point.size() > 2 && r_long > 0.6 * IMGH) || (right_end_point.size() == 2 && abs(right_end_point[0].y - right_end_point[1].y) > 0.3 * IMGH))
		right_continue = true;
	else right_continue = false;
}

void MainImage::edge_filter(int wid, int side)
{
	int i;
	for (i = IMGH - 2; i > 0; i--) {
		if (side == 0 || side == 2)
		{
			if (exist_left_edge_point[i] && exist_left_edge_point[i + 1]) {
				if (abs(left_edge_point[i] - left_edge_point[i + 1]) >= wid) {
					exist_left_edge_point[i + 1] = false;
				}
			}
		}
		if (side == 1 || side == 2)
			if (exist_right_edge_point[i] && exist_right_edge_point[i + 1]) {
				if (abs(right_edge_point[i] - right_edge_point[i + 1]) >= wid) {
					exist_right_edge_point[i + 1] = false;
				}
			}
	}
}

void MainImage::end_filter(int side) {
	int i, j;
	if (side == 0 || side == 2)
	{
		if (left_end_point.size() > 0) {
			for (i = 0; i < left_end_point.size(); i += 2) {
				if (abs(left_end_point[i + 1].y - left_end_point[i].y) < 3) {
					for (j = left_end_point[i + 1].y; j <= left_end_point[i].y; j++) {
						exist_left_edge_point[j] = false;
					}
					left_end_point.erase(left_end_point.begin() + i, left_end_point.begin() + i + 2);
				}
			}
		}
	}
	if (side == 1 || side == 2)
	{
		if (right_end_point.size() > 0) {
			for (i = 0; i < right_end_point.size(); i += 2) {
				if (abs(right_end_point[i + 1].y - right_end_point[i].y) < 3) {
					for (j = right_end_point[i + 1].y; j <= right_end_point[i].y; j++) {
						exist_right_edge_point[j] = false;
					}
					right_end_point.erase(right_end_point.begin() + i, right_end_point.begin() + i + 2);
				}
			}
		}
	}

}
/**
 * @brief 保存图像
*/

void MainImage::show(float dev, float angle_result, float speed, int current_speed, bool c, bool l, bool r, bool l_e, bool r_e, bool l_c, bool r_c, bool c_c)
{
    Mat channels[3];
    store.image_mat += 128;
    store.image_mat.copyTo(channels[0]);
    store.image_mat.copyTo(channels[1]);
    store.image_mat.copyTo(channels[2]);

    // 在图像上添加边缘、端点和圆锥标记
    merge(channels, 3, store.image_show);

    // 在图像上添加边缘、端点和圆锥标记
    for (int i = 0; i < IMGH; i++) {
        if (exist_left_edge_point[i] || exist_right_edge_point[i])
        {
            if (exist_left_edge_point[i]) store.image_show.at<Vec3b>(i, left_edge_point[i]) = Vec3b(255, 0, 0);
            if (exist_right_edge_point[i]) store.image_show.at<Vec3b>(i, right_edge_point[i]) = Vec3b(0, 255, 0);
            
        }
    }


	for (int i = 0; i < left_cone_point.size(); i++){
		store.image_show.at<Vec3b>(left_cone_point[i].y, left_cone_point[i].x) = Vec3b(0, 0, 0);
	}
	for (int i = 0; i < right_cone_point.size(); i++){
		store.image_show.at<Vec3b>(right_cone_point[i].y, right_cone_point[i].x) = Vec3b(0, 0, 0);
	}


	for(int i = IMGH-1; i > center_lost; i--){
		store.image_show.at<Vec3b>(i, center_point[i]) = Vec3b(0, 255, 255);
	}
    if (l_e) for (int i = 0; i < left_end_point.size(); i++) store.image_show.at<Vec3b>(left_end_point[i].y, left_end_point[i].x) = Vec3b(0, 0, 255);
    if (r_e) for (int i = 0; i < right_end_point.size(); i++) store.image_show.at<Vec3b>(right_end_point[i].y, right_end_point[i].x) = Vec3b(255, 0, 255);
    if (l_c) for (int i = 0; i < left_cone.size(); i++) store.image_show.at<Vec3b>(left_cone[i].y, left_cone[i].x) = Vec3b(19, 78, 39);
    if (r_c) for (int i = 0; i < right_cone.size(); i++) store.image_show.at<Vec3b>(right_cone[i].y, right_cone[i].x) = Vec3b(121, 77, 166);
    if (c_c) for (int i = 0; i < center_cone.size(); i++) store.image_show.at<Vec3b>(center_cone[i].y, center_cone[i].x) = Vec3b(0, 153, 255);

	for(int i = 0; i < cone_number.size();i++){
		store.image_show.at<Vec3b>(cone_number[i].y, cone_number[i].x) = Vec3b(19, 78, 39);
	}
	store.image_show.at<Vec3b>(MainImage::deviation_thresh, 80) = Vec3b(60, 90, 30);
	store.image_show.at<Vec3b>((MainImage::deviation_thresh - re.main.up_scope), 80) = Vec3b(60, 90, 30);
	store.image_show.at<Vec3b>((MainImage::deviation_thresh + re.main.down_scope),80) = Vec3b(60, 90, 30);

	store.image_show.at<Vec3b>(MainImage::speed_deviation_thresh_near, 80) = Vec3b(74, 23, 145);
	store.image_show.at<Vec3b>((MainImage::speed_deviation_thresh_near - re.main.up_scope), 80) = Vec3b(74, 23, 145);
	store.image_show.at<Vec3b>((MainImage::speed_deviation_thresh_near + re.main.down_scope),80) = Vec3b(74, 23, 145);

	store.image_show.at<Vec3b>(MainImage::speed_deviation_thresh_far, 80) = Vec3b(77, 80, 44);
	store.image_show.at<Vec3b>((MainImage::speed_deviation_thresh_far - re.main.up_scope), 80) = Vec3b(77, 80, 44);
	store.image_show.at<Vec3b>((MainImage::speed_deviation_thresh_far + re.main.down_scope),80) = Vec3b(77, 80, 44);

	store.image_show.at<Vec3b>((MI.center_lost),80) = Vec3b(127, 127, 255);
	store.image_show.at<Vec3b>((slope_thresh_near),80) = Vec3b(255, 0, 255);
	store.image_show.at<Vec3b>(top_cone.y, top_cone.x) = Vec3b(0, 255, 0);
	

    // 在图像顶部添加变量文本
    int fontFace = FONT_HERSHEY_SIMPLEX;
    double fontScale = 0.25;
    int thickness = 1;
    int baseline = 0;
	int interval = 5;//行间距
    Size textSize = getTextSize("Text", fontFace, fontScale, thickness, &baseline);
    baseline += thickness;

    // 文本位置的起始点
    Point textOrg(2, textSize.height + 5);
	// 主文本颜色
	cv::Scalar mainColor = Scalar(60,20,220,0.75);
    // 绘制每个变量

    putText(store.image_show, "state: " + 
		string(((state_out == straight) ? "straight" :
				(state_out == right_circle) ? "right_circle" :
				(state_out == left_circle) ? "left_circle" :
				(state_out == turn_state) ? "turn" :
				(state_out == start_state) ? "start_state" :
				(state_out == bomb_find) ? "bomb_find" :
	 			"other")),
				textOrg, fontFace, fontScale, mainColor, thickness, 8);
	textOrg.y += textSize.height + interval;

	if(state_out == right_circle || state_out == left_circle)
	{
		putText(store.image_show, "circle state: " + 
		string(((MI.state_r_circle == right_circle_in_find) ? "right_circle_in_find" :
				(MI.state_r_circle == right_circle_in_strai) ? "right_circle_in_strai" :
				(MI.state_r_circle == right_circle_in_circle) ? "right_circle_in_circle" :
				(MI.state_r_circle == right_circle_inside_before) ? "right_circle_inside_before" :
				(MI.state_r_circle == right_circle_inside) ? "right_circle_inside" :
				(MI.state_r_circle == right_circle_out) ? "right_circle_out" :
				(MI.state_r_circle == right_circle_out_find) ? "right_circle_out_find" :
				(MI.state_r_circle == right_circle_out_strai) ? "right_circle_out_strai" :
				(MI.state_r_circle == right_circle_out_out) ? "right_circle_out_out" :
				(MI.state_l_circle == left_circle_in_find) ? "left_circle_in_find" :
				(MI.state_l_circle == left_circle_in_strai) ? "left_circle_in_strai" :
				(MI.state_l_circle == left_circle_in_circle) ? "left_circle_in_circle" :
				(MI.state_l_circle == left_circle_inside_before) ? "left_circle_inside_before" :
				(MI.state_l_circle == left_circle_inside) ? "left_circle_inside" :
				(MI.state_l_circle == left_circle_out) ? "left_circle_out" :
				(MI.state_l_circle == left_circle_out_find) ? "left_circle_out_find" :
				(MI.state_l_circle == left_circle_out_strai) ? "left_circle_out_strai" :
	 			"other")),
				textOrg, fontFace, fontScale, mainColor, thickness, 8);
	textOrg.y += textSize.height + interval;
	}

	if(state_out == turn_state)
	{
		putText(store.image_show, "turn state: " + 
		string(((MI.state_turn_state == turn_inside) ? "turn_inside" :
				(MI.state_turn_state == turn_slow_down) ? "turn_slow_down" :
				(MI.state_turn_state == judge_side) ? "judge_side" :
	 			"other")),
				textOrg, fontFace, fontScale, mainColor, thickness, 8);
		textOrg.y += textSize.height + interval;
	}

	if(state_out == bomb_find)
	{
		putText(store.image_show, "cone state: " + 
		string(((MI.state_turn_state == cone_slowdown) ? "cone_slowdown" :
				(MI.state_turn_state == judge_side) ? "judge_side" :
	 			"other")),
				textOrg, fontFace, fontScale, mainColor, thickness, 8);
		textOrg.y += textSize.height + interval;
	}

    putText(store.image_show, "kp: " + to_string(cur_kp), textOrg, fontFace, fontScale, mainColor, thickness, 8);
    textOrg.y += textSize.height + interval;

    putText(store.image_show, "Angle: " + to_string(angle_result), textOrg, fontFace, fontScale, mainColor, thickness, 8);
    textOrg.y += textSize.height + interval;

    putText(store.image_show, "speed: " + to_string(speed), textOrg, fontFace, fontScale, mainColor, thickness, 8);
    textOrg.y += textSize.height + interval;

    putText(store.image_show, "current_speed: " + to_string(current_speed), textOrg, fontFace, fontScale, mainColor, thickness, 8);
	textOrg.y += textSize.height + interval;

	putText(store.image_show, "angle forward dist: " + to_string(MainImage::angle_new_forward_dist), textOrg, fontFace, fontScale, mainColor, thickness, 8);
	textOrg.y += textSize.height + interval;
	
    putText(store.image_show, "angle deviation: " + to_string(angle_deviation), textOrg, fontFace, fontScale, mainColor, thickness, 8);
	textOrg.y += textSize.height + interval;
	
    putText(store.image_show, "speed deviation far: " + to_string(speed_deviation_far), textOrg, fontFace, fontScale, mainColor, thickness, 8);
	textOrg.y += textSize.height + interval;

    putText(store.image_show, "slope: " + to_string(MainImage::slope), textOrg, fontFace, fontScale, mainColor, thickness, 8);
	textOrg.y += textSize.height + interval;

    putText(store.image_show, "curvature_near: " + to_string(MainImage::curvature_near), textOrg, fontFace, fontScale, mainColor, thickness, 8);
	textOrg.y += textSize.height + interval;
    // putText(store.image_show, "end point size: " + to_string(right_end_point.size()), textOrg, fontFace, fontScale, mainColor, thickness, 8);

    resize(store.image_show, store.image_show, cv::Size(IMGW * 2, IMGH * 2));
    if (re.set.color)store.wri << store.image_BGR;
    else store.wri << store.image_show;
}

void MainImage::count_cone(int begin) {
	left_cone.clear(); right_cone.clear(); center_cone.clear();
	Point p;
	int i;
	int center;
	int l_wid, l_wid_last = 0;
	int r_wid, r_wid_last = 0;
	bool l_rise = true;
	bool r_rise = true;
	center = last_center_in_cone > 0 ? last_center_in_cone : IMGW / 2;
	for (i = begin; i > 10; i--)
	{
		const uchar* r = store.image_mat.ptr<uchar>(i);
		for (int j = center - 1; j > 0; j--)
		{
			if (r[j] != 0 && r[j + 1] == 0)
			{
				int k = j - 1;
				while ((r[k] != 0) && k > 0)k--;
				if (begin < IMGH - 5 && k == 0)break;
				l_wid = j - k;
				if (l_wid < 6 || l_wid>20)break;
				if (l_wid <= l_wid_last && l_rise)
				{
					l_rise = false;
					p.x = (j + k) / 2;
					p.y = i;
					if (left_cone.size() > 0)
					{
						if (left_cone.back().y - p.y > 55 || left_cone.back().y - p.y < i / 5)break;
					}
					else
					{
						if (begin - p.y > 50)break;
					}
					bool h = true;
					for (int m = i; m > i - 3; m--) {
						if (store.image_mat.ptr<uchar>(m)[p.x] == 0)h = false;
					}
					if (!h)break;
					if (p.x < center)left_cone.push_back(p);

				}
				else if (l_wid > l_wid_last)
				{
					l_rise = true;
				}
				l_wid_last = l_wid;
				break;
			}
		}
		for (int j = center + 1; j < IMGW; j++)
		{
			if (r[j] != 0 && r[j - 1] == 0)
			{
				int k = j + 1;
				while ((r[k] != 0) && k < IMGW)k++;
				if (begin < IMGH - 5 && k == IMGW - 1)break;
				r_wid = k - j;
				if (r_wid < 6 || r_wid>20)break;
				if (r_wid <= r_wid_last && r_rise)
				{
					r_rise = false;
					p.x = (j + k) / 2;
					p.y = i;
					if (right_cone.size() > 0)
					{
						if (right_cone.back().y - p.y > 55 || right_cone.back().y - p.y < i / 5)break;
					}
					else
					{
						if (begin - p.y > 50)break;
					}
					bool h = true;
					for (int m = i; m > i - 3; m--) {
						if (store.image_mat.ptr<uchar>(m)[p.x] == 0)h = false;
					}
					if (!h)break;
					if (p.x > center)right_cone.push_back(p);
				}
				else if (r_wid > r_wid_last)
				{
					r_rise = true;
				}
				r_wid_last = r_wid;
				break;
			}
		}
		if (left_cone.size() > 0 && right_cone.size() > 0)center = (left_cone.back().x + right_cone.back().x) / 2;
		else if (left_cone.size() > 0)
		{
			if (begin >= IMGH - 5)center = (left_cone.back().x + IMGW - 1) / 2;
		}
		else if (right_cone.size() > 0)
		{
			if (begin >= IMGH - 5)center = right_cone.back().x / 2;
		}

	}

	Point p0;
	int l_num = left_cone.size();
	int r_num = right_cone.size();
	if (l_num == 0) {
		l_num = 1; p0.x = (IMGH - begin) * 0.75; p0.y = begin; left_cone.push_back(p0);
	}
	if (r_num == 0) {
		r_num = 1; p0.x = IMGW - (IMGH - begin) * 0.75; p0.y = begin; right_cone.push_back(p0);
	}
	int num = l_num > r_num ? l_num : r_num;
	for (int i = 0; i < num; i++)
	{
		if (l_num > i && r_num > i)
		{
			p.x = (left_cone[i].x + right_cone[i].x) / 2;
			p.y = (left_cone[i].y + right_cone[i].y) / 2;
			center_cone.push_back(p);
		}
		else if (l_num > i)
		{
			p.x = (left_cone[i].x + right_cone.back().x) / 2;
			p.y = (left_cone[i].y + right_cone.back().y) / 2;
			center_cone.push_back(p);
		}
		else if (r_num > i)
		{
			p.x = (left_cone.back().x + right_cone[i].x) / 2;
			p.y = (left_cone.back().y + right_cone[i].y) / 2;
			center_cone.push_back(p);
		}
	}
	if (center_cone.size() > 1)last_center_in_cone = center_cone[1].x;
}
void MainImage::refind_edge_point_in_garage(){
	//cerr<<"try to refind edge"<<endl;
	Mat mat = store.image_mat;
	int i, j, center = IMGW / 2;
	int last_right = IMGW - 1;
	int last_left = 0;
	int count = 0;
	int m = 1;
	for (i = IMGH - 1; i > 0; i--) {
		exist_left_edge_point[i] = false;
		exist_right_edge_point[i] = false;
	}
	for (i = IMGH - 1; i > center_lost; i--)
	{                     //从下向上开始巡线
		uchar* this_row = mat.ptr(i);                  //指向第i行
		if (this_row[center] == 0) {
			if (count_white(store.image_mat, i) < 5) {
				exist_left_edge_point[i] = false;
				exist_right_edge_point[i] = false;
				break;
			}
			while (true) {
				if (this_row[(center + m < IMGW ? center + m : IMGW - 1)] != 0) {
					center = (center + m < IMGW ? center + m : IMGW - 1);
					m = 1;
					break;
				}
				if (this_row[center - m >= 0 ? center - m : 0] != 0) {
					center = center - m >= 0 ? center - m : 0;
					m = 1;
					break;
				}
				m++;
			}
		}
		if (this_row[center] != 0)
		{
			if(lost_left){
				for (j = center + 1; j < IMGW - 1; j++) {                                    //找寻右边界 
					count++;
					if (this_row[j - 1] != 0 && this_row[j] == 0) {
						right_edge_point[i] = j;
						exist_right_edge_point[i] = true;
						last_right = j;
						break;
					}
					if (j == IMGW - 1) {
						exist_right_edge_point[i] = false;
					}
				}
			}
			if(lost_right){
				for (j = center - 1; j > 0; j--) {                                            //找寻左边界
					count++;
					if (this_row[j + 1] != 0 && this_row[j] == 0) {
						left_edge_point[i] = j;
						exist_left_edge_point[i] = true;
						last_left = j;
						break;
					}
					if (j == 1)
					{
						exist_left_edge_point[i] = false;
					}
				}
			}

		}
		center = (last_left + last_right) / 2;
		if (last_left > last_right) {
			center_lost = i;
			break;
		}
		if (count < 3 * MINX) {    //3 * MINX
			center_lost = i;
			break;
		}
		if ((exist_left_edge_point[i] && left_edge_point[i] > IMGW - 8) || (exist_right_edge_point[i] && right_edge_point[i] < 8)) {
			center_lost = i;
			break;
		}
		if (exist_left_edge_point[i]) {
			if (count_white(mat, i,
				(left_edge_point[i] + 5 < IMGW ? left_edge_point[i] + 5 : IMGW), (left_edge_point[i] - 4 > -1 ? left_edge_point[i] - 4 : 0)) == 0) {
				center_lost = i;
				break;
			}
		}
		if (exist_right_edge_point[i]) {
			if (count_white(mat, i,
				(right_edge_point[i] + 5 < IMGW ? right_edge_point[i] + 5 : IMGW), (right_edge_point[i] - 4 > -1 ? right_edge_point[i] - 4 : 0)) == 0) {
				center_lost = i;
				break;
			}
		}
		count = 0;
	}
}
void MainImage::find_center_in_garage(Point top_cone){
	for (int i = IMGH - 1; i > 0; i--) {
		exist_left_edge_point[i] = false;
		exist_right_edge_point[i] = false;
	}
	// Point car_center;
	int car_center_x = IMGW/2;
	int car_center_y = IMGH-1;
	float garage_k;
	int garage_last_center = IMGW/2;
	float temp_center  = IMGW/2;
	int px = top_cone.x;
	int py = top_cone.y;
	garage_k = -(float)(px-car_center_x)/(float)(py-car_center_y);


	cerr<<"px: "<<px<<"py: "<<py<<endl;
	cerr<<garage_k<<endl;
	for(int i = IMGH-1; i >py; i--){
		temp_center = temp_center + garage_k;
		center_point[i] = temp_center;
		int c = center_point[i];
		// cerr<<c<<endl;
		garage_last_center = i;
	}
	center_lost = garage_last_center;
	for (int i = center_lost; i > -1; i--) {
	center_point[i] = center_point[center_lost + 1];
    } 
	// cerr<<"try to refind center"<<endl;
	// for (int i = IMGH - 1; i >= 0; i--) {
	// 	if(lost_right){
	// 		int j;
	// 		for (j = i; j >= 0; j--) {
	// 			if (!exist_left_edge_point[j] || exist_right_edge_point[j]) break;
	// 		}
	// 		int mov, devide;
	// 		float wid;
	// 		if (i > IMGH - 5 && IMGW - left_edge_point[i] < 140) {
	// 			wid = 125;//IMGW - left_edge_point[i];110
	// 		}
	// 		else wid = IMGW - left_edge_point[i];
	// 		for (int k = i; k > j; k--) {
	// 			wid -= 0.28;//0.43
	// 			//devide = (left_edge_point[k] + IMGW) / 2;
	// 			mov = (2 * left_edge_point[k] + round(wid)) / 2;
	// 			center_point[k] = ((2 * mov) / 2 < IMGW ? (2 * mov) / 2 : IMGW - 1);
	// 		}
	// 		i = j + 1;
	// 	}
	// 	else if(lost_left){
	// 			int j;
	// 			for (j = i; j >= 0; j--) {
	// 				if (!exist_right_edge_point[j] || exist_left_edge_point[j]) break;
	// 			}
	// 			int mov, devide;
	// 			float wid;
	// 			if (i > IMGH - 5 && right_edge_point[i] < 140) {
	// 				wid = 125;//(2 * right_edge_point[i] - 100)/2;110
	// 			}
	// 			else wid = right_edge_point[i];

	// 			for (int k = i; k > j; k--) {
	// 				wid -= 0.28f;
	// 				//devide = right_edge_point[k] / 2;
	// 				mov = (2 * right_edge_point[k] - round(wid)) / 2;
	// 				center_point[k] = ((2 * mov) / 2 > -1 ? (2 * mov) / 2 : 0);
	// 			}
	// 			i = j + 1;
	// 	}
	// }
}
void MainImage::find_center_in_farm(int begin) {
	if (center_cone.size() > 0)
	{
		for (int i = center_cone.size() - 1; i > 0; i--)
		{
			float x1 = center_cone[i].x;
			float x2 = center_cone[i - 1].x;
			float y1 = center_cone[i].y;
			float y2 = center_cone[i - 1].y;
			for (int j = center_cone[i].y; j <= center_cone[i - 1].y; j++)
			{
				center_point[j] = x1 + (j - y1) * (x2 - x1) / (y2 - y1);
				exist_left_edge_point[j] = true;
				exist_right_edge_point[j] = true;
			}
		}
		for (int j = center_cone[0].y; j <= begin; j++)
		{
			float x = center_cone[0].x;
			int bottom;
			if (begin < IMGH - 5)bottom = begin + 8 < IMGH - 5 ? center_point[begin + 8] : IMGW / 2;
			else bottom = IMGW / 2;
			center_point[j] = x + (float)(j - center_cone[0].y) * (bottom - (float)center_cone[0].x) / (center_lost - (float)center_cone[0].y);
			exist_left_edge_point[j] = true;
			exist_right_edge_point[j] = true;
		}
	}
}

void MainImage::refind_edge_in_farm_out(Point start) {
	int i, j;
	int last_right = IMGW - 1;
	int last_left = 0;
	center_lost = 0;
	int center = start.x;
	for (i = start.y; i > center_lost; i--)
	{
		uchar* r = store.image_mat.ptr(i);
		if (r[center] == 0) { center_lost = i; break; }
		for (j = center + 1; j < IMGW - 1; j++) {                                    //找寻右边界 
			if (r[j - 1] != 0 && r[j] == 0) {
				right_edge_point[i] = j;
				exist_right_edge_point[i] = true;
				last_right = j;
				break;
			}
			if (j == IMGW - 1) {
				exist_right_edge_point[i] = false;
			}
		}
		for (j = center - 1; j > 0; j--) {                                            //找寻左边界
			if (r[j + 1] != 0 && r[j] == 0) {
				left_edge_point[i] = j;
				exist_left_edge_point[i] = true;
				last_left = j;
				break;
			}
			if (j == 1)
			{
				exist_left_edge_point[i] = false;
			}
		}
		center = (last_left + last_right) / 2;
		center_point[i] = center;
	}
}

void MainImage::produce_dv(int error) {
	int i;
	int dv = 0;
	float delta = 0;
	if (abs(error) < 6) {
		for (i = IMGH - 40; i > 30; i--) {
			if (abs(center_point[i] - IMGW / 2) < 6) {
				delta += 1;
			}
			else {
				break;
			}
		}
		dv += round(delta);
	}
	return;
}


bool j_zebra_line(const MainImage& mi, uchar& state_in){
	bool j_line = false;
	for(int i = IMGH-1; i > 0; i--){
		int cross_count=0;
		const uchar* row = mi.store.image_mat.ptr<uchar>(i);
		for(int j = mi.left_edge_point[i]-5; j < mi.right_edge_point[i]+5; j++){
			if(row[j]!=row[j+1]){
				cross_count++;
			}
			if(cross_count>15){
				break;
			}
		}
		//cerr<<"cross_count"<< cross_count<<endl;
		if(cross_count>15){
			j_line = true;
			break;
		}
	}
	return j_line;
}

bool j_right_circle_in_circle(const MainImage& mi, uchar& state_in)
{
	/*if (mi.right_end_point[0].y < IMGH - 5) return false;
	if (mi.right_end_point.size() == 4 && mi.right_end_point[1].y > IMGH - 30) {
		//state_in = right_circle_in_circle;
	return true;
	}*/
	int i, max = IMGW - 1;
	if (mi.right_end_point.size() > 2)
	{
		for (i = mi.right_end_point[0].y; i > mi.right_end_point[1].y; i--)
		{
			if (mi.right_edge_point[i] < max)
			{
				max = mi.right_edge_point[i];
			}
		}
		if ((max != IMGW - 1 && max > IMGW - 30 && mi.right_end_point[0].y>IMGH-25) || mi.right_end_point[1].y > IMGH - 30)return true;
	}
	else return true;
	return false;
}

bool j_right_circle_inside_before(const MainImage& mi, uchar& state_in)
{
	if (mi.left_end_point.size() > 0 && mi.left_end_point[1].x > IMGW - 60 && mi.left_end_point[0].y <IMGH-20) {//50, 30
		state_in = right_circle_inside_before;
		return true;
	}
	return false;
}

bool j_right_circle_inside(const MainImage& mi, uchar& state_in)
{
	if ((mi.right_end_point.size() == 0 && mi.left_end_point.size() == 2 && mi.left_end_point[1].x > IMGW / 2 + 20 &&
		(mi.left_end_point[0].y > IMGH - 5 || mi.left_end_point[0].x < 5)) ||
		(mi.right_end_point.size() == 2 && mi.right_end_point[1].y > IMGH / 2 + 10 && mi.left_end_point.size() == 2 && mi.left_end_point[1].x > IMGW / 2 + 20 &&
			(mi.left_end_point[0].y > IMGH - 5 || mi.left_end_point[0].x < 5)))
	{
		return true;
	}
	return false;
}

void j_right_circle_out_find(const MainImage& mi, uchar& state_in)
{
	int i, count = 0;
	const uchar* r = mi.store.image_mat.ptr<uchar>(mi.left_end_point[1].y - 1);
	for (i = mi.left_end_point[1].x - 3; i < mi.left_end_point[1].x + 4; i++) {
		if (r[i] != 0)count++;
	}
	if (mi.left_end_point.size() == 2 && (mi.left_end_point[0].y > IMGH - MINY || mi.left_end_point[0].x < MINX) && count > 2) {
		state_in = right_circle_out_find;
	}
}

//Re
void j_right_circle_out_strai(const MainImage& mi, uchar& state_in)
{
	int count = 0;
	//const uchar* r = mi.store.image_mat.ptr<uchar>(IMGH - mi.re.r_circle.out_strai_find_pos);
	const uchar* r = mi.store.image_mat.ptr<uchar>(mi.left_end_point[1].y - 10); //before .y-20
	for (int i = 0; i < IMGW; i++) {
		if (r[i] != 0)count++;
		else break;
	}
	if (count > IMGW - 10) {
		state_in = right_circle_out_strai;
	}
}

void j_right_circle_out_out(const MainImage& mi, uchar& state_in)
{
	if (mi.left_end_point.size() != 0 && abs(mi.left_end_point[0].y - mi.left_end_point[1].y) > 30 &&
		mi.right_end_point.size() != 0 && mi.right_end_point[0].y < IMGH - 30) {
		state_in = right_circle_out_out;
	}
}

bool j_left_circle_in_circle(const MainImage& mi, uchar& state_in) {
	/*if (mi.left_end_point[0].y < IMGH - 5) return false;
	if (mi.left_end_point.size() == 4 && mi.left_end_point[1].y > IMGH - 30) {
		//state_in = left_circle_in_circle;
	return true;
	}*/
	int i, max = 0;
	if (mi.left_end_point.size() > 2)
	{
		for (i = mi.left_end_point[0].y; i > mi.left_end_point[1].y; i--)
		{
			if (mi.left_edge_point[i] > max)
			{
				max = mi.left_edge_point[i];
			}
		}
		if ((max != 0 && max < 30) || mi.left_end_point[1].y > IMGH - 30)return true;
	}
	else return true;
	return false;
}

bool j_left_circle_inside_before(const MainImage& mi, uchar& state_in) {
	if (mi.right_end_point.size() > 0 && mi.right_end_point[1].x < 40) {
		state_in = left_circle_inside_before;
		return true;
	}
	return false;
}

bool j_left_circle_inside(const MainImage& mi, uchar& state_in) {
	if ((mi.left_end_point.size() == 0 && mi.right_end_point.size() == 2 && mi.right_end_point[1].x < IMGW / 2 - 20 &&
		(mi.right_end_point[0].y > IMGH - 5 || mi.right_end_point[0].x > IMGW - 5)) ||
		(mi.left_end_point.size() == 2 && mi.left_end_point[1].y > IMGH / 2 + 10 && mi.right_end_point.size() == 2 && mi.right_end_point[1].x < IMGW / 2 - 20 &&
			(mi.right_end_point[0].y > IMGH - 5 || mi.right_end_point[0].x > IMGW - 5)))
	{
		return true;
	}
	return false;
}

void j_left_circle_out_find(const MainImage& mi, uchar& state_in) {
	int i, count = 0;
	const uchar* r = mi.store.image_mat.ptr<uchar>(mi.right_end_point[1].y - 1);
	for (i = mi.right_end_point[1].x - 3; i < mi.right_end_point[1].x + 4; i++) {
		if (r[i] != 0)count++;
	}
	if (mi.right_end_point.size() == 2 && (mi.right_end_point[0].y > IMGH - MINY || mi.right_end_point[0].x > IMGW - MINX) && count > 2) {
		state_in = left_circle_out_find;
	}
}

void j_left_circle_out_strai(const MainImage& mi, uchar& state_in) {
	int count = 0;
	//const uchar* r = mi.store.image_mat.ptr<uchar>(IMGH - mi.re.l_circle.out_strai_find_pos);
	const uchar* r = mi.store.image_mat.ptr<uchar>(mi.right_end_point[1].y - 20);
	for (int i = IMGW - 1; i > -1; i--) {
		if (r[i] != 0)count++;
		else break;
	}
	if (count > IMGW - 10) {
		state_in = left_circle_out_strai;
	}
}

void j_left_circle_out_out(const MainImage& mi, uchar& state_in) {
	if (mi.right_end_point.size() != 0 && abs(mi.right_end_point[0].y - mi.right_end_point[1].y) > 30 &&
		mi.left_end_point.size() != 0 && mi.left_end_point[0].y < IMGH - 30) {
		state_in = left_circle_out_out;
	}
}

bool j_farm_in_find(const MainImage& mi, uchar& state_in) {
	int i;
	int start = IMGW / 2;
	int y = IMGH - 1;
	int count = 0;
	int error = 0;
	const uchar* r = mi.store.image_mat.ptr<uchar>(y);
	while (r[start] != 0)
	{
		y--;
		const uchar* r = mi.store.image_mat.ptr<uchar>(y);
	}
	if (y < 50 || y > 70)return false;
	r = mi.store.image_mat.ptr<uchar>(y);
	for (i = 0; i < 30; i++)
	{
		int k;
		int c = 0;
		bool left = false;
		bool right = false;
		y--;
		if (r[start] != 0)return false;
		const uchar* r = mi.store.image_mat.ptr<uchar>(y - 2);
		/*for (k = start - 1; k >= start - 35; k--) {
			if (r[k] == 0)c++;
			else {
				left = true;
			}
		}
		for (k = start + 1; k <= start + 35; k++) {
			if (r[k] == 0)c++;
			else {
				right = true;
			}
		}
		if (!left || !right)return false;
		if (c < 20)error++;
		if (error > 2)return false;*/
	}
	return true;
}

void j_farm_inside(const MainImage& mi, uchar& state_in) {
	int c1, c2, c3;
	c1 = count_white(mi.store.image_mat, IMGH - 10);
	c2 = count_white(mi.store.image_mat, IMGH - 11);
	c3 = count_white(mi.store.image_mat, IMGH - 12);
	if (c1 < 50 && c2 < 50 && c3 < 50)
	{
		state_in = farm_inside;
	}
}

void j_left_garage_before(const MainImage& mi, uchar& state_in){
	int cone_point = 0;
	for(int i = IMGH-1; i > 0; i--){
		const uchar* row = mi.store.image_mat.ptr<uchar>(i);
		int cone_flag = 0;
		for(int j = mi.left_edge_point[i]+1; j > mi.left_edge_point[i]+10; j++){
			if(row[j]==0){
				cone_flag++;
			}
			cv::Vec3b val = mi.store.image_BGR_small.at<cv::Vec3b>(i, j);

			if(val[0]<=50 && val[1]>=120 && val[2]>=120){
				// int r,g,b;
				// b = val[0];
				// g = val[1];
				// r = val[2];
				// cout<<"r "<<r<<" g "<<g<<" b "<<b<<endl;	
				// cout<<i<<endl;
				// cout<< "cone flag" << cone_flag << endl;
				if(i > IMGH-5){
					// mi.top_cone.x = j;
					// mi.top_cone.y = i;
					
					state_in = left_garage_into;
					break;
				}
			
			}
		}
	}
}

void j_left_garage_into(const MainImage& mi, uchar& state_in){

}

void j_farm_out_find(const MainImage& mi, uchar& state_in) {
	/*if (mi.right_cone.back().y > IMGH / 2 && mi.right_cone.back().x < IMGW - 40)
	{
		int count = 0;
		int start = mi.right_cone.back().x;
	if(mi.right_cone.back().y>40)
	{
	  const uchar* r = mi.store.image_mat.ptr<uchar>(mi.right_cone.back().y + 15);
		for (int i = start - 1; i > 0; i--)
		  {
			  if (r[i] != 0)count++;
		}
		if (count > mi.re.farm.out_find_count)state_in = farm_out_find;
	}
	}
	if (mi.left_cone.back().y > IMGH / 2 && mi.left_cone.back().x > 40)
	{
		int count = 0;
		int start = mi.left_cone.back().x;
	if(mi.left_cone.back().y>40)
	{
	  const uchar* r = mi.store.image_mat.ptr<uchar>(mi.left_cone.back().y + 15);
		  for (int i = start + 1; i < IMGW - 1; i++)
		  {
			if (r[i] != 0)count++;
		  }
		  if (count > mi.re.farm.out_find_count)state_in = farm_out_find;
	}
	}*/

	/*int i;
	int count = 0;
	int h=mi.left_cone.back().y<mi.right_cone.back().y?mi.left_cone.back().y:mi.right_cone.back().y;
	h=h>30?h-10:h;
	const uchar* r = mi.store.image_mat.ptr<uchar>(h);
	for(i=mi.left_cone.back().x;i<mi.right_cone.back().x;i++)
	{
	  if(r[i]!=0)count++;
	  else{count=0;break;}
	}
	if(count > mi.re.farm.out_find_count)state_in=farm_out_find;
	*/

	//********************

	/*int i;
	int count = 0;
	const uchar* r = mi.store.image_mat.ptr<uchar>(IMGH - mi.re.farm.out_find_line);
	for(i=32;i<IMGW-32;i++)
	{
	  if(r[i]!=0)count++;
	}
	if(count > mi.re.farm.out_find_count)state_in=farm_out_find;*/

	//********************

	int i;
	int count = 0;
	int length;
	if (mi.left_cone.size() > 0 && mi.right_cone.size() > 0)
	{
		int y = mi.left_cone.back().y < mi.right_cone.back().y ? mi.left_cone.back().y : mi.right_cone.back().y;
		y = y >= 40 ? y - 10 : 30;
		length = mi.right_cone.back().y - mi.left_cone.back().y;
		const uchar* r = mi.store.image_mat.ptr<uchar>(y);
		for (i = mi.left_cone.back().y; i < mi.right_cone.back().y; i++)
		{
			if (r[y] != 0)count++;
		}
		if (count / length > 0.8)state_in = farm_out_find;
	}

}

void j_farm_out(const MainImage& mi, uchar& state_in) {
	//if (mi.center_cone.back().y >= IMGH - 5)state_in = farm_out;
	int i;
	int count = 0;
	const uchar* r = mi.store.image_mat.ptr<uchar>(IMGH - mi.re.farm.out_line);
	for (i = 0; i < IMGW - 32; i++)
	{
		if (r[i] != 0)count++;
	}
	if (count > mi.re.farm.out_count)state_in = farm_out;
}



void j_hump_on(const MainImage& mi, uchar& state_in) {
	int i, k;
	int count = 0;
	bool find_down = false;
	for (k = 0; k < 4; k++)
	{
		const uchar* r_down = mi.store.image_mat.ptr<uchar>(IMGH - mi.re.hump.on_line - k);
		for (i = 1; i < IMGW; i++)
		{
			if (r_down[i] != 0 && r_down[i - 1] == 0)
			{
				int k = 0;
				while (r_down[i + k] != 0)k++;
				if (k <= 15)continue;
				else if (k < 70) count++;
				else count = 0;
				break;
			}
		}
	}
	if (count > 2)state_in = hump_on;
}

void j_hump_out(const MainImage& mi, uchar& state_in) {
	int i, k;
	int count = 0;
	bool find_down = false;
	for (k = 0; k < 50; k++)
	{
		const uchar* r_down = mi.store.image_mat.ptr<uchar>(IMGH - 1 - k);
		for (i = 1; i < IMGW; i++)
		{
			if (r_down[i] != 0 && r_down[i - 1] == 0)
			{
				int k = 0;
				while (r_down[i + k] != 0)k++;
				if (k <= 15)continue;
				else if (k > 60) count++;
				else count = 0;
				break;
			}
		}
	}
	if (count == 50)state_in = hump_out;
}



int count_white(const Mat& img, int row, int end, int start)    //第row行白点数统计
{
	int i, count = 0;
	const uchar* this_row = img.ptr<uchar>(row);
	for (i = start; i < end; i++) {
		if (this_row[i] != 0)
			count++;
	}
	return count;
}
int count_wid(const Mat& src, Point seed) {
	int i, count = 1;
	const uchar* this_row = src.ptr<uchar>(seed.y);
	if (this_row[seed.x] != 0) {
		for (i = seed.x - 1; i > -1; i--) {
			if (this_row[i] != 0)count++;
			else break;
		}
		for (i = seed.x + 1; i < IMGW; i++) {
			if (this_row[i] != 0)count++;
			else break;
		}
		return count;
	}
	else return(-1);
}

void line(Mat& img, Point A, Point B, int color)
{
	float x0 = A.x;
	float y0 = A.y;
	float x1 = B.x;
	float y1 = B.y;
	int x, y;
	float k = (y1 - y0) / (x1 - x0);
	if (x1 < x0)
	{
		while (true)
		{
			if (k > 0)
			{
				if (k > 1)
				{
					x0 -= 1 / k;
					y0--;
				}
				else
				{
					x0--;
					y0 -= k;
				}
				x = round(x0);
				y = round(y0);
				if (x <= x1 && y <= y1)break;
				img.ptr<uchar>(y)[x] = color;
			}
			else
			{
				if (k < -1)
				{
					x0 += 1 / k;
					y0++;
				}
				else
				{
					x0--;
					y0 -= k;
				}
				x = round(x0);
				y = round(y0);
				if (x <= x1 && y >= y1)break;
				img.ptr<uchar>(y)[x] = color;
			}
		}
	}
	else
	{
		while (true)
		{
			if (k > 0)
			{
				if (k > 1)
				{
					x0 += 1 / k;
					y0++;
				}
				else
				{
					x0++;
					y0 += k;
				}
				x = round(x0);
				y = round(y0);
				if (x >= x1 && y >= y1)break;
				img.ptr<uchar>(y)[x] = color;
			}
			else
			{
				if (k < -1)
				{
					x0 -= 1 / k;
					y0--;
				}
				else
				{
					x0++;
					y0 += k;
				}
				x = round(x0);
				y = round(y0);
				if (x >= x1 && y <= y1)break;
				img.ptr<uchar>(y)[x] = color;
			}
		}
	}
}
void ray(Mat& img, Point start, float angle, int color)
{
	float x = start.x;
	float y = start.y;
	int x0, y0;
	float k = tan(angle);
	if (abs(angle) > 1.5707)
	{
		while (true)
		{
			if (k > 1) {
				x -= 1 / k;
				y++;
			}
			else if (k > 0 && k <= 1) {
				x--;
				y += k;
			}
			else if (k > -1 && k <= 0) {
				x--;
				y += k;
			}
			else if (k <= -1) {
				x += 1 / k;
				y--;
			}
			x0 = round(x);
			y0 = round(y);
			if (y0 > IMGH - 1 || y0 < 1 || x0 < 1 || x0 > IMGW - 1) break;
			img.ptr<uchar>(y0)[x0] = color;
		}
	}
	else
	{
		while (true)
		{
			if (k > 1) {
				x += 1 / k;
				y--;
			}
			else if (k > 0 && k <= 1) {
				x++;
				y -= k;
			}
			else if (k > -1 && k <= 0) {
				x++;
				y -= k;
			}
			else if (k <= -1) {
				x -= 1 / k;
				y++;
			}
			x0 = round(x);
			y0 = round(y);
			if (y0 > IMGH - 1 || y0 < 1 || x0 < 1 || x0 > IMGW - 1) break;
			img.ptr<uchar>(y0)[x0] = color;
		}
	}
}


void MainImage::find_far_zebra() {
	int zebra_dist = re.zebra.zebra_far_dist;
	int thresh = IMGH - zebra_dist;
	int i, j;
	int count = 0, r_count = 0;
	bool f = false;
	uchar* r;
	uchar* r_b;
	for (i = thresh; i >= thresh - 8; i--) {
		r = store.image_mat.ptr<uchar>(i);
		r_b = store.image_mat_cone.ptr<uchar>(i);
		for (j = 10; j < IMGW - 10; j++) {
			if (r[j - 2] != 0 && r[j - 1] != 0 && r[j] != 0 && r_b[j - 2] != 0 && r_b[j - 1] != 0 && r_b[j] != 0) {
				f = true;
				break;
			}
		}
		if (!f) return;
		for (; j < IMGW; j++) {
			if (r[j] == 0 && r[j + 1] == 0 && r[j + 2] == 0 && r[j + 3] == 0 && r[j + 4] == 0 && r[j + 5] == 0 && r[j + 6] == 0 && r[j + 7] == 0 && r[j + 8] == 0) break;
			if (r[j] != r[j + 1]) count++;
			if ((r[j] != 0 && r[j + 1] == 0 && r[j + 2] != 0) || (r[j] != 0 && r[j + 1] == 0 && r[j + 2] == 0 && r[j + 3] != 0)) count = count - 2;
		}
		cout<<"zebra count"<<count<<endl;
		if (count >= 10) {
			r_count++;
			if (r_count >= 3) {
				zebra_far_find = true;
				return;
			}
		}
		else {
			count = 0;
		}
	}
	zebra_far_find = false;
}

void MainImage::find_near_zebra() {
	int zebra_dist = re.zebra.zebra_near_dist;
	int thresh = IMGH - zebra_dist;
	int i, j;
	int count = 0, r_count = 0;
	bool f = false;
	uchar* r;
	for (i = thresh; i >= thresh - 8; i--) {
		r = store.image_mat.ptr<uchar>(i);
		for (j = 10; j < IMGW - 10; j++) {
			if (r[j - 2] != 0 && r[j - 1] != 0 && r[j] != 0) {
				f = true;
				break;
			}
		}
		if (!f) return;
		for (; j < IMGW; j++) {
			if (r[j] == 0 && r[j + 1] == 0 && r[j + 2] == 0 && r[j + 3] == 0 && r[j + 4] == 0 && r[j + 5] == 0 && r[j + 6] == 0 && r[j + 7] == 0 && r[j + 8] == 0) break;
			if (r[j] != r[j + 1]) count++;
		}
		if (count >= 10) {
			r_count++;
			if (r_count >= 2) {
				zebra_near_find = true;
				return;
			}
		}
		else {
			count = 0;
		}
	}
	zebra_near_find = false;

}

void MainImage::judge_cone_side(){
	int right_b_count = 0;
	int left_b_count=0;
	int left_cone_x = 0;
	int left_cone_y = 0;
	int right_cone_x = 0;
	int right_cone_y = 0;
	Point left_cone;
	Point right_cone;
	left_cone.y = 0;
	right_cone.y = 0;
	int last_right_cone, last_left_cone;
	int right_cone_num, left_cone_num=0;
	int last_right_y = 0;
	int last_left_y = 0;
	int start_left, start_right = 0;
	exist_right_cone = false;
	exist_left_cone = false;
	for(int i = IMGH-Re.cone.judge_down_scope; i > (center_lost<Re.cone.judge_up_scope?Re.cone.judge_up_scope:center_lost); i--){
		uchar* row_b = store.image_mat_cone.ptr<uchar>(i);
		uchar* row_r = store.image_mat.ptr<uchar>(i);
		// if(b_count>35){
		// 	break;
		// }
		if(exist_left_edge_point[i]&& exist_right_edge_point[i]){
			for(int j = left_edge_point[i]+5; j < center_point[i]; j++){
				if(row_b[j] == 0&&row_r[j]!=0){
					if(last_left_y!=0 && abs(last_left_y - i) > 5 && exist_right_cone && exist_left_cone){
						break;
					}
					if(left_b_count==0){
						start_left = i;
					}
					left_cone.x = j;
					left_cone.y = i;
					left_cone_point.push_back(left_cone);
					// store.image_mat.ptr<uchar>(i)[j] = 0;
					left_cone_x+=j;
					// cone_right+=abs(j-mi.right_edge_point[i]);
					left_cone_y+=i;
					left_b_count++;
					last_left_y = i;
					
				}
			}
			for(int j = right_edge_point[i]-5; j > center_point[i]; j--){
				if(row_b[j] == 0&&row_r[j]!=0){
					if(last_right_y!=0 && abs(last_right_y - i) > 5 && exist_right_cone && exist_left_cone){
						break;
					}
					if(right_b_count==0){
						start_right = i;
					}
					right_cone.x = j;
					right_cone.y = i;
					right_cone_point.push_back(right_cone);
					// cerr<<"hey"<<endl;
					// store.image_mat.ptr<uchar>(i)[j] = 0;
					right_cone_x+=j;
					// cone_right+=abs(j-mi.right_edge_point[i]);
					right_cone_y+=i;
					right_b_count++;
					last_right_y = i;
					// cerr<<start_right<<endl;
				}
			}
		}else if(!exist_right_edge_point[i]&&exist_left_edge_point[i]){
			for(int j = left_edge_point[i]+5; j < center_point[i]; j++){
				if(row_b[j] == 0&&row_r[j]!=0){
					if(last_left_y!=0 && abs(last_left_y - i) > 5 && exist_right_cone && exist_left_cone){
						break;
					}
					if(left_b_count==0){
						start_left = i;
					}
					left_cone.x = j;
					left_cone.y = i;
					left_cone_point.push_back(left_cone);
					// store.image_mat.ptr<uchar>(i)[j] = 0;
					left_cone_x+=j;
					// cone_right+=abs(j-mi.right_edge_point[i]);
					left_cone_y+=i;
					left_b_count++;
					last_left_y = i;
					
				}
			}
			for(int j = IMGW-6; j > center_point[i]; j--){
				if(row_b[j] == 0&&row_r[j]!=0){
					if(last_right_y!=0 && abs(last_right_y - i) > 5 && exist_right_cone && exist_left_cone){
						break;
					}
					if(right_b_count==0){
						start_right = i;
					}
					right_cone.x = j;
					right_cone.y = i;
					right_cone_point.push_back(right_cone);
					// cerr<<"hey"<<endl;
					// store.image_mat.ptr<uchar>(i)[j] = 0;
					right_cone_x+=j;
					// cone_right+=abs(j-mi.right_edge_point[i]);
					right_cone_y+=i;
					right_b_count++;
					last_right_y = i;
					// cerr<<start_right<<endl;
				}
			}
		}else if(exist_right_edge_point[i]&&!exist_left_edge_point[i]){
			for(int j = 5; j < center_point[i]; j++){
				if(row_b[j] == 0&&row_r[j]!=0){
					if(last_left_y!=0 && abs(last_left_y - i) > 5 && exist_right_cone && exist_left_cone){
						break;
					}
					if(left_b_count==0){
						start_left = i;
					}
					left_cone.x = j;
					left_cone.y = i;
					left_cone_point.push_back(left_cone);
					// store.image_mat.ptr<uchar>(i)[j] = 0;
					left_cone_x+=j;
					// cone_right+=abs(j-mi.right_edge_point[i]);
					left_cone_y+=i;
					left_b_count++;
					last_left_y = i;
					
				}
			}
			for(int j = right_edge_point[i]-5; j > center_point[i]; j--){
				if(row_b[j] == 0&&row_r[j]!=0){
					if(last_right_y!=0 && abs(last_right_y - i) > 5 && exist_right_cone && exist_left_cone){
						break;
					}
					if(right_b_count==0){
						start_right = i;
					}
					right_cone.x = j;
					right_cone.y = i;
					right_cone_point.push_back(right_cone);
					// cerr<<"hey"<<endl;
					// store.image_mat.ptr<uchar>(i)[j] = 0;
					right_cone_x+=j;
					// cone_right+=abs(j-mi.right_edge_point[i]);
					right_cone_y+=i;
					right_b_count++;
					last_right_y = i;
					// cerr<<start_right<<endl;
				}
			}
		}
		// cerr<<"left_b_count"<<left_b_count<<endl;
		// cerr<<"right_b_count"<<right_b_count<<endl;
		if(left_b_count>Re.cone.b_cone_count){
			exist_left_cone = true;
		}
		if(right_b_count>Re.cone.b_cone_count){
			exist_right_cone = true;
		}

	}
	// if(right_b_count>25&&left_b_count>25){
	if((right_cone_y/right_b_count)>(left_cone_y/left_b_count)&&exist_right_cone&&last_right_y<80){
		right_cone_first = true;
		left_cone_first = false;
		// state_cone = first_right_cone;

	}
	else if((right_cone_y/right_b_count)<(left_cone_y/left_b_count)&&exist_left_cone&&last_left_y<80){
		// state_cone = first_left_cone;
		left_cone_first = true;
		right_cone_first = false;

	}
	else{
		if(!exist_left_cone&&!exist_left_cone){
			if(right_cone_first){
				state_cone = left_block;
			}else{
				state_cone = right_block;
			}
			// state_out = straight;
		}
	}
	single_right_cone.x = right_cone_x/right_b_count;
	single_right_cone.y = right_cone_y/right_b_count;
	single_left_cone.x = left_cone_x/left_b_count;
	single_left_cone.y = left_cone_x/left_b_count;
	Point cone_edge_point;
	Point cone_end_point;
	if(left_cone_first){
		cone_edge_point.x = single_left_cone.x;
		cone_edge_point.y = single_left_cone.y;
		// cone_end_point.x = right_edge_point[single_left_cone.y+10];
		// cone_end_point.y = single_left_cone.y+10;
		for(int i = IMGH-1; i >last_left_y ; i--){
			exist_left_edge_point[i] = true;
			left_edge_point[i] = center_point[i]+2;
		}

		// for(int i = last_left_y; i > last_left_y-20; i--){
		// 	exist_right_edge_point[i] = true;
		// 	right_edge_point[i] = center_point[i];
		// 	// cerr<<"hey"<<endl;
		// }
	}else if(right_cone_first){
		cone_edge_point.x = single_right_cone.x;
		cone_edge_point.y = single_right_cone.y;
		// cone_end_point.x = right_edge_point[single_left_cone.y+10];
		// cone_end_point.y = single_left_cone.y+10;
		for(int i = IMGH-1; i > last_right_y; i--){
			exist_right_edge_point[i] = true;
			right_edge_point[i] = center_point[i]-2;
		}
		// for(int i = last_right_y; i > last_right_y-20; i--){
		// 	exist_left_edge_point[i] = true;
		// 	left_edge_point[i] = center_point[i];
		// }

	}
	find_center();
	// }
	// else if(rifht_b_cout)
	// state_in = ((cone_left)>(cone_right)?first_right_cone:first_left_cone);
	// if((cone_y/b_count) > Re.main.cone_slowdown_thresh){
	// 	state_in = ((cone_left)>(cone_right)?first_right_cone:first_left_cone);
	// }
}

// void MainImage::find_edge_point_for_cone(){
// 	Point cone_edge_point;
// 	Point cone_end_point;
// 	if(left_cone_first){
// 		cone_edge_point.x = single_left_cone.x;
// 		cone_edge_point.y = single_left_cone.y;
// 		cone_end_point.x = right_edge_point[single_left_cone.y+10];
// 		cone_end_point.y = single_left_cone.y+10;
// 		for(int i = IMGH-1; i > cone_edge_point.y; i--){
// 			left_edge_point[i] = cone_edge_point.x;
// 		}
// 	}
// }



void MainImage::find_edge_point_end()
{
	Mat mat = store.image_mat;
	int i, j, center = last_center;
	int last_right = IMGW - 1;
	int last_left = 0;
	int count = 0;
	int m = 1;
	uchar* r = mat.ptr<uchar>(IMGH - 1);           //指向最下面一行

	for (j = last_center + 1; j < IMGW; j++) {                 //中线向右找寻右边界
		if (j == IMGW - 1) {
			last_right = IMGW - 1;
		}
		if (r[j - 1] != 0 && r[j] == 0 && r[j + 1] == 0) {
			last_right = j;
			break;
		}
	}
	for (j = last_center - 1; j > -1; j--) {                   //中线向左找寻左边界
		if (j == 0) {
			last_left = 0;
		}
		if (r[j + 1] != 0 && r[j] == 0 && r[j - 1] == 0) {
			last_left = j;
			break;
		}
	}
	

	for (i = IMGH - 1; i > center_lost; i--) {                     //从下向上开始巡线
		uchar* this_row = mat.ptr(i);                  //指向第i行
		if (this_row[center] == 0) {
			if (count_white(mat, i) <= 8)  //第i行白像素数量不大于8，则第i行中心位置缺失
			{
				exist_left_edge_point[i] = false;
				exist_right_edge_point[i] = false;
				center_lost = i;
				break;
			}
			while (true)
			{
				//将中心点挪到白色位置
				if (this_row[(center + m < IMGW ? center + m : IMGW - 1)] != 0)
				{
					center = (center + m < IMGW ? center + m : IMGW - 1);
					// for(int k = (center + m < IMGW ? center + m : IMGW - 1); k < IMGW-1; k++){
					// 	if(this_row[k-1]==0&&this_row[k]!=0){
					// 		center = k;
					// 	}
					// }

					m = 1;
					break;
				}
				if (this_row[center - m >= 0 ? center - m : 0] != 0)
				{
					center = center - m >= 0 ? center - m : 0;
					// for(int k = (center - m < 0 ? center - m : 0); k > 0; k--){
					// 	if(this_row[k+1]==0&&this_row[k]!=0){
					// 		center = k;
					// 	}
					// }
					m = 1;
					break;
				}
				m++;
			}
		}
		for (j = center + 1; j < IMGW; j++) {                                    //找寻右边界 
			count++;
			if (j == IMGW - 8) {
				exist_right_edge_point[i] = false;
				break;
			}
			if (this_row[j - 1] != 0 && this_row[j] == 0 && this_row[j+1] == 0 && this_row[j+2] == 0 && this_row[j+3] == 0 && this_row[j+4] == 0 && this_row[j+5] == 0&& this_row[j+6] == 0&& this_row[j+7] == 0) {
				if (j < last_right + 2)
				{//j <= last_right + 2&&(abs(j-last_right)<20    abs(j-last_right) <=  5
				    // bool right_edge = true;

				    // for(int edge_refine = j; edge_refine < j + 10; j++){
					// 	if(this_row[edge_refine] != 0){
					// 		right_edge = false;
					// 		cerr<<"here"<<endl;
					// 		break;
					// 	}
					// }
					// if(right_edge){
					right_edge_point[i] = j;
					exist_right_edge_point[i] = true;
					last_right = j;
					// }
					// else {
				// 	// exist_left_edge_point[i] = false;
				// }
				}
				else {
					exist_right_edge_point[i] = false;
				}
				break;
			}
		}
		for (j = center - 1; j > -1; j--) {                                            //找寻左边界
			count++;
			if (j == 8) {
				exist_left_edge_point[i] = false;
				break;
			}
			if (this_row[j + 1] != 0 && this_row[j] == 0&& this_row[j-1] == 0 && this_row[j-2] == 0 && this_row[j-3] == 0 && this_row[j-4] == 0 && this_row[j-5] == 0&& this_row[j-6] == 0&& this_row[j-7] == 0) {
				if (j > last_left - 2)
				{  //j >= last_left - 2abs(j - last_left) <= 5
				    // bool left_edge = true;

				    // for(int edge_refine = j; edge_refine > j - 10; j--){
					// 	if(this_row[edge_refine] != 0){
					// 		left_edge = false;
					// 		break;
					// 	}
					// }
					// if(left_edge){
					left_edge_point[i] = j;
					exist_left_edge_point[i] = true;
					last_left = j;
				// 	}
				// 	else {
				// 	exist_left_edge_point[i] = false;
				// }
				}
				else {
					exist_left_edge_point[i] = false;
				}
				break;
			}

		}

		
		if (last_left > last_right) {
			center_lost = i;
			exist_left_edge_point[i] = false;
			exist_right_edge_point[i] = false;
			break;
		}
		if (count < 3 * MINX) {    //3 * MINX
			center_lost = i;
			exist_left_edge_point[i] = false;
			exist_right_edge_point[i] = false;
			break;
		}
		if ((exist_left_edge_point[i] && left_edge_point[i] > IMGW - 8) || (exist_right_edge_point[i] && right_edge_point[i] < 8)) {
			center_lost = i;
			exist_left_edge_point[i] = false;
			exist_right_edge_point[i] = false;
			break;
		}
		if (exist_left_edge_point[i]) {
			if (count_white(mat, i,
				(left_edge_point[i] + 5 < IMGW ? left_edge_point[i] + 5 : IMGW), (left_edge_point[i] - 4 > -1 ? left_edge_point[i] - 4 : 0)) == 0) {
				center_lost = i;
				exist_left_edge_point[i] = false;
				exist_right_edge_point[i] = false;
				break;
			}
		}
		if (exist_right_edge_point[i]) {
			if (count_white(mat, i,
				(right_edge_point[i] + 5 < IMGW ? right_edge_point[i] + 5 : IMGW), (right_edge_point[i] - 4 > -1 ? right_edge_point[i] - 4 : 0)) == 0) {
				center_lost = i;
				exist_left_edge_point[i] = false;
				exist_right_edge_point[i] = false;
				break;
			}
		}
		// if(exist_left_edge_point[i] && exist_right_edge_point[i]&& i < IMGH-3){
		// 	int count_zebra = 0;
		// 	for(int zebra_judge = left_edge_point[i]; zebra_judge < right_edge_point[i]; zebra_judge++){
		// 		uchar* zebra_row = mat.ptr(i-5);
		// 		if(zebra_row[zebra_judge]!=zebra_row[zebra_judge+1]){
		// 			count_zebra++;
		// 		}
		// 	}
		// 	if(count_zebra > 8){
		// 		for(int lost = i; lost > i - 5; lost--){
		// 			exist_left_edge_point[lost] = false;
		// 			exist_right_edge_point[lost] = false;	
		// 		center_lost = i - 4;				
		// 		}
		// 	}
		// }
		count = 0;
		center = (last_left + last_right) / 2;
		center_point[i] = center;
	}
	if (exist_left_edge_point[IMGH - MINY] && exist_right_edge_point[IMGH - MINY])
		last_center = (left_edge_point[IMGH - MINY] + right_edge_point[IMGH - MINY]) >> 1;
	else if (exist_left_edge_point[IMGH - MINY])
		last_center = (left_edge_point[IMGH - MINY] + IMGW - 1) >> 1;
	else if (exist_right_edge_point[IMGH - MINY])
		last_center = right_edge_point[IMGH - MINY] >> 1;
	for (i = 0; i < 3; i++)
	{
		exist_left_edge_point[i] = false;
		exist_right_edge_point[i] = false;
	}
	exist_left_edge_point[IMGH - 1] = false;
	exist_right_edge_point[IMGH - 1] = false;
	exist_left_edge_point[IMGH - 2] = false;
	exist_right_edge_point[IMGH - 2] = false;
}


