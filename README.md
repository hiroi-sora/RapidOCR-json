# RapidOCR-json 图片转文字程序 v0.1.0

本项目尚属于开发阶段。

这是一个基于 [RapidOcrOnnx](https://github.com/RapidAI/RapidOcrOnnx) 的离线图片OCR文字识别程序。通过管道等方式输入本地图片路径，输出识别结果json字符串。适用于 `Win7 x64` 及以上的系统。

![](/readme_images/img-1.png)

### 写在前面

本项目是 [PaddleOCR-json](https://github.com/hiroi-sora/PaddleOCR-json) 的姐妹项目，都是旨在提供一个对现有OCR引擎（RapidOCR、PaddleOCR）的封装，使得没有C++编程基础的用户也可以用别的语言来简单地调用OCR，享受到更快的运行效率和更便捷的打包&部署手段。

## 差异

RapidOCR-json 与 PaddleOCR-json 无论使用上还是功能上都极为相似，你可以在项目中方便的切换这两个引擎组件。下表说明了它们之间的差异。（“👍”表示此项显著优于对方）

|                    | PaddleOCR-json                           | RapidOCR-json  |
| ------------------ | ---------------------------------------- | -------------- |
| 最低支持系统       | Win10                                    | Win7 👍         |
| mkldnn加速推理     | 支持                                     | 无             |
| 识别时间（相对值） | 100%，快（启用mkldnn加速）👍              | 200%，慢       |
|                    | 400%，极慢（不启用mkldnn）               |                |
| 初始化耗时         | 约2s，慢                                 | 0.1s内，快 👍   |
| 支持语言           | 已整理好`简,繁,英,日,韩,俄,德,法`识别库👍 | 需自己转换模型 |
| 体积（压缩）       | 52MB                                     | 15MB 👍         |
| 体积（部署）       | 250MB                                    | 30MB 👍         |
| 内存占用峰值       | >2000MB（启用mkldnn）                    | ~500MB 👍       |
|                    | ~600MB（不启用mkldnn）                   |                |

- 若追求识别速度优先且硬件性能较强，推荐Paddle。
- 若需多国语言识别，推荐Paddle。
- 若可用内存较少或硬件性能较差，推荐Rapid。
- 若需兼容win7，只能选择Rapid。

## 准备工作

下载 [RapidOCR-json v0.1.0](https://github.com/hiroi-sora/RapidOCR-json/releases/tag/v0.1.0) 并解压，即可。

### 简单试用

方式一：

打开控制台，输入 `path/RapidOCR-json.exe --imagePath=path/test1.png` 。

方式二：

直接双击打开 `RapidOCR_json.exe` 。等程序初始化完毕输出`OCR init completed.`后，直接输入图片路径，回车。

若路径存在中文，则需要通过ASCII转义的json字符串输入。如：

`{"imagePath":"D:/\u6d4b\u8bd5\u56fe\u7247.png"}`


## 指令说明

| 键名称         | 值说明                               | 默认值                                |
| -------------- | ------------------------------------ | ------------------------------------- |
| ensureAscii    | 启用(1)/禁用(0) ASCII转义输出        | 0                                     |
| models         | 模型目录地址，可绝对or相对路径       | "models"                              |
| det            | det库名称                            | "ch_PP-OCRv3_det_infer.onnx"          |
| cls            | cls库名称                            | "ch_ppocr_mobile_v2.0_cls_infer.onnx" |
| rec            | rec库名称                            | "ch_PP-OCRv3_rec_infer.onnx"          |
| keys           | rec字典名称                          | "ppocr_keys_v1.txt"                   |
| doAngle        | 启用(1)/禁用(0) 文字方向检测         | 1                                     |
| mostAngle      | 启用(1)/禁用(0) 角度投票             | 1                                     |
| numThread      | 线程数                               | 4                                     |
| padding        | 预处理白边宽度，可优化窄边图片识别率 | 50                                    |
| maxSideLen     | 图片长边缩小值，可提高大图速度       | 1024                                  |
| boxScoreThresh | 文字框置信度门限值                   | 0.5                                   |
| boxThresh      |                                      | 0.3                                   |
| unClipRatio    | 单个文字框大小倍率                   | 1.6                                   |
| imagePath      | 初始图片路径                         | ""                                    |


例1：（启动时传入图片路径，执行一次识别，然后关闭程序）
```
RapidOCR_json.exe  --imagePath="D:/images/test(1).png"
输出: 识别结果
```

例2：（启动时不传入图片路径，进入无限循环，不断接受图片路径或json输入）
```
RapidOCR_json.exe  --ensureAscii=1
输出: OCR init completed.
D:/images/test(1).png
输出: 识别结果
{"imagePath": "D:/images/test(1).png"}
输出: 识别结果
```

例3：（手动指定参数）
```
RapidOCR_json.exe --doAngle=0 --mostAngle=0 --numThread=12 --padding=100 --imagePath="D:/images/test(1).png"
```


## 返回值说明

通过API调用一次OCR，无论成功与否，都会返回一个字典。

字典中，根含两个元素：状态码`code`和内容`data`。

状态码`code`为整数，每种状态码对应一种情况：

##### `100` 识别到文字

- data内容为数组。数组每一项为字典，含三个元素：
  - `text` ：文本内容，字符串。
  - `box` ：文本包围盒，长度为4的数组，分别为左上角、右上角、右下角、左下角的`[x,y]`。整数。
  - `score` ：识别置信度，浮点数。
- 例：
  ```
    {'code':100,'data':[{'box':[[13,5],[161,5],[161,27],[13,27]],'score':0.9996442794799805,'text':'飞舞的因果交流'}]}
  ```

##### `101` 未识别到文字

- data为字符串：`No text found in image. Path:"图片路径"`
- 例：```{'code':101,'data':'No text found in image. Path: "D:\\空白.png"'}```
- 这是正常现象，识别没有文字的空白图片时会出现这种结果。

##### `200` 图片路径不存在

- data为字符串：`Image path dose not exist. Path:"图片路径".`
- 例：`{'code':200,'data':'Image path dose not exist. Path: "D:\\不存在.png"'}`
- 注意，在系统未开启utf-8支持（`使用 Unicode UTF-8 提供全球语言支持"`）时，不能读入含emoji等特殊字符的路径（如`😀.png`）。但一般的中文及其他 Unicode 字符路径是没问题的，不受系统区域及默认编码影响。

##### `201` 图片路径string无法转换到wstring

- data为字符串：`Image path failed to convert to utf-16 wstring. Path: "图片路径".`
- 使用API时，理论上不会报这个错。
- 开发API时，若传入字符串的编码不合法，有可能报这个错。

##### `202` 图片路径存在，但无法打开文件

- data为字符串：`Image open failed. Path: "图片路径".`
- 可能由系统权限等原因引起。

##### `203` 图片打开成功，但读取到的内容无法被opencv解码

- data为字符串：`Image decode failed. Path: "图片路径".`
- 注意，引擎不以文件后缀来区分各种图片，而是对存在的路径，均读入字节尝试解码。若传入的文件路径不是图片，或图片已损坏，则会报这个错。
- 反之，将正常图片的后缀改为别的（如`.png`改成`.jpg或.exe`），也可以被正常识别。

##### `210` 剪贴板打开失败

- data为字符串：`Clipboard open failed.`
- 可能由别的程序正在占用剪贴板等原因引起。

##### `211` 剪贴板为空

- data为字符串：`Clipboard is empty.`

##### `212` 剪贴板的格式不支持

- data为字符串：`Clipboard format is not valid.`
- 引擎只能识别剪贴板中的位图或文件。若不是这两种格式（如复制了一段文本），则会报这个错。

##### `213` 剪贴板获取内容句柄失败

- data为字符串：`Getting clipboard data handle failed.`
- 可能由别的程序正在占用剪贴板等原因引起。

##### `214` 剪贴板查询到的文件的数量不为1

- data为字符串：`Clipboard number of query files is not valid. Number: 文件数量`
- 只允许一次复制一个文件。一次复制多个文件再调用OCR会得到此报错。

##### `215` 剪贴板检索图形对象信息失败

- data为字符串：`Clipboard get bitmap object failed.`
- 剪贴板中是位图，但获取位图信息失败。可能由别的程序正在占用剪贴板等原因引起。

##### `216` 剪贴板获取位图数据失败

- data为字符串：`Getting clipboard bitmap bits failed.`
- 剪贴板中是位图，获取位图信息成功，但读入缓冲区失败。可能由别的程序正在占用剪贴板等原因引起。

##### `217` 剪贴板中位图的通道数不支持

- data为字符串：`Clipboard number of image channels is not valid. Number: 通道数`
- 引擎只允许读入通道为1（黑白）、3（RGB）、4（RGBA）的图片。位图通道数不是1、3或4，会报这个错。

##### `299` 未知异常

- data为字符串：`An unknown error has occurred.`
- 正常情况下不应该出现此状态码。请提issue。

##### `300` 返回数据无法转换为json字符串

- data为字符串：`JSON dump failed. Coding error.`
- 通过启动参数-image_dir传入非法编码的路径（含中文）时引起。（中文路径应该先启动程序再输入）


## 通过API调用

### 1. Python API

[资源目录](api/python)

使用示例：
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

其他待填坑……


### 项目构建指南

待填坑……

### 感谢

感谢 [RapidAI/RapidOcrOnnx](https://github.com/RapidAI/RapidOcrOnnx) ，没有它就没有本项目。

本项目中使用了 [nlohmann/json](https://github.com/nlohmann/json) ：
> “JSON for Modern C++”

## 更新日志

#### v0.1.0 `2023.4.29` 
