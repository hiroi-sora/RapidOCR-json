#include <string>
#include <iostream>
#include <fstream> // 读文件 
#include <windows.h> // 剪贴板相关
#include <filesystem> // 配置文件存在检查 

#include "tools.h"
#include "tools_flags.h" // 标志
#include "opencv2/imgproc.hpp" // 色彩空间转换
#include "base64.h" // base64库

// 编码转换
#include<codecvt>
std::wstring_convert<std::codecvt_utf8<wchar_t>> conv_Ustr_Wstr; // string utf-8 与 wstring utf-16 的双向转换器

using namespace std;
using namespace nlohmann;

namespace tool {
    // ==================== 引擎参数 ====================
    // 冷参数，启动前设定 
    bool ensureAscii = false; // 输出json是否转ascii 
    bool ensureLogger = false; // 启用日志输出 
    string modelsDir = "models"; // 模型库目录 
    string modelDetPath = "ch_PP-OCRv3_det_infer.onnx"; // 默认检测模型 
    string modelClsPath = "ch_ppocr_mobile_v2.0_cls_infer.onnx"; // 默认方向分类模型 
    string modelRecPath = "ch_PP-OCRv3_rec_infer.onnx"; // 默认识别模型 
    string keysPath = "ppocr_keys_v1.txt"; // 默认字典 
    int numThread = 4; // 线程数 
    int flagGpu = -1; // 使用的GPU编号，不使用为-1 
    // 热参数，启动后可修改 
    int padding = 50; // 预处理白边宽度 
    int maxSideLen = 1024; // 长边缩放至该值 
    float boxScoreThresh = 0.5f; // 文字框置信度门限 
    float boxThresh = 0.3f; 
    float unClipRatio = 1.6f; // 单个文字框大小倍率，越大时单个文字框越大
    bool doAngle = true; // 启用文字方向检测 
    bool mostAngle = true; // 启用角度投票


    // ==================== 执行状态 ====================

    int ToolCode = 0; // 缓存本回合错误码
    string ToolMsg = ""; // 缓存本回合错误提示
    // 获取状态
    void get_state(int& code, string& msg) {
        code = ToolCode;
        msg = ToolMsg;
    }
    // 设置状态
    void set_state(int code, string msg) {
        ToolCode = code;
        ToolMsg = msg;
    }
    // 专门用于消息的wstring转string，转换失败时返回默认提示文字
    string msg_wstr_2_ustr(wstring& msg) {
        try {
            string msgU8 = conv_Ustr_Wstr.to_bytes(msg); // 转回u8
            return msgU8;
        }
        catch (...) {
            return "wstring failed to convert to utf-8 string";
        }
    }
    // 多字节ANSI字符数组转宽字符数组
    wchar_t* char_2_wchar(char* c) {
        setlocale(LC_ALL, ""); // 程序的区域设置为windows系统当前区域
        size_t lenWchar = mbstowcs(NULL, c, 0); // 得到转为宽字符串的长度
        wchar_t* wc = new wchar_t[lenWchar + 1]; // 存放文件名的宽字符串
        int n = mbstowcs(wc, c, lenWchar + 1); // 多字节转宽字符
        setlocale(LC_ALL, "C"); // 还原区域设置为默认
        return wc;
    }

	// ==================== JSON相关 ==================== 

	// 输出一个json 
	void print_json(const json& j) { 
		try {
			cout << j.dump(-1, ' ', ensureAscii) << endl;
		}
		catch (...) {
			json j2;
			j2["code"] = 300;
			j2["data"] = "JSON dump failed. Coding error.";
			cout << j2.dump(-1, ' ', ensureAscii) << endl;
		}
	}

	// 输出错误信息 
	void print_ocr_fail(int code, const string& msg) {
		json j;
		j["code"] = code;
		j["data"] = msg;
		print_json(j);
	}

    // 输出当前错误信息
    void print_now_fail()
    {
        int code;
        std::string msg;
        get_state(code, msg);
        if (code != CODE_INIT) { // 有报告异常
            print_ocr_fail(code, msg); // 输出json
            return;
        }
        print_ocr_fail(CODE_ERR_UNKNOW, MSG_ERR_UNKNOW); // 未知错误
    }

    // {"image_path":"D:\Test\Test.png"}
    // 加载一条json，传入str_in字符串，将str_in更改为json中的路径或base64字符串。返回类型标识。
    std::string load_json_str(string& str_in) {
        set_state(); // 报告状态：初始化
        string origin_str = str_in;
        try {
            auto j = json::parse(str_in); // 转json对象
            for (auto& el : j.items()) { // 遍历键值 
                string value = to_string(el.value());
                int vallen = value.length();
                if (vallen > 2 && value[0] == '\"' && value[vallen - 1] == '\"') {
                    value = value.substr(1, vallen - 2); // 删去nlohmann字符串的两端引号
                }
                if (el.key() == "image_path") { // 图片路径
                    str_in = value;
                    return "path";
                }
                else if (el.key() == "image_base64") { // 图片base64
                    str_in = value;
                    return "base64";
                }
                // 其它参数，暂不考虑
            }
            set_state(CODE_ERR_JSON_NO_IMAGE, MSG_ERR_JSON_NO_IMAGE(origin_str)); // 报告状态：未找到图片路径
            str_in = "";
        }
        catch (...) {
            set_state(CODE_ERR_LOAD_JSON, MSG_ERR_LOAD_JSON(origin_str)); // 报告状态：反序列化JSON失败
            str_in = "";
        }
        return "";
    }



    // ==================== 读图 ====================

    // 检查路径pathW是否为文件，是返回true
    bool is_exists_wstr(wstring pathW) {
        struct _stat buf;
        int result = _wstat((wchar_t*)pathW.c_str(), &buf);
        if (result != 0) { // 发生错误
            return false;
        }
        if (S_IFREG & buf.st_mode) { // 是文件
            return true;
        }
        // else if (S_IFDIR & buf.st_mode) { // 是目录
           //return false;
        // }
        return false;
    }

    // 代替 cv::imread ，从路径pathW读入一张图片。pathW必须为unicode的wstring
    cv::Mat imread_wstr(wstring pathW, int flags = cv::IMREAD_COLOR) {
        string pathU8 = msg_wstr_2_ustr(pathW); // 再转回utf-8，以备输出错误。
        // ↑ 由于这个函数要被剪贴板CF_UNICODETEXT等复用，可能调用方只能提供wstring，所以多此一举转换一次。
        if (!is_exists_wstr(pathW)) { // 路径不存在
            set_state(CODE_ERR_PATH_EXIST, MSG_ERR_PATH_EXIST(pathU8)); // 报告状态：路径不存在且无法输出
            return cv::Mat();
        }
        FILE* fp = _wfopen((wchar_t*)pathW.c_str(), L"rb"); // wpath强制类型转换到whar_t，尝试打开文件
        if (!fp) { // 打开失败
            set_state(CODE_ERR_PATH_READ, MSG_ERR_PATH_READ(pathU8)); // 报告状态：无法读取
            return cv::Mat();
        }
        // 将文件读到内存
        fseek(fp, 0, SEEK_END); // 设置流 fp 的文件位置为 SEEK_END 文件的末尾
        long sz = ftell(fp); // 获取流 fp 的当前文件位置，即总大小（字节）
        char* buf = new char[sz]; // 存放文件字节内容
        fseek(fp, 0, SEEK_SET); // 设置流 fp 的文件位置为 SEEK_SET 文件的开头
        long n = fread(buf, 1, sz, fp); // 从给定流 fp 读取数据到 buf 所指向的数组中，返回成功读取的元素总数
        cv::_InputArray arr(buf, sz); // 转换为OpenCV数组
        cv::Mat img = cv::imdecode(arr, flags); // 解码内存数据，变成cv::Mat数据
        delete[] buf; // 释放buf空间
        fclose(fp); // 关闭文件
        if (!img.data) {
            set_state(CODE_ERR_PATH_DECODE, MSG_ERR_PATH_DECODE(pathU8)); // 报告状态：解码失败
        }
        return img;
    }

    // 从剪贴板读入一张图片。
    cv::Mat imread_clipboard(int flags = cv::IMREAD_COLOR) {
        // 参考文档： https://docs.microsoft.com/zh-cn/windows/win32/dataxchg/using-the-clipboard

        // 尝试打开剪贴板，锁定，防止其他应用程序修改剪贴板内容
        if (!OpenClipboard(NULL)) {
            set_state(CODE_ERR_CLIP_OPEN, MSG_ERR_CLIP_OPEN); // 报告状态：剪贴板打开失败
        }
        else {
            static UINT auPriorityList[] = {  // 允许读入的剪贴板格式：
              CF_BITMAP,                      // 位图
              CF_HDROP,                       // 文件列表句柄（文件管理器选中文件复制）
            };
            int auPriorityLen = sizeof(auPriorityList) / sizeof(auPriorityList[0]); // 列表长度
            int uFormat = GetPriorityClipboardFormat(auPriorityList, auPriorityLen); // 获取当前剪贴板内容的格式
            // 根据格式分配不同任务。
            //     若任务成功，释放全部资源，关闭剪贴板，返回图片mat。
            //     若任务失败，释放已打开的资源和锁，报告状态，跳出switch，统一关闭剪贴板和返回空mat
            switch (uFormat)
            {

            case CF_BITMAP: { // 1. 位图 ===================================================================
                HBITMAP hbm = (HBITMAP)GetClipboardData(uFormat); // 1.1. 从剪贴板中录入指针，得到文件句柄
                if (hbm) {
                    // GlobalLock(hbm); // 返回值总是无效的，读位图似乎不需要锁？
                  // https://social.msdn.microsoft.com/Forums/vstudio/en-US/d2a6aa71-68d7-4db0-8b1f-5d1920f9c4ce/globallock-and-dib-transform-into-hbitmap-issue?forum=vcgeneral
                    BITMAP bmp; // 存放指向缓冲区的指针，缓冲区接收有关指定图形对象的信息
                    GetObject(hbm, sizeof(BITMAP), &bmp); // 1.2. 获取图形对象的信息（不含图片内容本身）
                    if (!hbm) {
                        set_state(CODE_ERR_CLIP_GETOBJ, MSG_ERR_CLIP_GETOBJ); // 报告状态：检索图形对象信息失败
                        break;
                    }
                    int nChannels = bmp.bmBitsPixel == 1 ? 1 : bmp.bmBitsPixel / 8; // 根据色深计算通道数，32bit为4，24bit为3
                    // 1.3. 将句柄hbm中的位图复制到缓冲区
                    long sz = bmp.bmHeight * bmp.bmWidth * nChannels; // 图片大小（字节）
                    cv::Mat mat(cv::Size(bmp.bmWidth, bmp.bmHeight), CV_MAKETYPE(CV_8U, nChannels));  // 创造空矩阵，传入位图大小和深度
                    long getsz = GetBitmapBits(hbm, sz, mat.data); // 将句柄hbm中sz个字节复制到缓冲区img.data
                    if (!getsz) {
                        set_state(CODE_ERR_CLIP_BITMAP, MSG_ERR_CLIP_BITMAP); // 报告状态：获取位图数据失败
                        break;
                    }
                    CloseClipboard();  // 释放资源
                    // 1.4. 返回合适的通道
                    if (mat.data) {
                        if (nChannels == 1 || nChannels == 3) { // 1或3通道，PPOCR可识别，直接返回
                            return mat;
                        }
                        else if (nChannels == 4) { // 4通道，PPOCR不可识别，删去alpha转3通道再返回
                            cv::Mat mat_c3;
                            cv::cvtColor(mat, mat_c3, cv::COLOR_BGRA2BGR); // 色彩空间转换
                            return mat_c3;
                        }
                        set_state(CODE_ERR_CLIP_CHANNEL, MSG_ERR_CLIP_CHANNEL(nChannels)); // 报告状态：通道数异常
                        break;
                    }
                    // 理论上上面 !getsz 已经 break 了，不会走到这里。保险起见再报告一次
                    set_state(CODE_ERR_CLIP_BITMAP, MSG_ERR_CLIP_BITMAP); // 报告状态：获取位图数据失败
                    break;
                }
                set_state(CODE_ERR_CLIP_DATA, MSG_ERR_CLIP_DATA); // 报告状态：获取剪贴板数据失败
                break;
            }

            case CF_HDROP: { // 2. 文件列表句柄 =========================================================== 
                HDROP hClip = (HDROP)GetClipboardData(uFormat); // 2.1. 得到文件列表的句柄
                if (hClip) {
                    // https://docs.microsoft.com/zh-CN/windows/win32/api/shellapi/nf-shellapi-dragqueryfilea
                    GlobalLock(hClip); // 2.2. 锁定全局内存对象
                    int iFiles = DragQueryFile(hClip, 0xFFFFFFFF, NULL, 0); // 2.3. 0xFFFFFFFF表示返回文件列表的计数
                    if (iFiles != 1) { // 只允许1个文件
                        GlobalUnlock(hClip);
                        set_state(CODE_ERR_CLIP_FILES, MSG_ERR_CLIP_FILES(iFiles)); // 报告状态：文件的数量不为1
                        break;
                    }
                    //for (int i = 0; i < iFiles; i++) {
                    int i = 0; // 只取第1个文件
                    UINT lenChar = DragQueryFile(hClip, i, NULL, 0); // 2.4. 得到第i个文件名读入所需缓冲区的大小（字节）
                    char* nameC = new char[lenChar + 1]; // 存放文件名的字节内容
                    DragQueryFileA(hClip, i, nameC, lenChar + 1); // 2.5. 读入第i个文件名
                    wchar_t* nameW = char_2_wchar(nameC); // 2.6. 文件名转为宽字节数组
                    cv::Mat mat = imread_wstr(nameW); // 2.7. 尝试读取文件
                    // 释放资源
                    delete[] nameC;
                    delete[] nameW;
                    GlobalUnlock(hClip); // 2.x.1 释放文件列表句柄
                    CloseClipboard(); // 2.x.2 关闭剪贴板
                    return mat;
                }
                set_state(CODE_ERR_CLIP_DATA, MSG_ERR_CLIP_DATA); // 报告状态：获取剪贴板数据失败
                break;
            }

            case NULL: // 剪贴板为空
                set_state(CODE_ERR_CLIP_EMPTY, MSG_ERR_CLIP_EMPTY); // 报告状态：剪贴板为空
                break;
            case -1: // 其它不支持的格式
            default: // 未知
                set_state(CODE_ERR_CLIP_FORMAT, MSG_ERR_CLIP_FORMAT); // 报告状态： 剪贴板的格式不支持
                break;
            }
            CloseClipboard(); // 为break的情况关闭剪贴板，使其他窗口能够继续访问剪贴板。
        }
        return cv::Mat();
    }

    // 代替 cv::imread ，从路径pathU8读入一张图片。pathU8必须为utf-8的string
    cv::Mat imread_utf8(string pathU8, int flags) {
        set_state(); // 报告状态：初始化
        if (pathU8 == u8"clipboard") { // 若为剪贴板任务
            return imread_clipboard(flags);
        }
        // string u8 转 wchar_t
        std::wstring wpath;
        try {
            wpath = conv_Ustr_Wstr.from_bytes(pathU8); // 利用转换器转换
        }
        catch (...) {
            set_state(CODE_ERR_PATH_CONV, MSG_ERR_PATH_CONV(pathU8)); // 报告状态：转wstring失败
            return cv::Mat();
        }
        return imread_wstr(wpath);
    }

    // 输入base64编码的字符串，返回Mat
    cv::Mat imread_base64(string b64str, int flag) {
        std::string decoded_string;
        try {
            decoded_string = base64_decode(b64str);
        }
        catch (...) {
            set_state(CODE_ERR_BASE64_DECODE, MSG_ERR_BASE64_DECODE); // 报告状态：解析失败 
            return cv::Mat();
        }
        try {
            std::vector<uchar> data(decoded_string.begin(), decoded_string.end());
            cv::Mat img = cv::imdecode(data, flag);
            if (img.empty()) {
                set_state(CODE_ERR_BASE64_IM_DECODE, MSG_ERR_BASE64_IM_DECODE); // 报告状态：转Mat失败 
            }
            return img;
        }
        catch (...) {
            set_state(CODE_ERR_BASE64_IM_DECODE, MSG_ERR_BASE64_IM_DECODE); // 报告状态：转Mat失败 
            return cv::Mat();
        }
    }
}

