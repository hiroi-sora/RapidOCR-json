import os
import sys

from RapidOCR_api import OcrAPI

testImgPath = r"D:\Pictures\Screenshots\test.png"
ocrPath = os.getcwd() + "/cpp/build-win-vs2019-x64/Release/RapidOCR-json.exe"
if not os.path.exists(ocrPath):
    print(f"未在以下路径找到引擎！\n{ocrPath}")
    sys.exit()
ocr = OcrAPI(ocrPath)

# 路径识图
print("OCR初始化完毕，开始路径识图。")
res = ocr.run(testImgPath)
ocr.printResult(res)

# base64识图
print("\n\n开始base64识图。")
with open(testImgPath, "rb") as f:  # 获取图片字节流
    imageBytes = f.read()  # 实际使用中，可以联网下载或者截图获取字节流，直接送入OCR，无需保存到本地中转。
res = ocr.runBytes(imageBytes)
ocr.printResult(res)
