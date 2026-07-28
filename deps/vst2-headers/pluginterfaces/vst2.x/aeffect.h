/*
  ==============================================================================

    aeffect.h — VST 2.x 插件宿主 ABI 净室定义（Steinberg 命名版）

    本文件是 Xaymar/vst2sdk（BSD-3-Clause，净室逆向工程）ABI 定义的
    Steinberg 命名衍生版，供 JUCE 的 VST2 宿主代码编译使用。
    结构体布局 / 枚举数值均逐一对照蓝本 include/vst.h 核实；
    操作码编号另与实战验证过的 VeSTige 系实现交叉确认。

    许可证：BSD-3-Clause（见 deps/vst2-headers/LICENSE）。
    Copyright (c) 2020 Xaymar Dirks <info@xaymar.com>

    注意：本头会被 JUCE 置于 namespace Vst2 内包含，禁止包含任何
    标准库头文件；整型必须使用编译器内建类型定义。

  ==============================================================================
*/

#pragma once

//==============================================================================
// 基础整型（不用 <cstdint>，理由见文件头注释）
#if defined (_MSC_VER)
typedef __int32 VstInt32;
typedef __int64 VstInt64;
 #if defined (_WIN64)
typedef __int64 VstIntPtr;
 #else
typedef __int32 VstIntPtr;
 #endif
#else
typedef int VstInt32;
typedef long long VstInt64;
 #if defined (__x86_64__) || defined (__aarch64__) \
     || (defined (__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 8)
typedef long VstIntPtr;
 #else
typedef int VstIntPtr;
 #endif
#endif

//==============================================================================
// 调用约定：32 位 Windows 为 __cdecl，其余平台默认调用约定
#if defined (_WIN32) && defined (_M_IX86)
 #define VSTCALLBACK __cdecl
#else
 #define VSTCALLBACK
#endif

//==============================================================================
// 四字符常量与插件魔数
#define CCONST(a, b, c, d) ((((VstInt32) (a)) << 24) | (((VstInt32) (b)) << 16) \
                            | (((VstInt32) (c)) << 8) | ((VstInt32) (d)))

const VstInt32 kEffectMagic = CCONST ('V', 's', 't', 'P');

//==============================================================================
struct AEffect;

/** 宿主回调：插件经此向宿主发起请求。 */
typedef VstIntPtr (VSTCALLBACK* audioMasterCallback) (AEffect* effect,
                                                      VstInt32 opcode,
                                                      VstInt32 index,
                                                      VstIntPtr value,
                                                      void* ptr,
                                                      float opt);

//==============================================================================
/** 插件实例对象（ABI 布局固定，逐字段与蓝本 vst_effect_t 对齐）。 */
struct AEffect
{
    VstInt32 magic;             ///< 恒为 kEffectMagic
    VstIntPtr (VSTCALLBACK* dispatcher) (AEffect*, VstInt32, VstInt32, VstIntPtr, void*, float);
    void (VSTCALLBACK* process) (AEffect*, float**, float**, VstInt32);   ///< VST 1.x 遗留累加接口
    void (VSTCALLBACK* setParameter) (AEffect*, VstInt32, float);
    float (VSTCALLBACK* getParameter) (AEffect*, VstInt32);

    VstInt32 numPrograms;
    VstInt32 numParams;
    VstInt32 numInputs;
    VstInt32 numOutputs;
    VstInt32 flags;             ///< AEffectFlags 位组合

    void* resvd1;               ///< 保留，创建时必须为 0
    void* resvd2;               ///< 保留，创建时必须为 0

    VstInt32 initialDelay;      ///< 插件延迟（采样数）
    VstInt32 realQualities;     ///< 实时品质（未使用）
    VstInt32 offQualities;      ///< 离线品质（未使用）
    float    ioRatio;           ///< I/O 比率（未使用）

    void* object;               ///< 插件内部指针
    void* user;                 ///< 宿主内部指针

    VstInt32 uniqueID;          ///< 插件唯一标识（FourCC）
    VstInt32 version;           ///< 插件版本

    void (VSTCALLBACK* processReplacing) (AEffect*, float**, float**, VstInt32);
    void (VSTCALLBACK* processDoubleReplacing) (AEffect*, double**, double**, VstInt32);

    char future[56];            ///< 保留
};

//==============================================================================
/** 宿主发往插件的操作码（dispatcher 的 opcode 参数）。
    数值与蓝本 VST_EFFECT_OPCODE 逐位一致，并与 VeSTige 系交叉确认。 */
enum AEffectOpcodes
{
    effOpen = 0,
    effClose,
    effSetProgram,
    effGetProgram,
    effSetProgramName,
    effGetProgramName,
    effGetParamLabel,
    effGetParamDisplay,
    effGetParamName,
    effGetVu,                   ///< 9：VST 1.x 遗留，已废弃
    effSetSampleRate,
    effSetBlockSize,
    effMainsChanged,
    effEditGetRect,
    effEditOpen,
    effEditClose,
    effEditDraw,                ///< 16：VST 1.x 遗留
    effEditMouse,               ///< 17：VST 1.x 遗留
    effEditKey,                 ///< 18：VST 1.x 遗留
    effEditIdle,
    effEditTop,
    effEditSleep,               ///< 21：VST 1.x 遗留
    effIdentify,                ///< 22：返回 FourCC
    effGetChunk,
    effSetChunk,
    effProcessEvents,           ///< 25：VST 2.x 起
    effCanBeAutomated,
    effString2Parameter,
    effGetNumProgramCategories,
    effGetProgramNameIndexed,
    effCopyProgram,
    effConnectInput,
    effConnectOutput,
    effGetInputProperties,
    effGetOutputProperties,
    effGetPlugCategory,
    effGetCurrentPosition,
    effGetDestinationBuffer,
    effOfflineNotify,
    effOfflinePrepare,
    effOfflineRun,
    effProcessVarIo,
    effSetSpeakerArrangement,
    effSetBlockSizeAndSampleRate,
    effSetBypass,
    effGetEffectName,
    effGetErrorText,
    effGetVendorString,
    effGetProductString,
    effGetVendorVersion,
    effVendorSpecific,
    effCanDo,
    effGetTailSize,
    effIdle,
    effGetIcon,
    effSetViewPosition,
    effGetParameterProperties,
    effKeysRequired,
    effGetVstVersion,
    effEditKeyDown,
    effEditKeyUp,
    effSetEditKnobMode,
    effGetMidiProgramName,
    effGetCurrentMidiProgram,
    effGetMidiProgramCategory,
    effHasMidiProgramsChanged,
    effGetMidiKeyName,
    effBeginSetProgram,
    effEndSetProgram,
    effGetSpeakerArrangement,
    effShellGetNextPlugin,
    effStartProcess,
    effStopProcess,
    effSetTotalSampleToProcess,
    effSetPanLaw,
    effBeginLoadBank,
    effBeginLoadProgram,
    effSetProcessPrecision,
    effGetNumMidiInputChannels,
    effGetNumMidiOutputChannels
};

//==============================================================================
/** AEffect::flags 位标志（数值与蓝本 VST_EFFECT_FLAG 一致）。 */
enum AEffectFlags
{
    effFlagsHasEditor         = 1 << 0,
    effFlagsCanReplacing      = 1 << 4,
    effFlagsProgramChunks     = 1 << 5,
    effFlagsIsSynth           = 1 << 8,
    effFlagsNoSoundInStop     = 1 << 9,
    effFlagsCanDoubleReplacing = 1 << 12
};

//==============================================================================
/** 编辑器窗口矩形（逆时针：上、左、下、右）。 */
struct ERect
{
    short top;
    short left;
    short bottom;
    short right;
};
