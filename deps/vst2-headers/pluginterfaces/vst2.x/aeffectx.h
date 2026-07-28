/*
  ==============================================================================

    aeffectx.h — VST 2.x 扩展 ABI 净室定义（Steinberg 命名版）

    本文件是 Xaymar/vst2sdk（BSD-3-Clause，净室逆向工程）ABI 定义的
    Steinberg 命名衍生版；宿主操作码编号以实战验证过的 VeSTige 系
    实现为准（蓝本在 0x07-0x09 段与线协议有出入，未采用）。

    许可证：BSD-3-Clause（见 deps/vst2-headers/LICENSE）。
    Copyright (c) 2020 Xaymar Dirks <info@xaymar.com>

    注意：本头会被 JUCE 置于 namespace Vst2 内包含，禁止包含任何
    标准库头文件。

  ==============================================================================
*/

#pragma once

#include "aeffect.h"

//==============================================================================
// 字符串缓冲区大小常量（数值与蓝本 VST_BUFFER_SIZE 一致）
enum VstStringConstants
{
    kVstMaxParamStrLen   = 8,   ///< 参数名/标签/显示串
    kVstMaxProgNameLen   = 24,  ///< 程序（预设）名
    kVstMaxEffectNameLen = 32,  ///< 效果名
    kVstMaxVendorStrLen  = 64,  ///< 厂商名
    kVstMaxProductStrLen = 64,  ///< 产品名
    kVstMaxNameLen       = 64,  ///< 通用名称（含 MIDI 键名）
    kVstMaxLabelSize     = 64,  ///< 引脚标签
    kVstMaxShortLabelSize = 8,  ///< 引脚短标签
    kVstMaxCategLabelSize = 24, ///< 引脚类别标签
    kVstMaxSpeakerNameLen = 64  ///< 扬声器名
};

//==============================================================================
/** 插件发往宿主的操作码（audioMasterCallback 的 opcode 参数）。
    编号以 VeSTige 系线协议实现为准。 */
enum AudioMasterOpcodes
{
    audioMasterAutomate = 0,
    audioMasterVersion,
    audioMasterCurrentId,
    audioMasterIdle,
    audioMasterPinConnected,
    audioMasterUnused05,                ///< 5：保留
    audioMasterWantMidi,
    audioMasterGetTime,
    audioMasterProcessEvents,
    audioMasterSetTime,
    audioMasterTempoAt,
    audioMasterGetNumAutomatableParameters,
    audioMasterGetParameterQuantization,
    audioMasterIOChanged,
    audioMasterNeedIdle,
    audioMasterSizeWindow,
    audioMasterGetSampleRate,
    audioMasterGetBlockSize,
    audioMasterGetInputLatency,
    audioMasterGetOutputLatency,
    audioMasterGetPreviousPlug,
    audioMasterGetNextPlug,
    audioMasterWillReplaceOrAccumulate,
    audioMasterGetCurrentProcessLevel,
    audioMasterGetAutomationState,
    audioMasterOfflineStart,
    audioMasterOfflineRead,
    audioMasterOfflineWrite,
    audioMasterOfflineGetCurrentPass,
    audioMasterOfflineGetCurrentMetaPass,
    audioMasterSetOutputSampleRate,
    audioMasterGetOutputSpeakerArrangement, ///< 31：VST 2.4 命名
    audioMasterGetVendorString,
    audioMasterGetProductString,
    audioMasterGetVendorVersion,
    audioMasterVendorSpecific,
    audioMasterSetIcon,
    audioMasterCanDo,
    audioMasterGetLanguage,
    audioMasterOpenWindow,
    audioMasterCloseWindow,
    audioMasterGetDirectory,
    audioMasterUpdateDisplay,
    audioMasterBeginEdit,
    audioMasterEndEdit,
    audioMasterOpenFileSelector,
    audioMasterCloseFileSelector,
    audioMasterEditFile,
    audioMasterGetChunkFile,
    audioMasterGetInputSpeakerArrangement,

    audioMasterGetSpeakerArrangement = audioMasterGetOutputSpeakerArrangement ///< 旧名别名
};

//==============================================================================
// 事件类型
enum VstEventTypes
{
    kVstMidiType = 1,
    kVstSysExType = 6
};

/** MIDI 事件（32 字节，布局与蓝本逐字段对齐）。 */
struct VstMidiEvent
{
    VstInt32 type;              ///< kVstMidiType
    VstInt32 byteSize;          ///< 本结构有效字节数（24）
    VstInt32 deltaFrames;       ///< 相对块起始的采样偏移
    VstInt32 flags;             ///< 未使用
    VstInt32 noteLength;        ///< 音符长度（采样）
    VstInt32 noteOffset;        ///< 音符起始偏移（采样）
    char midiData[4];           ///< MIDI 消息（1-3 字节有效）
    char detune;                ///< 微调（-64..+63）
    char noteOffVelocity;       ///< 音符释放力度
    char reserved1;
    char reserved2;
};

/** MIDI SysEx 事件（头部字段与 VstMidiEvent 同偏移）。 */
struct VstMidiSysexEvent
{
    VstInt32 type;              ///< kVstSysExType
    VstInt32 byteSize;
    VstInt32 deltaFrames;
    VstInt32 flags;
    VstInt32 dumpBytes;         ///< sysexDump 的字节数
    VstIntPtr resvd1;
    char* sysexDump;            ///< SysEx 数据（调用方管理生命周期）
    VstIntPtr resvd2;
};

/** 通用事件（头部与 VstMidiEvent 兼容）。 */
struct VstEvent
{
    VstInt32 type;              ///< VstEventTypes
    VstInt32 byteSize;
    VstInt32 deltaFrames;
    VstInt32 flags;
    char data[16];              ///< 事件负载（按 type 解释）
};

/** 事件块（effProcessEvents / audioMasterProcessEvents 的载体）。 */
struct VstEvents
{
    VstInt32 numEvents;
    VstIntPtr reserved;
    VstEvent* events[2];        ///< 变长数组（协议声明为 2）
};

//==============================================================================
/** 时间信息与走带状态（audioMasterGetTime 返回）。 */
struct VstTimeInfo
{
    double samplePos;           ///< 当前采样位置
    double sampleRate;          ///< 采样率
    double nanoSeconds;         ///< 系统时间（纳秒）
    double ppqPos;              ///< 当前 PPQ 位置
    double tempo;               ///< 当前速度（BPM）
    double barStartPos;         ///< 小节起点（PPQ）
    double cycleStartPos;       ///< 循环起点（PPQ）
    double cycleEndPos;         ///< 循环终点（PPQ）
    VstInt32 timeSigNumerator;  ///< 拍号分子
    VstInt32 timeSigDenominator; ///< 拍号分母
    VstInt32 smpteOffset;       ///< SMPTE 偏移（帧）
    VstInt32 smpteFrameRate;    ///< VstSmpteFrameRate
    VstInt32 samplesToNextClock;///< 距下一 MIDI 时钟的采样数
    VstInt32 flags;             ///< kVst*Valid / kVstTransport* 位组合
};

/** VstTimeInfo::flags 有效性位（数值与 VeSTige 系一致）。 */
enum VstTimeInfoFlags
{
    kVstTransportChanged     = 1 << 0,
    kVstTransportPlaying     = 1 << 1,
    kVstTransportCycleActive = 1 << 2,
    kVstTransportRecording   = 1 << 3,
    kVstAutomationWriting    = 1 << 6,
    kVstAutomationReading    = 1 << 7,
    kVstNanosValid           = 1 << 8,
    kVstPpqPosValid          = 1 << 9,
    kVstTempoValid           = 1 << 10,
    kVstBarsValid            = 1 << 11,
    kVstCyclePosValid        = 1 << 12,
    kVstTimeSigValid         = 1 << 13,
    kVstSmpteValid           = 1 << 14,
    kVstClockValid           = 1 << 15
};

/** SMPTE 帧率（VstTimeInfo::smpteFrameRate）。 */
enum VstSmpteFrameRate
{
    kVstSmpte24fps = 0,
    kVstSmpte25fps,
    kVstSmpte2997fps,
    kVstSmpte30fps,
    kVstSmpte2997dfps,
    kVstSmpte30dfps,
    kVstSmpteFilm16mm,
    kVstSmpteFilm35mm,
    kVstSmpte239fps,
    kVstSmpte249fps,
    kVstSmpte599fps,
    kVstSmpte60fps
};

//==============================================================================
/** 输入/输出引脚属性（effGetInputProperties / effGetOutputProperties）。 */
struct VstPinProperties
{
    char label[kVstMaxLabelSize];
    VstInt32 flags;             ///< kVstPinIsStereo 等
    VstInt32 arrangementType;   ///< kSpeakerArr* 之一
    char shortLabel[kVstMaxShortLabelSize];
    char categoryLabel[kVstMaxCategLabelSize];
};

enum VstPinPropertiesFlags
{
    kVstPinIsStereo   = 1 << 0,
    kVstPinIsValid    = 1 << 1,
    kVstPinUseSpeaker = 1 << 2
};

//==============================================================================
/** 参数属性（effGetParameterProperties）。 */
struct VstParameterProperties
{
    float stepFloat;
    float smallStepFloat;
    float largeStepFloat;
    char label[kVstMaxLabelSize];
    VstInt32 flags;             ///< VstParameterFlags
    VstInt32 minInteger;
    VstInt32 maxInteger;
    VstInt32 stepInteger;
    VstInt32 largeStepInteger;
    char shortLabel[kVstMaxShortLabelSize];
};

enum VstParameterFlags
{
    kVstParameterIsSwitch                = 1 << 0,
    kVstParameterUsesIntegerMinMax       = 1 << 1,
    kVstParameterUsesFloatStep           = 1 << 2,
    kVstParameterUsesIntStep             = 1 << 3,
    kVstParameterSupportsDisplayIndex    = 1 << 4,
    kVstParameterSupportsDisplayCategory = 1 << 5,
    kVstParameterCanRamp                 = 1 << 6
};

//==============================================================================
/** 插件类别（effGetPlugCategory 返回值）。
    数值与蓝本 VST_EFFECT_CATEGORY 逐位一致。 */
enum VstPlugCategory
{
    kPlugCategUnknown = 0,
    kPlugCategEffect,
    kPlugCategSynth,
    kPlugCategAnalysis,
    kPlugCategMastering,
    kPlugCategSpacializer,
    kPlugCategRoomFx,
    kPlugSurroundFx,
    kPlugCategRestoration,
    kPlugCategOfflineProcess,
    kPlugCategShell,
    kPlugCategGenerator,
    kPlugCategMaxCount
};

//==============================================================================
/** 单个扬声器的属性。 */
struct VstSpeakerProperties
{
    float azimuth;              ///< 方位角（弧度，-PI..PI）
    float elevation;            ///< 仰角
    float radius;               ///< 距离
    float reserved;
    char name[kVstMaxSpeakerNameLen];
    VstInt32 type;              ///< VstSpeakerTypes 之一（JUCE 会写入）
    char future[28];
};

/** 扬声器布局（effGet/SetSpeakerArrangement）。 */
struct VstSpeakerArrangement
{
    VstInt32 type;              ///< kSpeakerArr* 之一
    VstInt32 numChannels;       ///< speakers 中有效条目数
    VstSpeakerProperties speakers[8]; ///< 协议固定为 8
};

/** 布局类型（-2..28；关键值与蓝本交叉确认：4.0=11、5.0=14、5.1=15、7.1=23）。 */
enum VstSpeakerArrangementType
{
    kSpeakerArrUserDefined = -2,
    kSpeakerArrEmpty = -1,
    kSpeakerArrMono = 0,
    kSpeakerArrStereo,
    kSpeakerArrStereoSurround,
    kSpeakerArrStereoCenter,
    kSpeakerArrStereoSide,
    kSpeakerArrStereoCLfe,
    kSpeakerArr30Cine,
    kSpeakerArr30Music,
    kSpeakerArr31Cine,
    kSpeakerArr31Music,
    kSpeakerArr40Cine,
    kSpeakerArr40Music,
    kSpeakerArr41Cine,
    kSpeakerArr41Music,
    kSpeakerArr50,
    kSpeakerArr51,
    kSpeakerArr60Cine,
    kSpeakerArr60Music,
    kSpeakerArr61Cine,
    kSpeakerArr61Music,
    kSpeakerArr70Cine,
    kSpeakerArr70Music,
    kSpeakerArr71Cine,
    kSpeakerArr71Music,
    kSpeakerArr80Cine,
    kSpeakerArr80Music,
    kSpeakerArr81Cine,
    kSpeakerArr81Music,
    kSpeakerArr102
};

/** 扬声器类型（VstSpeakerProperties 的类型标识，顺序值 0..18）。 */
enum VstSpeakerTypes
{
    kSpeakerL = 0,
    kSpeakerR,
    kSpeakerC,
    kSpeakerLfe,
    kSpeakerLs,
    kSpeakerRs,
    kSpeakerLc,
    kSpeakerRc,
    kSpeakerS,
    kSpeakerCs = kSpeakerS,
    kSpeakerSl,
    kSpeakerSr,
    kSpeakerTm,
    kSpeakerTfl,
    kSpeakerTfc,
    kSpeakerTfr,
    kSpeakerTrl,
    kSpeakerTrc,
    kSpeakerTrr,
    kSpeakerLfe2
};

//==============================================================================
/** MIDI 键名查询（effGetMidiKeyName）。SDK 原始命名无前缀。 */
struct MidiKeyName
{
    VstInt32 thisProgramIndex;  ///< 输入：程序索引
    VstInt32 thisKeyNumber;     ///< 输入：键号（0-127）
    char keyName[kVstMaxNameLen]; ///< 输出：键名
};

//==============================================================================
/** 处理精度（effSetProcessPrecision）。 */
enum VstProcessPrecision
{
    kVstProcessPrecision32 = 0,
    kVstProcessPrecision64 = 1
};

/** 宿主语言（audioMasterGetLanguage 返回值，仅含 JUCE 用到的英语）。 */
enum VstHostLanguage
{
    kVstLangEnglish = 1
};
