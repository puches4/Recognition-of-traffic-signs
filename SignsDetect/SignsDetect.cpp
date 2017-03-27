// SignsDetect.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"

// OpenCV
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

 // C++
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <windows.h>
#include <string>



cv::Mat detectAndDisplay(cv::Mat frame, std::vector<cv::Rect> signsRed, std::vector<cv::Rect> signsBlue);
std::vector<std::string> scandir(std::string p);
std::vector<cv::Rect> ColorThreshold(cv::Mat frame, std::string color);
//void ColorThresholdTest(cv::Mat frame);

cv::CascadeClassifier cascadeProhibitory, cascadeDanger, cascadeYield, cascadeMandatory;
std::string imageName;

/*void on_low_h_thresh_trackbar(int, void *);
void on_high_h_thresh_trackbar(int, void *);
void on_low_s_thresh_trackbar(int, void *);
void on_high_s_thresh_trackbar(int, void *);
void on_low_v_thresh_trackbar(int, void *);
void on_high_v_thresh_trackbar(int, void *);
int low_h = 0, low_s = 0, low_v = 0;
int high_h = 179, high_s = 255, high_v = 255;*/

/** @function main */
int main(void)
{
	cv::Mat frame;
	std::vector<std::string> images = scandir("..\\..\\SignsDetect\\TestImages\\*");
	for (size_t i = 0; i < images.size(); i++)
	{
		imageName = "..\\..\\SignsDetect\\TestImages\\" + images[i];
		std::ifstream file(imageName);
		if (file.is_open()) //Проверка на наличие файла
		{
			file.close();
			frame = cv::imread(imageName, CV_LOAD_IMAGE_COLOR);
			//ColorThresholdTest(frame);
			//imshow(window_name, frameThreshold);

			//Осуществляем бинаризацию по цветовым порогам
			std::vector<cv::Rect> signsRed = ColorThreshold(frame, "red");
			std::vector<cv::Rect> signsBlue = ColorThreshold(frame, "blue");

			//Загружаем классификаторы и осуществляем поиск в найденных на предыдущем этапе областях
			if (!cascadeProhibitory.load("..\\..\\SignsDetect\\prohibitory.xml")) { printf("--(!)Error loading cascade\n"); return -1; };
			if (!cascadeDanger.load("..\\..\\SignsDetect\\danger3.xml")) { printf("--(!)Error loading cascade\n"); return -1; };
			if (!cascadeYield.load("..\\..\\SignsDetect\\yield3.xml")) { printf("--(!)Error loading cascade\n"); return -1; };
			if (!cascadeMandatory.load("..\\..\\SignsDetect\\mandatory.xml")) { printf("--(!)Error loading cascade\n"); return -1; };
			frame = detectAndDisplay(frame, signsRed, signsBlue); 

			//ОТображаем результат
			//cv::imwrite("..\\..\\SignsDetect\\Results\\" + images[i], frame);
			cvNamedWindow("Detected Signs", CV_WINDOW_NORMAL);
			cv::resizeWindow("Detected Signs", 1020, 600);
			imshow("Detected Signs", frame);
			int c = cv::waitKey();

			if ((char)c == 27) { break; } // escape
		}
		else
		{
			printf("--(!)Error loading image\n"); return -1;
		}
			
	}
	return 0;
}

/** @Осуществляем бинаризацию изображения по выбранному цвету (красному или синему) */
std::vector<cv::Rect> ColorThreshold(cv::Mat frame, std::string color)
{
	cv::Mat frameThreshold, frameThreshold1, frameThreshold2, frameHSV;
	cv::cvtColor(frame, frameHSV, CV_BGR2HSV);
	/*cvNamedWindow("Original Image", CV_WINDOW_NORMAL);
	cv::resizeWindow("Original Image", 640, 640);
	imshow("Original Image", frame);*/
	
	if (color == "red")
	{
		//В HSV для красного цвета экспериментально получены координаты HUE: (0 - 10) U (150 - 179)
		inRange(frameHSV, cv::Scalar(140, 30, 5), cv::Scalar(179, 255, 255), frameThreshold1);
		inRange(frameHSV, cv::Scalar(0, 50, 5), cv::Scalar(12, 255, 255), frameThreshold2);
		cv::bitwise_or(frameThreshold1, frameThreshold2, frameThreshold);
	}
	else
		if (color == "blue")
		inRange(frameHSV, cv::Scalar(103, 100, 5), cv::Scalar(130, 255, 255), frameThreshold);
		else
		{
			printf("--(!)Error, can't get color threshold\n");
		}
	

	//морфологическое открытие (убираем маленькие объекты)
	cv::erode(frameThreshold, frameThreshold, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
	cv::dilate(frameThreshold, frameThreshold, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));

	//морфологическое закрытие (заделываем маленькие дырки)
	cv::dilate(frameThreshold, frameThreshold, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
	cv::erode(frameThreshold, frameThreshold, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));

	/*cv::Mat frameThreshold, frameGray;
	cv::cvtColor(frame, frameGray, CV_BGR2GRAY);
	cv::blur(frameGray, frameGray, cv::Size(3,3));
	cv::Canny(frameGray, frameThreshold, 50, 120, 3);*/

	//Находим связные контура, выделяем их прямоугольниками и оставляем только достаточно большие прямоугольники, 
	//близкие к квадрату
	std::vector<std::vector<cv::Point> > contours; 
	std::vector<cv::Rect> signs; 
	cv::findContours(frameThreshold, contours, cv::RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	for (size_t i = 0; i < contours.size(); ++i)
	{
		cv::Rect r = cv::boundingRect(contours[i]); 
		if ((r.height / (float)r.width > 0.7) && (r.height / (float)r.width < 1.3) && (r.width > 20) && (r.height > 20))
			signs.push_back(r);
	}

	for (size_t i = 0; i < signs.size(); i++)
	{
		rectangle(frameThreshold, signs[i], cv::Scalar(255, 0, 255), 2, 8, 0);
	}
	/*cvNamedWindow("Object Detection", CV_WINDOW_NORMAL);
	cv::resizeWindow("Object Detection", 640, 640);
	imshow("Object Detection", frameThreshold);*/

	return signs;
}

/** @Расширяем rectangle на заданное значение */
std::vector<cv::Rect> ExpandRectangle(cv::Mat frame, std::vector<cv::Rect> r, float coef)
{
	for (size_t i = 0; i < r.size(); i++)
	{
		cv::Size deltaSize(r[i].width * coef, r[i].height * coef);
		cv::Point offset(deltaSize.width / 2, deltaSize.height / 2);
		r[i] += deltaSize;
		r[i] -= offset;
		if (r[i].x < 0) r[i].x = 0;
		if (r[i].x + r[i].width > frame.cols) r[i].width = frame.cols - r[i].x;
		if (r[i].y < 0) r[i].y = 0;
		if (r[i].y + r[i].height > frame.rows) r[i].height = frame.rows - r[i].y;
	}
	return r;
}

/** @К требуемым прямоугольным областям применяем поиск каскадным классификатором */
std::vector<cv::Rect> FindHaar(cv::Mat frame_gray, std::vector<cv::Rect> signs, cv::CascadeClassifier cascade)
{
	std::vector<cv::Rect> haarSigns, findSigns;
	for (size_t i = 0; i < signs.size(); i++)
	{
		cv::Mat signsROI = frame_gray(signs[i]);
		cascade.detectMultiScale(signsROI, haarSigns, 1.1, 2, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(20, 20));
		for (size_t j = 0; j < haarSigns.size(); j++)
		{
			cv::Rect r;
			r.x = signs[i].x + haarSigns[j].x;
			r.y = signs[i].y + haarSigns[j].y;
			r.width = haarSigns[j].width;
			r.height = haarSigns[j].height;
			findSigns.push_back(r);
		}
	}
	return findSigns;
}

/** @Нарисовать прямоугольники на заданном изображении в заданных вектором областях и заданным цветом */
void DrawRectangle(cv::Mat image, std::vector<cv::Rect> rectangles, cv::Vec3b color)
{
	for (size_t i = 0; i < rectangles.size(); i++)
	{
		rectangle(image, rectangles[i], color, 2, 8, 0);
	}
}

/** @Применяем  к ранее найденным контурам поиск четырьмя разными каскадными классификаторами */
cv::Mat detectAndDisplay(cv::Mat frame, std::vector<cv::Rect> signsRed, std::vector<cv::Rect> signsBlue)
{
	
	cv::Mat frame_gray;
	cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
	equalizeHist(frame_gray, frame_gray);

	//Расширяем область для поиска на 40%
	signsRed = ExpandRectangle(frame, signsRed, 0.4);
	signsBlue = ExpandRectangle(frame, signsBlue, 0.4);
	
	//Ищем знаки в расширенных областях
	std::vector<cv::Rect> signsProhibitory = FindHaar(frame_gray, signsRed, cascadeProhibitory);
	DrawRectangle(frame, signsProhibitory, cv::Vec3b(0, 0, 255));

	std::vector<cv::Rect> signsDanger = FindHaar(frame_gray, signsRed, cascadeDanger);
	DrawRectangle(frame, signsDanger, cv::Vec3b(0, 255, 255));

	std::vector<cv::Rect> signsYield = FindHaar(frame_gray, signsRed, cascadeYield);
	DrawRectangle(frame, signsYield, cv::Vec3b(255, 0, 255));

	std::vector<cv::Rect> signsMandatory = FindHaar(frame_gray, signsBlue, cascadeMandatory);
	DrawRectangle(frame, signsMandatory, cv::Vec3b(255, 0, 0));

	return frame;
}

/** @Функция для тестирования различных порогов цветовой бинаризации */
/*void ColorThresholdTest(cv::Mat frame)
{
	cv::Mat frameThreshold, frameHSV;
	cv::cvtColor(frame, frameHSV, CV_BGR2HSV);

	cv::createTrackbar("Low R", "Object Detection", &low_h, 255, on_low_h_thresh_trackbar);
	cv::createTrackbar("High R", "Object Detection", &high_h, 255, on_high_h_thresh_trackbar);
	cv::createTrackbar("Low G", "Object Detection", &low_s, 255, on_low_s_thresh_trackbar);
	cv::createTrackbar("High G", "Object Detection", &high_s, 255, on_high_s_thresh_trackbar);
	cv::createTrackbar("Low B", "Object Detection", &low_v, 255, on_low_v_thresh_trackbar);
	cv::createTrackbar("High B", "Object Detection", &high_v, 255, on_high_v_thresh_trackbar);

	cvNamedWindow("Video Capture", CV_WINDOW_NORMAL);
	cv::resizeWindow("Video Capture", 640, 640);
	imshow("Video Capture", frame);
	//frame.convertTo(frameTrashold, -1, (double)50 / 10.0, (double)50 / 10.0);
	while ((char)cv::waitKey(1) != 'q')
	{
		inRange(frameHSV, cv::Scalar(low_h, low_s, low_v), cv::Scalar(high_h, high_s, high_v), frameThreshold);
		cvNamedWindow("Object Detection", CV_WINDOW_NORMAL);
		cv::resizeWindow("Object Detection", 640, 640);
		imshow("Object Detection", frameThreshold);
	}
}

void on_low_h_thresh_trackbar(int, void *)
{
	low_h = min(high_h - 1, low_h);
	cv::setTrackbarPos("Low h", "Object Detection", low_h);
}
void on_high_h_thresh_trackbar(int, void *)
{
	high_h = max(high_h, low_h + 1);
	cv::setTrackbarPos("High h", "Object Detection", high_h);
}
void on_low_s_thresh_trackbar(int, void *)
{
	low_s = min(high_s - 1, low_s);
	cv::setTrackbarPos("Low s", "Object Detection", low_s);
}
void on_high_s_thresh_trackbar(int, void *)
{
	high_s = max(high_s, low_s + 1);
	cv::setTrackbarPos("High s", "Object Detection", high_s);
}
void on_low_v_thresh_trackbar(int, void *)
{
	low_v = min(high_v - 1, low_v);
	cv::setTrackbarPos("Low v", "Object Detection", low_v);
}
void on_high_v_thresh_trackbar(int, void *)
{
	high_v = max(high_v, low_v + 1);
	cv::setTrackbarPos("High v", "Object Detection", high_v);
}*/



std::wstring string_to_wstring(std::string& s)
{
	std::ofstream ofs("temp.txt", std::ofstream::out);
	if (!ofs) { std::cerr << "don't open file: 'temp.txt1' " << std::endl; exit(1); }
	ofs << s;
	ofs.close();

	std::wstring s1;
	std::wifstream wifs("temp.txt", std::wifstream::in);
	if (!wifs) { std::cerr << "don't open file: 'temp.txt2'" << std::endl; exit(1); }
	wifs >> s1;
	wifs.close();
	return s1;
}

std::string wstring_to_string(std::wstring s)
{
	std::wofstream wofs("temp.txt", std::wofstream::out);
	if (!wofs) { std::cerr << "don't open file: 'temp.txt3'" << std::endl; exit(1); }
	wofs << s;
	wofs.close();

	std::string s1;
	std::ifstream ifs("temp.txt", std::ifstream::in);
	if (!ifs) { std::cerr << "don't open file: 'temp.txt3'" << std::endl; exit(1); }
	ifs >> s1;
	ifs.close();
	return s1;
}
/** @Получение списка файлов из директории */
std::vector<std::string> scandir(std::string p)
{
	std::wstring Path = string_to_wstring(p);

	WIN32_FIND_DATA FindFileData;
	HANDLE hf;

	hf = FindFirstFile((string_to_wstring(p)).c_str(), &FindFileData);

	std::vector<std::string> v;
	if (hf != INVALID_HANDLE_VALUE)
	{
		do
		{
			std::wcout << "FindFileData.dwFileAttributes= " << FindFileData.dwFileAttributes << std::endl;
			if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY)
				v.push_back(wstring_to_string(FindFileData.cFileName));
		} while (FindNextFile(hf, &FindFileData) != 0);
	}

	return v;
}