// 代码基于 https://github.com/RapidAI/RapidOcrOnnx
// 二次开发 by https://github.com/hiroi-sora


// 版本信息
#define PROJECT_VER "v1.1.0"
#define PROJECT_NAME "RapidOCR-json " PROJECT_VER

#ifndef __JNI__
#ifndef __CLIB__
#include <cstdio>
#include <iostream>
#include <ctime> // 记录时间戳用 
#include "main.h"
#include "version.h"
#include "OcrLite.h"
#include "OcrUtils.h"
#include "nlohmann_json.hpp"
#include "tools.h"
#include "tools_flags.h" // 标志
#ifdef _WIN32
#include <windows.h>
#endif
using namespace tool;

OcrLite *OCRLiteP = NULL; // 引擎指针 

void printHelp(FILE *out, char *argv0) {
    fprintf(out, " ------- Usage -------\n");
    fprintf(out, "%s\n\n", argv0);
    fprintf(out, " ------- Optional Parameters -------\n");
    fprintf(out, "%s", optionalMsg);
    fprintf(out, " ------- Other Parameters -------\n");
    fprintf(out, "%s", otherMsg);
    fprintf(out, " ------- Examples -------\n");
    fprintf(out, example1Msg, argv0);
    fprintf(out, example2Msg, argv0);
}


// ==================== 执行一次OCR任务 ==================== 
void runOCR(const cv::Mat& mat)
{
    OcrLite &ocrLite = *OCRLiteP; // 获取引擎对象 

    // 执行一次OCR 
    OcrResult result = ocrLite.detect(mat, padding, maxSideLen,
        boxScoreThresh, boxThresh, unClipRatio, doAngle, mostAngle);

    // 输出
    // 3.1. 输出：识别失败
    if (result.dbNetTime == CODE_ERR_MAT_NULL) { // 错误标签
        tool::print_now_fail(); // 输出错误
        return;
    }
    // 3.2. 整理数据
    nlohmann::json outJ;
    outJ["code"] = 100;
    outJ["data"] = nlohmann::json::array();
    bool isEmpty = true; // 记录是否为空结果
    for (const auto& block : result.textBlocks) {
        if (block.text.empty()) // 合法性检查
            continue;
        nlohmann::json j;
        // 记录结果字符串
        j["text"] = block.text; 
        // 记录结果置信度
        double score = 0; // 平均字符置信度
        float charScoreSum = 0.0f;
        for (const auto& s : block.charScores) {
            score += s;
        }
        score /= static_cast<double>(block.charScores.size());
        j["score"] = score;
        // 记录包围盒
        std::vector<cv::Point> b = block.boxPoint;
        j["box"] = { {b[0].x, b[0].y}, {b[1].x, b[1].y},
            {b[2].x, b[2].y }, { b[3].x, b[3].y } };
        outJ["data"].push_back(j);
        isEmpty = false;
    }
    // 3.3. 输出：识别成功，无文字（rec未检出）
    if (isEmpty) {
        print_ocr_fail(CODE_OK_NONE, MSG_OK_NONE);
        return;
    }
    // 3.4. 输出：输出正常情况
    else {
        print_json(outJ);
    }
}

// 路径识图 
void runPath(std::string imgPath) {
    cv::Mat img = tool::imread_utf8(imgPath, cv::IMREAD_COLOR);
    if (img.empty()) {
        tool::print_now_fail();
    }
    else {
        runOCR(img);
    }
}

// base64识图 
void runBase64(std::string imgBase64) {
    cv::Mat img = tool::imread_base64(imgBase64, cv::IMREAD_COLOR);
    if (img.empty()) {
        tool::print_now_fail();
    }
    else {
        runOCR(img);
    }
}

// ==================== 启动OCR循环 ==================== 
void startOCR(std::string imgPath="")
{
    if (!imgPath.empty()) { // 只执行一次 
        runPath(imgPath);
    }
    else { // 重复执行 
        std::string jsonIn;
        while (1) {
            jsonIn = "";
            getline(std::cin, jsonIn);
            int strLen = jsonIn.length();
            // 若为json字符串，则解析 
            if (strLen > 2 && jsonIn[0] == '{' && jsonIn[strLen - 1] == '}') {
                std::string type = tool::load_json_str(jsonIn); // 解析并获取类型
                if (jsonIn.empty()) { // 未解析出图片
                    tool::print_now_fail(); // 输出错误
                    continue;
                }
                if(type == "path")
                    runPath(jsonIn);
                else if (type == "base64")
                    runBase64(jsonIn);
                continue;
            }
            tool::print_now_fail(); // 输出错误
        }
    }
}

int main(int argc, char **argv) {
    // ==================== 载入和解析参数 ==================== 
    //if (argc <= 1) {
    //    printHelp(stderr, argv[0]);
    //    return -1;
    //}
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::string imgPath;
    int opt;
    int optionIndex = 0;
    // ==================== 解析命令行参数 ==================== 
    // 禁用短选项。原："d:1:2:3:4:i:t:p:s:b:o:u:a:A:G:v:h"
    while ((opt = getopt_long(argc, argv, "", long_options, &optionIndex)) != -1) {
        switch (opt) {
            case 'd': // 模型目录地址
                modelsDir = optarg;
                break;
            case '1': // det模型文件名
                modelDetPath = optarg;
                break;
            case '2': // cls模型文件名 
                modelClsPath = optarg;
                break;
            case '3': // rec模型文件名 
                modelRecPath = optarg;
                break;
            case '4': // key字典文件名 
                keysPath = optarg;
                break;
            case 'i': // 初始图片路径 
                imgPath.assign(optarg);
                //imgDir.assign(imgPath.substr(0, imgPath.find_last_of('/') + 1));
                //imgName.assign(imgPath.substr(imgPath.find_last_of('/') + 1));
                break;
            case 't': // 线程数 
                numThread = (int) strtol(optarg, NULL, 10);
                break;
            case 'p': // 在图片外周添加白边的宽度 
                padding = (int) strtol(optarg, NULL, 10);
                break;
            case 's': // 图片长边缩小 
                maxSideLen = (int) strtol(optarg, NULL, 10);
                break;
            case 'b': // 文字框置信度门限 
                boxScoreThresh = strtof(optarg, NULL);
                break;
            case 'o':
                boxThresh = strtof(optarg, NULL);
                break;
            case 'u': // 单个文字框大小倍率 
                unClipRatio = strtof(optarg, NULL);
                break;
            case 'a': // 文字方向检测 
                doAngle = (int)strtol(optarg, NULL, 10)==0 ? false : true;
                break;
            case 'A': // 角度投票 
                mostAngle = (int)strtol(optarg, NULL, 10)==0 ? false : true;
                break;
            case 'v':
                return 0;
            case 'h': // 帮助 
                printHelp(stdout, argv[0]);
                return 0;
            case 'G': // 使用的GPU编号 
                flagGpu = (int)strtol(optarg, NULL, 10);
                break;
            case 'L': // 日志和可视化输出 
                ensureLogger = (int)strtol(optarg, NULL, 10) == 0 ? false : true;
                break;
            case 'S': // ASCII转义  
                ensureAscii = (int)strtol(optarg, NULL, 10) == 0 ? false : true;
                break;
            default:
                printf("other option %c :%s\n", opt, optarg);
        }
    }
    
    // ==================== 检测模型存在 ==================== 

    modelDetPath = modelsDir + "/" + modelDetPath;
    modelClsPath = modelsDir + "/" + modelClsPath;
    modelRecPath = modelsDir + "/" + modelRecPath;
    keysPath     = modelsDir + "/" + keysPath;
    bool hasModelDetFile = isFileExists(modelDetPath);
    if (!hasModelDetFile) {
        fprintf(stderr, "Model det file not found: %s\n", modelDetPath.c_str());
        return -1;
    }
    bool hasModelClsFile = isFileExists(modelClsPath);
    if (!hasModelClsFile) {
        fprintf(stderr, "Model cls file not found: %s\n", modelClsPath.c_str());
        return -1;
    }
    bool hasModelRecFile = isFileExists(modelRecPath);
    if (!hasModelRecFile) {
        fprintf(stderr, "Model rec file not found: %s\n", modelRecPath.c_str());
        return -1;
    }
    bool hasKeysFile = isFileExists(keysPath);
    if (!hasKeysFile) {
        fprintf(stderr, "keys file not found: %s\n", keysPath.c_str());
        return -1;
    }

    // ==================== 初始化OCR ==================== 
    OcrLite ocrLite;
    ocrLite.initLogger( // 设置日志输出
        ensureLogger,//isOutputConsole
        false,//isOutputPartImg
        ensureLogger);//isOutputResultImg
    ocrLite.setNumThread(numThread); // 设置线程 
    ocrLite.setGpuIndex(flagGpu); // 设置启用GPU 
    ocrLite.initModels(modelDetPath, modelClsPath, modelRecPath, keysPath); // 加载模型 
    ocrLite.Logger("=====Input Params=====\n");
    ocrLite.Logger(
            "numThread(%d),padding(%d),maxSideLen(%d),boxScoreThresh(%f),boxThresh(%f),unClipRatio(%f),doAngle(%d),mostAngle(%d),GPU(%d)\n",
            numThread, padding, maxSideLen, boxScoreThresh, boxThresh, unClipRatio, doAngle, mostAngle,
            flagGpu);
    OCRLiteP = &ocrLite;

    std::cout << PROJECT_NAME << std::endl; // 版本提示
    std::cout << "OCR init completed." << std::endl; // 完成提示

    // ==================== 启动OCR ==================== 
    startOCR(imgPath);
    return 0;
}

#endif
#endif