# RotateDisp

**RotateDisp** 是一款用于旋转屏幕的软件，他利用了读取串口加速度计传感器（GY-25T提供）感知屏幕方向，进而自动旋转（0°，90°，180°，270°）屏幕显示的功能。

## 使用

软件支持手动设置和自动设置。

当具有传感器并连接正确时，第一次获取到重力方向会自动进行校准，此后在传感器转动，发生重力方向改变时，会同步旋转监视器的显示角度。

手动设置使用了系统固定方向模式，可使用界面中4个按钮进行旋转。下、右、上、左分别对应系统默认显示方向、逆时针旋转90°，逆时针旋转180°，逆时针旋转270°。另外，*Ctrl + <Down/Right/Up/Left>* 快捷键可分别对应上面四种角度旋转。

> 提示：手动设置后会重新校准传感器。

注意：需要根据加速度计坐标轴的方向调整参数。源码中使用了X、Y轴，Z轴由于传感器实际姿态而无法区分重力方向。另外本人使用的加速度计由于X轴不如Y轴准确，故先根据Y轴判断姿态。

软件界面可隐藏至托盘运行。

## 环境

软件依赖 *Qt* 环境。

## 界面

![RotateDisp](https://github.com/doufu3344/RotateDisp/blob/main/RotateDisp.png)

# 视频演示

https://www.bilibili.com/video/BV1D3411A7pN

