import os
import sys

from RapidOCR_api import OcrAPI

ocrPath = os.getcwd()+'\RapidOCR_json.exe'
if not os.path.exists(ocrPath):
    print(f'未在以下路径找到引擎！\n{ocrPath}')
    input()
    sys.exit()
ocr = OcrAPI(ocrPath)
print('OCR初始化完毕，开始识别。')
res = ocr.run('样例.png')
print('OCR识别结果：\n', res)
ocr.stop()
print('\nOCR进程结束。')
input()
