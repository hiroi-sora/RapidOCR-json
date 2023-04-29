使用范例

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