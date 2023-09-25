## RapidOCR-json Python API

### 支持传入的任务类型：
- 图片路径
- 图片字节流
- 图片base64字符串

简单示例：
```python
import os
import sys

from RapidOCR_api import OcrAPI

ocrPath = '引擎路径/RapidOCR_json.exe'
ocr = OcrAPI(ocrPath)
res = ocr.run('样例.png')

print('OCR识别结果：\n', res)
ocr.stop()
```

详细示例：
- 见 [demo1.py](demo1.py)