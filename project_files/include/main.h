#ifndef __MAIN_H__
#define __MAIN_H__

#include "getopt.h"

// 参数列表 
static const struct option long_options[] = {
         // 冷参数，启动前设定 
        {"ensureAscii",    required_argument, NULL, 'S'}, // 启用(1)/禁用(0) ASCII转义输出 
        {"ensureLogger",   required_argument, NULL, 'L'}, // 启用(1)/禁用(0) 日志和可视化输出 
        {"models",         required_argument, NULL, 'd'}, // 模型目录地址，可绝对or相对路径 
        {"det",            required_argument, NULL, '1'}, // det模型文件名 
        {"cls",            required_argument, NULL, '2'}, // cls模型文件名 
        {"rec",            required_argument, NULL, '3'}, // rec模型文件名 
        {"keys",           required_argument, NULL, '4'}, // key字典文件名 
        {"numThread",      required_argument, NULL, 't'}, // 线程数 
        {"GPU",            required_argument, NULL, 'G'}, // 使用的GPU编号，不使用为-1 
        // 热参数，启动后可修改 
        {"imagePath",      required_argument, NULL, 'i'}, // 初始图片路径 
        {"padding",        required_argument, NULL, 'p'}, // 在图片外周添加白边的宽度 
        {"maxSideLen",     required_argument, NULL, 's'}, // 图片长边缩小 
        {"boxScoreThresh", required_argument, NULL, 'b'}, // 文字框置信度门限 
        {"boxThresh",      required_argument, NULL, 'o'},
        {"unClipRatio",    required_argument, NULL, 'u'}, // 单个文字框大小倍率，越大时单个文字框越大。此项与图片的大小相关，越大的图片此值应该越大。
        {"doAngle",        required_argument, NULL, 'a'}, // 启用(1)/禁用(0) 文字方向检测 
        {"mostAngle",      required_argument, NULL, 'A'}, // 启用(1)/禁用(0) 角度投票(整张图片以最大可能文字方向来识别)，当禁用文字方向检测时，此项也不起作用。
        {"version",        no_argument,       NULL, 'v'},
        {"help",           no_argument,       NULL, 'h'}, // 打印帮助 
        {"loopCount",      required_argument, NULL, 'l'},
        {NULL,             no_argument,       NULL, 0}
};


const char *optionalMsg = "--models: models directory.\n" \
                          "--det: model file name of det.\n" \
                          "--cls: model file name of cls.\n" \
                          "--rec: model file name of rec.\n" \
                          "--keys: keys file name.\n" \
                          "--imagePath: If filled, use this path for a single OCR.If left blank, enter OCR work loop.\n"\
                          "--ensureAscii: Enable(1)/Disable(0) output character escaping according to ASCII code.\n"\
                          "--ensureLogger: Enable(1)/Disable(0) logging output and visualization output of image results..\n"\
                          "--numThread: value of numThread(int), default: 4\n" \
                          "--padding: value of padding(int), default: 50\n" \
                          "--maxSideLen: Long side of picture for resize(int), default: 1024\n" \
                          "--boxScoreThresh: value of boxScoreThresh(float), default: 0.5\n" \
                          "--boxThresh: value of boxThresh(float), default: 0.3\n" \
                          "--unClipRatio: value of unClipRatio(float), default: 1.6\n" \
                          "--doAngle: Enable(1)/Disable(0) Angle Net, default: Enable\n" \
                          "--mostAngle: Enable(1)/Disable(0) Most Possible AngleIndex, default: Enable\n" \
                          "--GPU: Disable(-1)/GPU0(0)/GPU1(1)/... Use Vulkan GPU accelerate, default: Disable(-1)\n\n";

const char *otherMsg = "--version: show version\n" \
                       "--help: print this help\n\n";

const char *example1Msg = "Example1: %s --imagePath=\"D:/images/test(1).png\"\n\n";
const char *example2Msg = "Example2: %s --models=models --det=ch_PP-OCRv3_det_infer.onnx --cls=ch_ppocr_mobile_v2.0_cls_infer.onnx --rec=ch_PP-OCRv3_rec_infer.onnx --keys=ppocr_keys_v1.txt --ensureAscii=1 --ensureLogger=1 --numThread=8 --padding=50 --maxSideLen=1024 --boxScoreThresh=0.5 --boxThresh=0.3 --unClipRatio=1.6 --doAngle=1 --mostAngle=1 --GPU=-1\n\n";

#endif //__MAIN_H__
