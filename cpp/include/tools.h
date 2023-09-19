#ifndef _tools_
#define _tools_
#include "tools_flags.h" // 标志
#include "nlohmann_json.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"

namespace tool {
    // ==================== 参数 ====================
    // 冷参数，启动前设定 
    extern bool ensureAscii; // 输出json是否转ascii 
    extern bool ensureLogger; // 启用日志输出 
    extern std::string modelsDir; // 模型库目录 
    extern std::string modelDetPath; // 默认检测模型 
    extern std::string modelClsPath; // 默认方向分类模型 
    extern std::string modelRecPath; // 默认识别模型 
    extern std::string keysPath; // 默认字典 
    extern int numThread; // 线程数 
    extern int flagGpu; // 使用的GPU编号，不使用为-1 
    // 热参数，启动后可修改 
    extern int padding; // 预处理白边宽度 
    extern int maxSideLen; // 长边缩放至该值 
    extern float boxScoreThresh; // 文字框置信度门限 
    extern float boxThresh;
    extern float unClipRatio; // 单个文字框大小倍率，越大时单个文字框越大
    extern bool doAngle; // 启用文字方向检测 
    extern bool mostAngle; // 启用角度投票
    extern bool isEnsureAsci; // 是否转ascii 

    // ==================== 函数 ====================
    void get_state(int&, std::string&);
    void set_state(int code = CODE_INIT, std::string msg = "");

    void load_congif_file();

	void print_json(const nlohmann::json&);
	void print_ocr_fail(int, const std::string&); 
	void print_now_fail();
	void load_json_str(std::string&);

	cv::Mat imread_utf8(std::string, int flags = cv::IMREAD_COLOR);
}

#endif