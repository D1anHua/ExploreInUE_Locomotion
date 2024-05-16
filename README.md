# 0 序
本项目是`TalesRunner`子项目, 主要包括我个人目前对于`Unreal CharacterMovementComponent`的扩展, 
以及对`locomotion`动画系统的完善.
故命名为`Explore in Locomotion 及其网络同步`.

**项目配置**:
- UE5.2 (Epic Version)

# 1 简介

本项目的三个主要构成部分如下:
1. `TalesCharacterMovementComponent.h`: 设计并实现Custom Movement Mode

2. `TalesCharacterAnimInstance.h`: 参考`lyra Samples Games`的`Locomotion System`

3. 其他一些小Tips:
   1. 简单的利用`GAS`, 实现`Climb`的进入, 离开, 等等;
   2. `Input Mapping`: 设置不同的运动模式激活不同的`Input Mapping`;
   2. `Enhanced Input:` 通过`Enhanced Input`的`Custom Trigger`可以实现按键长按短按触发不同的`IA_Action`;
   3. `ENhanced Input:` 通过设置联合触发方式, 可以实现按下对于反向键来*Dash*到不同的方向;
   4. 通过`Curve`以及`CameraManager`设定相机移动;
   5. 添加支持网络同步的`Inventory System`(目前有一个不支持网络同步的, 以及粗糙的支持网络同步的系统).

**参考资料**:
   1. [《Exploring in UE4》移动组件详解[原理分析]（2019.7.14更新）](https://zhuanlan.zhihu.com/p/34257208)
   2. [ Networked Movement in the Character Movement Component](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine?application_version=5.0)
   3. [Unreal Engine | Character Movement Component: In-Depth](https://www.youtube.com/playlist?list=PLXJlkahwiwPmeABEhjwIALvxRSZkzoQpk)
   4. [Lyra Starter Game](https://dev.epicgames.com/community/learning/paths/Z4/lyra-starter-game)
   5. [UE5 白话Lyra动画系统](https://zhuanlan.zhihu.com/p/654430436) 
   6. [UE Enhanced Input 如何自定义Trigger](https://zhuanlan.zhihu.com/p/629350225)

# 3 TalesCharacterMovementComponent
关于本`Component`主要有以下5部分:
1. 前提: 拓展`Saved_Move() Struct`!!!重要!!!
2. 添加`CMOVE_Slide`运动模式, 实现`Phys_Slide()`函数, 添加`Compressed_Flag: Slide`标志位设置网络同步(参考`Crouch`)
3. 添加`CMOVE_Prone`运动模式, 实现`Phys_Prone()`函数, 添加`Compressed_Flag: Prone`标志位设置网络同步(参考`Crouch`)
4. 添加`CMOVE_Climb`运动模式, 实现`Phys_Climb()`函数, 没有提供网络同步方式.(默认提供`Replicate: Move Mode`)
5. (后续可能会添加到`GAS`中): 翻越的功能(中高墙的翻越), Dash(多方向Dash)

## 3.1 slide运动模式:
```cpp
//TalesCharacterMovementComponent.h
...
public:
//! @brief Check能否进入SLide
bool CanSlide();
//! @brief 用于进入slide运动模式的函数
void TalesInSlide();
//! @brief 用于退出slide运动模式的函数
void TalseOutSLide();

private:
//! pyhs function
void PhysClimb(float deltaTime, int32 Iterations);
```

同步细节: 利用Compressed_Flag来实现网络同步
```cpp
  enum CompressedFlag
  {
      FLAG_Sprint    = 0x10,
      FLAG_Slide     = 0x20,
      FLAG_Climb     = 0x40,
      FLAG_Custom_3  = 0x80,
  };
```

## 3.2 Prone运动模式:
```cpp
//TalesCharacterMovementComponent.h
...
public:
//! @brief Check能否进入SLide
bool CanProne();
//! @brief 用于进入slide运动模式的函数
void TalesInProne();
//! @brief 用于退出slide运动模式的函数
void TalseOutProne();

private:
//! pyhs function
void PhysProne(float deltaTime, int32 Iterations);
```
同步方式同Slide

## 3.3 Climb运动模式:
```cpp
//TalesCharacterMovementComponent.h
...
public:
//! @brief Check能否从底部进入攀爬状态
bool CanCLimbUp();
//! @brief Check能否从顶部进入攀爬状态
bool CanCLimbDown();

//! @breif 如果需要实现ledge的探测退出, 需要添加该Delegate
FOnArriveTopState  OnArriveTopStateDelegate;

private:
//! pyhs function
void PhysClimb(float deltaTime, int32 Iterations);
```

不提供默认同步方式(项目中作为Ability实现同步)

# 4 Lyra Locomotion System
Lyra Locomotion System主要有以下特点:
1. 采用`MVC`设计模式, 利用`Multi-thread`技术来更新动画蓝图以及解决动画蓝图的解耦问题.(`Character_ABP_Base --- Layer_Interface --- Layer_BaseAnimation`)
2. 利用`Distance Match, Motion Wraping, Pose Warping`实现**8向动画**, **解决滑步问题**, **实现急停转身**等等;
3. 利用`Sync Group`技术, 解决`Animation Sequence`之间更为完善的过渡.
4. `Turn In Place`, `Aim Offset`的实现
