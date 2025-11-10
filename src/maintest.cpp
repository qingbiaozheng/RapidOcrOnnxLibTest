#include "maintest.h"
#include <stdarg.h> //windows&linux for va_start
#include <OcrLiteCApi.h>
#include <OcrLite.h>
#include <stdexcept>
#include <cstdlib>
#include <opencv2/imgcodecs.hpp>
#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <dirent.h>
#ifdef _WIN32
#include <windows.h>
#endif

void Logger(const char *format, ...) {
    char *buffer = (char *) malloc(8192);
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    printf("%s", buffer);
    free(buffer);
}

void readImgFiles(std::string imageFilePath, std::vector<std::string>& imgFiles){
    DIR *dir;
    dir = opendir(imageFilePath.c_str());
    if(dir == NULL){//single image
        printf("single png_file:%s\n", imageFilePath.c_str());
        imgFiles.emplace_back(imageFilePath);
    }else{
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL){
            if (ent->d_type == DT_LNK || ent->d_type == DT_REG) {
                std::string file_name(ent->d_name);
                //if (file_name.find(".bin") != std::string::npos) {
                if (file_name.find(".jpg") != std::string::npos ||
                    file_name.find(".JPG") != std::string::npos ||
                    file_name.find(".jpeg") != std::string::npos ||
                    file_name.find(".bmp") != std::string::npos ||
                    file_name.find(".png") != std::string::npos ||
                    file_name.find(".jfif") != std::string::npos ||
                    file_name.find(".nv21") != std::string::npos || 
                    file_name.find(".bin") != std::string::npos) {
                    auto image_path = imageFilePath + '/' + ent->d_name;
                    printf("png_file:%s\n", image_path.c_str());
                    imgFiles.emplace_back(image_path);
                    
                }
            }
        }
        sort(imgFiles.begin(), imgFiles.end());
    }
}

int main(int argc, char *argv[]) {

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    //OCR_HANDLE handle = OcrInit(DET_MODEL, CLS_MODEL, REC_MODEL, KEY_FILE, THREAD_NUM);
    OCR_HANDLE handle = OcrInit(DET_MODEL, CLS_MODEL, REC_MODEL, nullptr, THREAD_NUM);
    if (!handle) {
        printf("cannot initialize the OCR Engine.\n");
        return -1;
    }
    OCR_PARAM param = {
        0,
        2000,
        0.3,
        0.5,
        1.6,
        0,  //0-close cls / 1-open
        0};
    //OCR_BOOL bRet = OcrDetect(handle, DEFAULT_IMG_PATH, DEFAULT_IMG_NAME, &param);
    std::vector<std::string> imgFiles;
    std::string imageFilePath = std::string(DEFAULT_IMG_PATH);
    readImgFiles(imageFilePath, imgFiles);

    bool isRecog = false;
    for(size_t index = 0; index < imgFiles.size(); index++){
        //std::string image_path = std::string(DEFAULT_IMG_PATH) + DEFAULT_IMG_NAME;
        std::string image_path = imgFiles[index];
        std::string::size_type iPos = image_path.find_last_of('/') + 1;
        std::string image_name = image_path.substr(iPos, image_path.length() - iPos);
        printf("%s\n", image_path.c_str());
        cv::Mat cvimg = cv::imread(image_path, cv::IMREAD_COLOR);
        if (cvimg.empty()) {
            printf("error: cannot read image");
            return 1;
        }
        //infer
        OCR_INPUT input;
        input.data = cvimg.data;
        input.type = 0;
        input.channels = 3;
        input.width = cvimg.cols;
        input.height = cvimg.rows;
        input.dataLength = input.channels * input.width * input.height;
        OCR_RESULT ocrResult;
        OCR_BOOL bRet = OcrDetectInput(handle, &input, &param, &ocrResult, isRecog);
        printf("OcrDetectInput ret=%d\n", bRet);
        if (bRet) {
            std::cout << "dbNetTime:" << ocrResult.dbNetTime
                    << " detectTime:" << ocrResult.detectTime
                    << " ocrResult.textBlocksLength=" << ocrResult.textBlocksLength << std::endl;
            //rect
            for(size_t i = 0; i < ocrResult.textBlocksLength; i++){
                std::cout << i << " crnnTime:" << ocrResult.textBlocks[i].crnnTime
                            << " blockTime:" << ocrResult.textBlocks[i].blockTime
                            << " angleTime:" << ocrResult.textBlocks[i].angleTime
                            << " -boxPointLength:" << ocrResult.textBlocks[i].boxPointLength 
                            << " angleIndex:" << ocrResult.textBlocks[i].angleIndex
                            << " angleScore:" << ocrResult.textBlocks[i].angleScore
                            << " boxScore:" << ocrResult.textBlocks[i].boxScore << std::endl;
                //box_point
                for(size_t j = 0; j < ocrResult.textBlocks[i].boxPointLength; ++j){
                    std::cout << " (" << ocrResult.textBlocks[i].boxPoint[j].x << "," << ocrResult.textBlocks[i].boxPoint[j].y << ")";
                }
                std::cout << std::endl;
                //char
                for(size_t j = 0; j < ocrResult.textBlocks[i].charScoresLength; ++j){
                    std::cout << " " << ocrResult.textBlocks[i].charScores[j];
                }
                std::cout << std::endl;
                //text
                std::cout << i << "-ocrResult.textBlocks[i].textLength=" << ocrResult.textBlocks[i].textLength << ":";
                std::string strObj(reinterpret_cast<char*>(ocrResult.textBlocks[i].text), ocrResult.textBlocks[i].textLength);
                std::cout <<" " << strObj << std::endl;
            }
        }
    }
    
    if (handle) {
        OcrDestroy(handle);
    }
    return 0;
}


