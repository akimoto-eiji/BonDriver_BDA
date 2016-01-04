//------------------------------------------------------------------------------
// File: BonTuner.h
//   Define CBonTuner class
//------------------------------------------------------------------------------
#pragma once

#include <Windows.h>
#include <stdio.h>

#include <list>
#include <vector>
#include <map>

#include "IBonDriver2.h"
#include "IBdaSpecials2.h"

#include "LockChannel.h"

#include <iostream>
#include <dshow.h>

#include <tuner.h>

#include "common.h"

// transform()
#include <algorithm>

using namespace std;

class CTsWriter;

// CBonTuner class
////////////////////////////////
class CBonTuner : public IBonDriver2
{
public:
	////////////////////////////////////////
	// コンストラクタ & デストラクタ
	////////////////////////////////////////
	CBonTuner();
	virtual ~CBonTuner();

	////////////////////////////////////////
	// IBonDriver メンバ関数
	////////////////////////////////////////
	const BOOL OpenTuner(void);
	void CloseTuner(void);

	const BOOL SetChannel(const BYTE byCh);
	const float GetSignalLevel(void);

	const DWORD WaitTsStream(const DWORD dwTimeOut = 0);
	const DWORD GetReadyCount(void);

	const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain);
	const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain);

	void PurgeTsStream(void);

	////////////////////////////////////////
	// IBonDriver2 メンバ関数
	////////////////////////////////////////
	LPCTSTR GetTunerName(void);

	const BOOL IsTunerOpening(void);

	LPCTSTR EnumTuningSpace(const DWORD dwSpace);
	LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel);

	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel);

	const DWORD GetCurSpace(void);
	const DWORD GetCurChannel(void);

	void Release(void);
	
	////////////////////////////////////////
	// 静的メンバ変数
	////////////////////////////////////////

	// Dllのモジュールハンドル
	static HMODULE st_hModule;

	// 作成されたCBontunerインスタンスの一覧
	static list<CBonTuner*> st_InstanceList;

	// st_InstanceList操作用
	static CRITICAL_SECTION st_LockInstanceList;

protected:
	////////////////////////////////////////
	// 内部メンバ関数
	////////////////////////////////////////

	// COM処理専用スレッド
	static DWORD WINAPI COMProcThread(LPVOID lpParameter);

	// Decode処理専用スレッド
	static DWORD WINAPI DecodeProcThread(LPVOID lpParameter);

	// TsWriter コールバック関数
	static int CALLBACK RecvProc(void* pParam, BYTE* pbData, DWORD dwSize);

	// データ受信スタート・停止
	void StartRecv(void);
	void StopRecv(void);

	// ini ファイル読込
	void ReadIniFile(void);

	// 信号状態を取得
	void GetSignalState(int* pnStrength, int* pnQuality, int* pnLock);

	// チャンネル切替
	BOOL LockChannel(const TuningParam *pTuningParam, BOOL bLockTwice);

	// チューナ固有Dllのロード
	HRESULT CheckAndInitTunerDependDll(void);

	// チューナ固有Dllでのキャプチャデバイス確認
	HRESULT CheckCapture(wstring displayName, wstring friendlyName);
		
	// チューナ固有関数のロード
	void LoadTunerDependCode(void);

	// チューナ固有関数とDllの解放
	void ReleaseTunerDependCode(void);

	// GraphBuilder
	HRESULT InitializeGraphBuilder(void);
	void CleanupGraph(void);
	HRESULT RunGraph(void);
	void StopGraph(void);

	// TuningSpace
	HRESULT CreateTuningSpace(void);
	void UnloadTuningSpace(void);
	HRESULT InitTuningSpace(void);

	// NetworkProvider
	HRESULT LoadNetworkProvider(void);
	void UnloadNetworkProvider(void);

	// TunerDevice
	HRESULT LoadAndConnectTunerDevice(void);
	void UnloadTunerDevice(void);
	
	// CaptureDevice
	HRESULT CBonTuner::LoadAndConnectCaptureDevice(wstring searchGuid, wstring searchFrienlyName);
	void UnloadCaptureDevice(void);
	
	// TsWriter
	HRESULT LoadAndConnectTsWriter(void);
	void UnloadTsWriter(void);

	// Demultiplexer
	HRESULT LoadAndConnectDemux(void);
	void UnloadDemux(void);
	
	// TIF (Transport Information Filter)
	HRESULT LoadAndConnectTif(void);
	void UnloadTif(void);

	// TsWriter/Demultiplexer/TIFをLoad&ConnectしRunする
	HRESULT CBonTuner::LoadAndConnectMiscFilters(void);

	// チューナ信号状態取得用インターフェース
	HRESULT LoadTunerSignalStatistics(void);
	void UnloadTunerSignalStatistics(void);

	// Pin の接続
	HRESULT Connect(const WCHAR* pszName, IBaseFilter* pFrom, IBaseFilter* pTo);

	// 全ての Pin を切断する
	void DisconnectAll(IBaseFilter* pFilter);

	// CCOM処理専用スレッドから呼び出される関数
	const BOOL _OpenTuner(void);
	void _CloseTuner(void);
	const BOOL _SetChannel(const DWORD dwSpace, const DWORD dwChannel);
	const float _GetSignalLevel(void);
	const BOOL _IsTunerOpening(void);
	const DWORD _GetCurSpace(void);
	const DWORD _GetCurChannel(void);


protected:
	////////////////////////////////////////
	// メンバ変数
	////////////////////////////////////////

	////////////////////////////////////////
	// COM処理専用スレッド用
	////////////////////////////////////////

	enum enumCOMRequest {
		eCOMReqOpenTuner = 1,
		eCOMReqCloseTuner,
		eCOMReqSetChannel,
		eCOMReqGetSignalLevel,
		eCOMReqIsTunerOpening,
		eCOMReqGetCurSpace,
		eCOMReqGetCurChannel,
	};

	struct COMReqParamSetChannel {
		DWORD dwSpace;
		DWORD dwChannel;
	};

	union COMReqParm {
		COMReqParamSetChannel SetChannel;
	};

	union COMReqRetVal {
		BOOL OpenTuner;
		BOOL SetChannel;
		float GetSignalLevel;
		BOOL IsTunerOpening;
		DWORD GetCurSpace;
		DWORD GetCurChannel;
	};

	struct COMProc {
		HANDLE hThread;					// スレッドハンドル
		HANDLE hReqEvent;				// COMProcスレッドへのコマンド実行要求
		HANDLE hEndEvent;				// COMProcスレッドからのコマンド完了通知
		CRITICAL_SECTION csLock;		// 排他用
		enumCOMRequest nRequest;		// リクエスト
		COMReqParm uParam;				// パラメータ
		COMReqRetVal uRetVal;			// 戻り値
		DWORD dwTick;					// 現在のTickCount
		DWORD dwTickLastCheck;			// 最後に異常監視の確認を行ったTickCount
		DWORD dwTickSignalLockErr;		// SignalLockの異常発生TickCount
		DWORD dwTickBitRateErr;			// BitRateの異常発生TckCount
		BOOL bSignalLockErr;			// SignalLockの異常発生中Flag
		BOOL bBitRateErr;				// BitRateの異常発生中Flag
		BOOL bDoReLockChannel;			// チャンネルロック再実行中
		BOOL bDoReOpenTuner;			// チューナー再オープン中
		unsigned int nReLockFailCount;	// Re-LockChannel失敗回数
		DWORD dwReOpenSpace;			// チューナー再オープン時のカレントチューニングスペース番号退避
		DWORD dwReOpenChannel;			// チューナー再オープン時のカレントチャンネル番号退避
		HANDLE hTerminateRequest;		// スレッド終了要求
		
		COMProc(void)
			: hThread(NULL),
			  hReqEvent(NULL),
			  hEndEvent(NULL),
			  hTerminateRequest(NULL),
			  dwTick(0),
			  dwTickLastCheck(0),
			  dwTickSignalLockErr(0),
			  dwTickBitRateErr(0),
			  bSignalLockErr(FALSE),
			  bBitRateErr(FALSE),
			  bDoReLockChannel(FALSE),
			  bDoReOpenTuner(FALSE),
			  dwReOpenSpace(CBonTuner::SPACE_INVALID),
			  dwReOpenChannel(CBonTuner::CHANNEL_INVALID),
			  nReLockFailCount(0)
		{
			hReqEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
			hEndEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
			hTerminateRequest = ::CreateEvent(NULL, FALSE, FALSE, NULL);
			::InitializeCriticalSection(&csLock);
		};
		
		~COMProc(void)
		{
			::CloseHandle(hReqEvent);
			hReqEvent = NULL;
			::CloseHandle(hEndEvent);
			hEndEvent = NULL;
			::CloseHandle(hTerminateRequest);
			hTerminateRequest = NULL;
			::DeleteCriticalSection(&csLock);
		};
		
		inline BOOL CheckTick(void)
		{
			dwTick = ::GetTickCount();
			if (dwTick - dwTickLastCheck > 1000) {
				dwTickLastCheck = dwTick;
				return TRUE;
			}
			return FALSE;
		}
		
		inline void ResetWatchDog(void)
		{
			bSignalLockErr = FALSE;
			bBitRateErr = FALSE;
		}

		inline BOOL CheckSignalLockErr(BOOL state, DWORD threshold)
		{
			if (state) {
				//正常
				bSignalLockErr = FALSE;
			} else {
				// 異常
				if (!bSignalLockErr) {
					// 今回発生
					bSignalLockErr = TRUE;
					dwTickSignalLockErr = dwTick;
				}
				else {
					// 前回以前に発生していた
					if ((dwTick - dwTickSignalLockErr) > threshold) {
						// 設定時間以上経過している
						ResetWatchDog();
						return TRUE;
					}
				}
			}
			return FALSE;
		}
		
		inline BOOL CheckBitRateErr(BOOL state, DWORD threshold)
		{
			if (state) {
				//正常
				bSignalLockErr = FALSE;
			}
			else {
				// 異常
				if (!bBitRateErr) {
					// 今回発生
					bBitRateErr = TRUE;
					dwTickBitRateErr = dwTick;
				}
				else {
					// 前回以前に発生していた
					if ((dwTick - dwTickBitRateErr) > threshold) {
						// 設定時間以上経過している
						ResetWatchDog();
						return TRUE;
					}
				}
			}
			return FALSE;
		}

		inline void SetReLockChannel(void)
		{
			bDoReLockChannel = TRUE;
			nReLockFailCount = 0;
		}
		
		inline void ResetReLockChannel(void)
		{
			bDoReLockChannel = FALSE;
			nReLockFailCount = 0;
		}
		
		inline BOOL CheckReLockFailCount(unsigned int threshold)
		{
			return (++nReLockFailCount >= threshold);
		}
		
		inline void SetReOpenTuner(DWORD space, DWORD channel)
		{
			bDoReOpenTuner = TRUE;
			dwReOpenSpace = space;
			dwReOpenChannel = channel;
		}
		
		inline void ClearReOpenChannel(void)
		{
			dwReOpenSpace = CBonTuner::SPACE_INVALID;
			dwReOpenChannel = CBonTuner::CHANNEL_INVALID;
		}
		
		inline BOOL CheckReOpenChannel(void)
		{
			return (dwReOpenSpace != CBonTuner::SPACE_INVALID && dwReOpenChannel != CBonTuner::CHANNEL_INVALID);
		}

		inline void ResetReOpenTuner(void)
		{
			bDoReOpenTuner = FALSE;
			ClearReOpenChannel();
		}
	};
	COMProc m_aCOMProc;

	////////////////////////////////////////
	// Decode処理専用スレッド用
	////////////////////////////////////////

	struct DecodeProc {
		HANDLE hThread;					// スレッドハンドル
		HANDLE hTerminateRequest;		// スレッド終了要求
		DecodeProc(void)
			: hThread(NULL),
			hTerminateRequest(NULL)
		{
			hTerminateRequest = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		};
		~DecodeProc(void)
		{
			::CloseHandle(hTerminateRequest);
			hTerminateRequest = NULL;
		};
	};
	DecodeProc m_aDecodeProc;

	////////////////////////////////////////
	// チューナパラメータ関係
	////////////////////////////////////////

	// INIファイルで指定できるGUID/FriendlyName最大数
	static const unsigned int MAX_GUID = 100;

	// チューナ・キャプチャ検索に使用するGUID文字列とFriendlyName文字列の組合せ
	struct TunerSearchData {
		wstring TunerGUID;
		wstring TunerFriendlyName;
		wstring CaptureGUID;
		wstring CaptureFriendlyName;
		TunerSearchData(void)
		{
		};
		TunerSearchData(WCHAR* tunerGuid, WCHAR* tunerFriendlyName, WCHAR* captureGuid, WCHAR* captureFriendlyName)
			: TunerGUID(tunerGuid),
			TunerFriendlyName(tunerFriendlyName),
			CaptureGUID(captureGuid),
			CaptureFriendlyName(captureFriendlyName)
		{
			::transform(TunerGUID.begin(), TunerGUID.end(), TunerGUID.begin(), towlower);
			::transform(CaptureGUID.begin(), CaptureGUID.end(), CaptureGUID.begin(), towlower);
		};
	};

	// INI ファイルで指定するチューナパラメータ
	struct TunerParam {
		map<unsigned int, TunerSearchData*> Tuner;
												// TunerとCaptureのGUID/FriendlyName指定
		BOOL bNotExistCaptureDevice;			// TunerデバイスのみでCaptureデバイスが存在しない場合TRUE
		BOOL bCheckDeviceInstancePath;			// TunerとCaptureのデバイスインスタンスパスが一致しているかの確認を行うかどうか
#ifdef UNICODE
		wstring sTunerName;						// GetTunerNameで返す名前
#else
		string sTunerName;						// GetTunerNameで返す名前
#endif
		wstring sDLLBaseName;					// 固有DLL
		TunerParam(void)
			: bNotExistCaptureDevice(TRUE),
			  bCheckDeviceInstancePath(TRUE)
		{
		};
		~TunerParam(void)
		{
			for (map<unsigned int, TunerSearchData*>::iterator it = Tuner.begin(); it != Tuner.end(); it++) {
				SAFE_DELETE(it->second);
			}
			Tuner.clear();
		};
	};
	TunerParam m_aTunerParam;

	// Tone信号切替時のWait時間
	unsigned int m_nToneWait;

	// CH切替後のLock確認時間
	unsigned int m_nLockWait;

	// CH切替後のLock確認Delay時間
	unsigned int m_nLockWaitDelay;

	// CH切替後のLock確認Retry回数
	unsigned int m_nLockWaitRetry;

	// CH切替動作を強制的に2度行うかどうか
	BOOL m_bLockTwice;

	// CH切替動作を強制的に2度行う場合のDelay時間(msec)
	unsigned int m_nLockTwiceDelay;

	// SignalLockedの監視時間(msec) 0で監視しない
	unsigned int m_nWatchDogSignalLocked;

	// BitRateの監視時間(msec) 0で監視しない
	unsigned int m_nWatchDogBitRate;

	// 異常検知時、チューナの再オープンを試みるまでのCH切替動作試行回数
	unsigned int m_nReOpenWhenGiveUpReLock;

	// SignalLevel 算出方法
	// 0 .. Strength値 / StrengthCoefficient
	// 1 .. Quality値 / QualityCoefficient
	// 2 .. (Strength値 / StrengthCoefficient) * (Quality値 / QualityCoefficient)
	unsigned int m_nSignalLevelCalcType;

	// Strength 値補正係数
	float m_fStrengthCoefficient;

	// Quality 値補正係数
	float m_fQualityCoefficient;

	////////////////////////////////////////
	// BonDriver パラメータ関係
	////////////////////////////////////////

	// バッファ1個あたりのサイズ
	DWORD m_dwBuffSize;

	// 最大バッファ数
	DWORD m_dwMaxBuffCount;

	// m_hOnDecodeEventをセットするデータバッファ個数
	unsigned int m_nWaitTsCount;

	// WaitTsStreamで最低限待機する時間
	unsigned int m_nWaitTsSleep;

	////////////////////////////////////////
	// チャンネルパラメータ
	////////////////////////////////////////

	// チャンネルデータ
	struct ChData {
#ifdef UNICODE
		wstring sServiceName;
#else
		string sServiceName;
#endif
		unsigned int Satellite;			// 衛星受信設定番号
		unsigned int Polarisation;		// 偏波種類番号 (0 .. 未指定, 1 .. H, 2 .. V, 3 .. L, 4 .. R)
		unsigned int ModulationType;	// 変調方式設定番号
		long Frequency;					// 周波数(KHz)
		long SID;						// サービスID
		long TSID;						// トランスポートストリームID
		long ONID;						// オリジナルネットワークID
		BOOL LockTwiceTarget;			// CH切替動作を強制的に2度行う対象
		ChData(void)
			: Satellite(0),
			  Polarisation(0),
			  ModulationType(0),
			  Frequency(0),
			  SID(-1),
			  TSID(-1),
			  ONID(-1),
			  LockTwiceTarget(FALSE)
		{
		};
	};

	// チューニング空間データ
	struct TuningSpaceData {
#ifdef UNICODE
		wstring sTuningSpaceName;		// EnumTuningSpaceで返すTuning Space名
#else
		string sTuningSpaceName;		// EnumTuningSpaceで返すTuning Space名
#endif
		map<unsigned int, ChData*> Channels;		// チャンネル番号とチャンネルデータ
		DWORD dwNumChannel;				// チャンネル数
		TuningSpaceData(void)
			: dwNumChannel(0)
		{
		};
		~TuningSpaceData(void)
		{
			for (map<unsigned int, ChData*>::iterator it = Channels.begin(); it != Channels.end(); it++) {
				SAFE_DELETE(it->second);
			}
			Channels.clear();
		};
	};

	// チューニングスペース一覧
	struct TuningData {
		map<unsigned int, TuningSpaceData*> Spaces;	// チューニングスペース番号とデータ
		DWORD dwNumSpace;					// チューニングスペース数
		TuningData(void)
			: dwNumSpace(0)
		{
		};
		~TuningData(void)
		{
			for (map<unsigned int, TuningSpaceData*>::iterator it = Spaces.begin(); it != Spaces.end(); it++) {
				SAFE_DELETE(it->second);
			}
			Spaces.clear();
		};
	};
	TuningData m_TuningData;

	// iniファイルからCH設定を読込む際に
	// 使用されていないCH番号があっても前詰せず確保しておくかどうか
	// FALSE .. 使用されてない番号があった場合前詰し連続させる
	// TRUE .. 使用されていない番号をそのまま空CHとして確保しておく
	BOOL m_bReserveUnusedCh;

	////////////////////////////////////////
	// 衛星受信パラメータ
	////////////////////////////////////////

	// iniファイルで受付ける偏波種類数
	static const unsigned int POLARISATION_SIZE = 5;

	// CBonTunerで使用する偏波種類番号とPolarisation型のMapping
	static const Polarisation PolarisationMapping[POLARISATION_SIZE];

	// 偏波種類毎のiniファイルでの記号
	static const WCHAR PolarisationChar[POLARISATION_SIZE];

	// iniファイルで設定できる最大衛星数 + 1
	static const unsigned int MAX_SATELLITE = 5;

	// 衛星受信設定データ
	struct Satellite {
		AntennaParam Polarisation[POLARISATION_SIZE];	// 偏波種類毎のアンテナ設定
	};
	Satellite m_aSatellite[MAX_SATELLITE];

	// チャンネル名の自動生成に使用する衛星の名称
	wstring m_sSatelliteName[MAX_SATELLITE];

	////////////////////////////////////////
	// 変調方式パラメータ
	////////////////////////////////////////

	// iniファイルで設定できる最大変調方式数
	static const unsigned int MAX_MODULATION = 4;

	// 変調方式設定データ
	ModulationMethod m_aModulationType[MAX_MODULATION];

	// チャンネル名の自動生成に使用する変調方式の名称
	wstring m_sModulationName[MAX_MODULATION];

	////////////////////////////////////////
	// BonDriver 関連
	////////////////////////////////////////

	// iniファイルのPath
	WCHAR m_szIniFilePath[_MAX_PATH + 1];

	// TSバッファ操作用
	CRITICAL_SECTION m_csTSBuff;

	// Decode処理後TSバッファ操作用
	CRITICAL_SECTION m_csDecodedTSBuff;

	// 受信イベント
	HANDLE m_hOnStreamEvent;

	// デコードイベント
	HANDLE m_hOnDecodeEvent;

	// 受信TSデータバッファ
	struct TS_DATA{
		BYTE* pbyBuff;
		DWORD dwSize;
		TS_DATA(void)
			: pbyBuff(NULL),
			  dwSize(0)
		{
		};
		~TS_DATA(void) {
			SAFE_DELETE_ARRAY(pbyBuff);
		};
	};
	vector<TS_DATA*> m_TsBuff;

	// Decode処理の終わったTSデータバッファ
	vector<TS_DATA*> m_DecodedTsBuff;

	// GetTsStreamで参照されるバッファ
	TS_DATA* m_LastBuff;

	// バッファ作成用テンポラリバッファ
	BYTE *m_pbyRecvBuff;

	// テンポラリバッファの位置
	DWORD m_dwBuffOffset;

	// データ受信中
	BOOL m_bRecvStarted;

	// ビットレート計算用
	class BitRate {
	private:
		DWORD Rate1sec;					// 1秒間のレート加算用 (bytes/sec)
		DWORD RateLast[5];				// 直近5秒間のレート (bytes/sec)
		DWORD DataCount;				// 直近5秒間のデータ個数 (0〜5)
		float Rate;						// 平均ビットレート (Mibps)
		DWORD LastTick;					// 前回のTickCount値
		CRITICAL_SECTION csRate1Sec;	// nRate1sec 排他用
		CRITICAL_SECTION csRateLast;	// nRateLast 排他用

	public:
		BitRate(void)
			: Rate1sec(0),
			  RateLast(),
			  DataCount(0),
			  Rate(0.0F)
		{
			::InitializeCriticalSection(&csRate1Sec);
			::InitializeCriticalSection(&csRateLast);
			LastTick = ::GetTickCount();
		};

		~BitRate(void)
		{
			::DeleteCriticalSection(&csRateLast);
			::DeleteCriticalSection(&csRate1Sec);
		}

		inline void AddRate(DWORD Count)
		{
			::EnterCriticalSection(&csRate1Sec);
			Rate1sec += Count;
			::LeaveCriticalSection(&csRate1Sec);
		}

		inline DWORD CheckRate(void)
		{
			DWORD total = 0;
			DWORD Tick = ::GetTickCount();
			if (Tick - LastTick > 1000) {
				::EnterCriticalSection(&csRateLast);
				for (unsigned int i = (sizeof(RateLast) / sizeof(RateLast[0])) - 1; i > 0; i--) {
					RateLast[i] = RateLast[i - 1];
					total += RateLast[i];
				}
				::EnterCriticalSection(&csRate1Sec);
				RateLast[0] = Rate1sec;
				Rate1sec = 0;
				::LeaveCriticalSection(&csRate1Sec);
				total += RateLast[0];
				if (DataCount < 5)
					DataCount++;
				if (DataCount)
					Rate = ((float)total / (float)DataCount) / 131072.0F;
				LastTick = Tick;
				::LeaveCriticalSection(&csRateLast);
			}
			DWORD remain = 1000 - (Tick - LastTick);
			return (remain > 1000) ? 1000 : remain;
		}

		inline void Clear(void)
		{
			::EnterCriticalSection(&csRateLast);
			::EnterCriticalSection(&csRate1Sec);
			Rate1sec = 0;
			for (unsigned int i = 0; i < sizeof(RateLast) / sizeof(RateLast[0]); i++) {
				RateLast[i] = 0;
			}
			DataCount = 0;
			Rate = 0.0F;
			LastTick = ::GetTickCount();
			::LeaveCriticalSection(&csRate1Sec);
			::LeaveCriticalSection(&csRateLast);
		}

		inline float GetRate(void)
		{
			return Rate;
		}
	};
	BitRate m_BitRate;

	////////////////////////////////////////
	// チューナ関連
	////////////////////////////////////////

	// チューナデバイス排他処理用
	HANDLE m_hSemaphore;

	// Graph
	ITuningSpace *m_pITuningSpace;
	IBaseFilter *m_pNetworkProvider;
	IBaseFilter *m_pTunerDevice;
	IBaseFilter *m_pCaptureDevice;
	IBaseFilter *m_pTsWriter;
	IBaseFilter *m_pDemux;
	IBaseFilter *m_pTif;
	IGraphBuilder *m_pIGraphBuilder;
	IMediaControl *m_pIMediaControl;
	CTsWriter *m_pCTsWriter;

	// チューナ信号状態取得用インターフェース
	IBDA_SignalStatistics *m_pIBDA_SignalStatistics;

	// チューナーのGUIDとFriendlyName
	wstring m_sTunerDisplayName;
	wstring m_sTunerFriendlyName;

	// チューナーの使用するTuningSpace/NetworkProvider等の種類
	enum enumTunerType {
		eTunerTypeDVBS = 1,
		eTunerTypeDVBT = 2,
	};
	enumTunerType m_nDVBSystemType;

	// 衛星受信パラメータ/変調方式パラメータのデフォルト値 1 .. SPHD, 2 .. BS/CS110, 3 .. UHF/CATV
	DWORD m_nDefaultNetwork;

	// Tuner is opened
	BOOL m_bOpened;

	// カレントチューニングスペース番号
	DWORD m_dwCurSpace;

	// チューニングスペース番号不明
	static const DWORD SPACE_INVALID = 0xFFFFFFFF;

	// カレントチャンネル番号
	DWORD m_dwCurChannel;

	// チャンネル番号不明
	static const DWORD CHANNEL_INVALID = 0xFFFFFFFF;

	// 現在のトーン切替状態
	long m_nCurTone; // current tone signal state

	// トーン切替状態不明
	static const long TONE_UNKNOWN = -1;

	// 最後にLockChannelを行った時のチューニングパラメータ
	TuningParam m_LastTuningParam;

	// TunerSpecial DLL module handle
	HMODULE m_hModuleTunerSpecials;

	// チューナ固有関数 IBdaSpecials
	IBdaSpecials *m_pIBdaSpecials;
	IBdaSpecials2a *m_pIBdaSpecials2;

	// チューナ固有の関数が必要かどうかを自動判別するDB
	// GUID をキーに DLL 名を得る
	struct TUNER_SPECIAL_DLL {
		wstring sTunerGUID;
		wstring sDLLBaseName;
	};
	static const TUNER_SPECIAL_DLL aTunerSpecialData[];

	// チャンネル名自動生成 inline 関数
	inline int MakeChannelName(WCHAR* pszName, size_t size, CBonTuner::ChData* pChData)
	{
		long m = pChData->Frequency / 1000;
		long k = pChData->Frequency % 1000;
		if (k == 0)
			return ::swprintf_s(pszName, size, L"%s/%05ld%c/%s", m_sSatelliteName[pChData->Satellite].c_str(), m, PolarisationChar[pChData->Polarisation], m_sModulationName[pChData->ModulationType].c_str());
		else {
			return ::swprintf_s(pszName, size, L"%s/%05ld.%03ld%c/%s", m_sSatelliteName[pChData->Satellite].c_str(), m, k, PolarisationChar[pChData->Polarisation], m_sModulationName[pChData->ModulationType].c_str());

		}
	}
};

