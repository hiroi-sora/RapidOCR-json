import os
import atexit  # 退出处理
import threading
import subprocess  # 进程，管道
from psutil import Process as psutilProcess  # 内存监控
from sys import platform as sysPlatform  # popen静默模式
from json import loads as jsonLoads, dumps as jsonDumps
from base64 import b64encode # base64 编码

InitTimeout = 15  # 初始化超时时间，秒


class OcrAPI:
    """调用OCR"""

    def __init__(self, exePath, argsStr=""):
        """初始化识别器。\n
        :exePath: 识别器`PapidOCR_json.exe`的路径。\n
        :argsStr: 启动参数，字符串。
        """
        cwd = os.path.abspath(os.path.join(exePath, os.pardir))  # 获取exe父文件夹
        # 处理启动参数
        args = ' '
        if argsStr:  # 添加用户指定的启动参数
            args += f' {argsStr}'
        if 'ensureAscii' not in args:
            args += f' --ensureAscii=1'
        # 设置子进程启用静默模式，不显示控制台窗口
        startupinfo = None
        if 'win32' in str(sysPlatform).lower():
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.dwFlags = subprocess.CREATE_NEW_CONSOLE | subprocess.STARTF_USESHOWWINDOW
            startupinfo.wShowWindow = subprocess.SW_HIDE
        self.ret = subprocess.Popen(  # 打开管道
            exePath+args, cwd=cwd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            startupinfo=startupinfo  # 开启静默模式
        )
        atexit.register(self.stop)  # 注册程序终止时执行强制停止子进程
        self.psutilProcess = psutilProcess(self.ret.pid)  # 进程监控对象

        self.initErrorMsg = f'OCR init fail.\n引擎路径：{exePath}\n启动参数：{args}'

        # 子线程检查超时
        def cancelTimeout():
            checkTimer.cancel()

        def checkTimeout():
            self.initErrorMsg = f'OCR init timeout: {InitTimeout}s.\n{exePath}'
            self.ret.kill()  # 关闭子进程
        checkTimer = threading.Timer(InitTimeout, checkTimeout)
        checkTimer.start()

        # 循环读取，检查成功标志
        while True:
            if not self.ret.poll() == None:  # 子进程已退出，初始化失败
                cancelTimeout()
                raise Exception(self.initErrorMsg)
            # 必须按行读，所以不能使用communicate()来规避超时问题
            initStr = self.ret.stdout.readline().decode('ascii', errors='ignore')
            if 'OCR init completed.' in initStr:  # 初始化成功
                break
        cancelTimeout()

    def runDict(self, writeDict: dict):
        """传入指令字典，发送给引擎进程。\n
        `writeDict`: 指令字典。\n
        `return`:  {"code": 识别码, "data": 内容列表或错误信息字符串}\n"""
        if not self.ret.poll() == None:
            return {'code': 400, 'data': f'子进程已结束。'}
        try:  # 输入地址转为ascii转义的json字符串，规避编码问题
            writeStr = jsonDumps(writeDict, ensure_ascii=True, indent=None)+"\n"
        except Exception as e:
            return {'code': 403, 'data': f'输入字典转json失败。字典：{writeDict} || 报错：[{e}]'}
        # 输入路径
        try:
            self.ret.stdin.write(writeStr.encode('ascii'))
            self.ret.stdin.flush()
        except Exception as e:
            return {'code': 400, 'data': f'向识别器进程写入图片地址失败，疑似子进程已崩溃。{e}'}
        # 获取返回值
        try:
            getStr = self.ret.stdout.readline().decode('utf-8', errors='ignore')
        except Exception as e:
            return {'code': 401, 'data': f'读取识别器进程输出值失败，疑似传入了不存在或无法识别的图片。{e}'}
        try:
            return jsonLoads(getStr)
        except Exception as e:
            return {'code': 402, 'data': f'识别器输出值反序列化JSON失败，疑似传入了不存在或无法识别的图片。异常信息：{e}。原始内容：{getStr}'}

    def run(self, imgPath: str):
        """对一张本地图片进行文字识别。\n
        `exePath`: 图片路径。\n
        `return`:  {"code": 识别码, "data": 内容列表或错误信息字符串}\n"""
        writeDict = {"image_path": imgPath}
        return self.runDict(writeDict)
    
    def runBase64(self, imageBase64: str):
        """对一张编码为base64字符串的图片进行文字识别。\n
        `imageBase64`: 图片base64字符串。\n
        `return`:  {"code": 识别码, "data": 内容列表或错误信息字符串}\n"""
        writeDict = {"image_base64": imageBase64}
        return self.runDict(writeDict)
    
    def runBytes(self, imageBytes):
        """对一张图片的字节流信息进行文字识别。\n
        `imageBytes`: 图片字节流。\n
        `return`:  {"code": 识别码, "data": 内容列表或错误信息字符串}\n"""
        imageBase64 = b64encode(imageBytes).decode('utf-8')
        return self.runBase64(imageBase64)
    
    def stop(self):
        self.ret.kill()  # 关闭子进程。误重复调用似乎不会有坏的影响

    @staticmethod
    def printResult(res: dict):
        """用于调试，格式化打印识别结果。\n
        `res`: OCR识别结果。"""

        # 识别成功
        if res["code"] == 100:
            index = 1
            for line in res["data"]:
                print(f"{index}-置信度：{round(line['score'], 2)}，文本：{line['text']}")
                index+=1
        elif res["code"] == 100:
            print("图片中未识别出文字。")
        else:
            print(f"图片识别失败。错误码：{res['code']}，错误信息：{res['data']}")

    def __del__(self):
        self.stop()
        atexit.unregister(self.stop)  # 移除退出处理
