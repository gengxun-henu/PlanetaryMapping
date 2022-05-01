//  [5/15/2014 gengxun]，此文件不建议使用，改为使用QImgMatchTH中的功能

#ifndef QIMGMATCH_ESTIMATE_H
#define QIMGMATCH_ESTIMATE_H

/*
#include "qimgmatch_global.h"

#include "gdal_priv.h"

#include <string>
#include <vector>
using namespace std;

// 一般情况下，如果已有重叠区域的一定数量的同名点，任意给出左像一点，可百分百给出右像一点
enum EMATCH_RESULT
{
	EMATCH_RESULT_NONE = 0,
	EMATCH_RESULT_GOOD = 1,		// 比较可信，预测基础上使用NCC匹配得出，精度在1个像素
	EMATCH_RESULT_NORM = 2,		// 精度一般，由已知点直接预测得出，精度由已知点决定，一般为2~3像素
	EMATCH_RESULT_BAD  = 3,		// 预测结果不好，如超出边界，或者搜索像素距离太大，如超过1000个像素
	EMATCH_RESULT_FAIL = 4,     // 预测失败，如输入，输出参数不合适
};

namespace ImgMatch
{
	struct MyPt2
	{
		int id;
		double x;
		double y;
	};


	struct ImgData
	{
		int width;
		int height;
		GByte* pImgData;
	};


	// 默认搜索、匹配窗口大小均为11*11，当点数多时，搜索窗口可以小一些，比如7*7，搜索窗口仍然使用11*11
	struct NCC
	{
		int nMatchWindowWidth;		
		int nMatchWindowHeight;
		int nSearchWindowWdith;
		int nSearchWindowHeight;
		double dfThreashold;      // 默认为0.75
	};

	struct SplitPara
	{
		int nWidth;
		int nHeight;
		int nOverlapX;
		int nOverlapY;
	};
}


// ref--reference
// norm--normalization
// ori--original
// src--source

//////////////////////////////////////////////////////////////////////////
// 用户调用此类进行点位估计，步骤如下：
// 1. 先设置匹配图像
// 2. 设置匹配所需要参数文件，内部进行有效性检查
// 3. 调用匹配函数，可由原始图像、分块后图像（未归一化）、分块后归一化图像预测出参考图像上的点坐标

class QIMGMATCH_EXPORT QImgMatchEstimate
{
public:
	QImgMatchEstimate(void);
	~QImgMatchEstimate(void);

public:
	//////////////////////////////////////////////////////////////////////////
	// 用户接口
	bool setMatchImg(const string& strRefImgNorm, const string& strMatchImgNorm);
	bool setMatchParameter(const string& strNormParaFile, const string& strRefImgNormKey, const string& strMatchImgNormKey);
	
	// 给出归一化匹配图像上一点，估计出归一化参考图像上一点
	EMATCH_RESULT normMatchImgToNormRefImg(double x, double y, double& mx, double &my);
	// 给出分块后的重叠区域匹配图像上一点，估计出归一化参考图像上一点（归一化参考图像与分块重叠区域图像相同）
	EMATCH_RESULT overlapMatchImgToOverlapRefImg(double x1, double y1, double&x2, double& y2);
	// 给出原始匹配图像上一点，估计出归一化参考图像上一点（间接调用其它函数）
	EMATCH_RESULT oriMatchImgToNormRefImg(double x1, double y1, double& x2, double& y2);

	// 根据已知点的多少，设置不同NCC，点多时NCC可小一些
	void setNCC(ImgMatch::NCC& ncc) {_ncc = ncc;}
	void setDefaultNCC();
	void setDefaultSmallNCC();
	
	// 外部函数调用，判断打开的图像是否可以使用当前建立的匹配关系进行预测，即如果是GFB，则是否满足条件
	bool IsCurrentMatchImg(const string& strFile);
	bool canBeMatched(const string& strFile, vector<QImgMatchEstimate*>& vecImgMatcher ); // 文件名称是否包含匹配图像与参考图像
	bool IsOverlapImg(const string& strFile);	// 文件是否是匹配图像的Overlap图像
	bool IsNormalizedReferenceImage(const string& strFile); // 文件是否是参考图像的归一化图像

	// 从原始图像坐标转换至Overlap分块图像坐标（针对匹配图像，如GFB）
	bool FromOriToOverlap(double xOri, double yOri, double& xOverlap, double& yOverlap);

	// 从分块图像坐标转换至原始大图像坐标（针对匹配图像，如GFB）
	bool FromOvarlapToOri(double xOverlap, double yOverlap, double& xOri, double& yOri);

	// 查找分块图像对应的行、列
	bool FindSplitRowCol( const string& file, int& row, int& col );

protected:
	//////////////////////////////////////////////////////////////////////////
	// 以下为内部调用
	bool readKeyFile(const string& keyFile, vector<ImgMatch::MyPt2>& vecKeypoint, int& nWidth, int& nHeight);
	bool readNormParaFile(const string& normParafile);

	string getShortFilename(const string& strFilepath, bool bHasExt = false);

	bool readImgData(const string& imgFile, ImgMatch::ImgData& imgData);
	void clearImgData();

	bool readSplitInfo(const string& splitFile, ImgMatch::SplitPara& splitPara);

	//////////////////////////////////////////////////////////////////////////
	// 点位估计，最邻近点查找
	bool estimatePosition(const ImgMatch::MyPt2& keypoint, const vector<ImgMatch::MyPt2>& vecKeypoint1, const vector<ImgMatch::MyPt2>& vecKeypoint2, ImgMatch::MyPt2& pointEstimated);
	bool searchNearestPoints(const ImgMatch::MyPt2 & point, const vector<ImgMatch::MyPt2>& vecKeyPointTotal, double searchRadiusInPixels, int nMaxIterTimes, int nMinPointsNeeded, vector<ImgMatch::MyPt2>& nearestPoints, vector<int>& nearestPointsID);
	
	bool searchNearestPointsByKDTree(const ImgMatch::MyPt2 & point );
	//////////////////////////////////////////////////////////////////////////
	// 矩阵操作函数
	int  Gauss(double *a,double *b,int n);
	void transpose(double *a,double *b, int m, int n);
	void Multiply(double *a,double *b,int m,int n,int k,double *c);	

	//////////////////////////////////////////////////////////////////////////
	// NCC匹配
	double NormalizedCrossCorrelation(void* pData1, void* pData2, GDALDataType eDataType,
		int nWidth1, int nHeight1, int nWidth2, int nHeigh2,
		int x1, int y1, int x2, int y2,
		int nMatchWindowWidth, int nMatchWindowHeight);
	bool NCCMatch(void* pData1, void* pData2, GDALDataType eDataType,	// 图像数据
		int nWidth1, int nHeight1, int nWidth2, int nHeight2,	// 图像宽度、高度
		int x1, int y1, int pre_x2, int pre_y2,					// 左影像x1,y1与其预测右影像点位x2,y2
		int nMatchWindowWidth, int nMatchWindowHeight,			// 匹配窗口
		int nSearchWindowWidth, int nSearchWindowHeight,		// 搜索窗口
		double dfThresholdNCC,									// 相关系数阈值
		int& match_x2, int& match_y2							// 匹配出的点位，若为-1，-1表示未匹配成功
		);
	double NormalizedCrossCorrelationGByte(const vector<GByte>& vecImgData1, const vector<GByte>& vecImgData2, int nWindowWidth, int nWindowHeight);
	double NormalizedCrossCorrelationGUInt16(const vector<GUInt16>& vecImgData1, const vector<GUInt16>& vecImgData2, int nWindowWidth, int nWindowHeight);

private:
	string _strNormParaFile;			// 归一化参数文件
	string _strOriRefImg;				// 原始参考图像文件路径
	string _strOriMatchImg;				// 原始匹配图像文件路径
	string _strRefImgNormKey;			// 参考图像归一化后的匹配点文件
	string _strMatchImgNormKey;			// 匹配图像归一化后的匹配点文件
	string _strRefImgNorm;				// 参考图像归一化后的文件路径（一个分块）
	string _strMatchImgNorm;			// 匹配图像归一化后的文件路径（一个分块）

	ImgMatch::ImgData _refImgData;				// 归一化参考图像数据
	ImgMatch::ImgData _matchImgData;			// 归一化匹配图像数据

	ImgMatch::NCC _ncc;

	//////////////////////////////////////////////////////////////////////////
	// 由归一化参数文件中获取
	double _scale;								// 匹配图像缩放至参考图像
	// 原始匹配图像重叠区域
	int _nLeftMatch;						
	int _nTopMatch;
	int _nRightMatch;
	int _nBottomMatch;
	// 原始参考图像重叠区域
	int _nLeftRef;
	int _nTopRef;
	int _nRightRef;
	int _nBottomRef;

	//////////////////////////////////////////////////////////////////////////
	// 由归一化图像匹配点文件中获取
	vector<ImgMatch::MyPt2> _vecPtRefNorm;		// 参考图像归一化后的匹配点
	vector<ImgMatch::MyPt2> _vecPtMatchNorm;		// 匹配图像归一化后的匹配点

	int _nNormImgWidthMatch;		// 归一化后匹配图像一个分块的宽度，与key文件对应
	int _nNormImgHeightMatch;		// 归一化后匹配图像一个分块的高度，与key文件对应
	int _nNormImgWidthRef;			// 归一化后参考图像一个分块的宽度，与key文件对应		
	int _nNormImgHeightRef;			// 归一化后参考图像一个分块的高度，与key文件对应

	// 分割图像信息
	int _nCurSplitRow;
	int _nCurSplitCol;

	ImgMatch::SplitPara _MatchImgOverlapSplitInfo; // 原始图像Overlap重叠区域（未变化分辨率）分割信息
	//ImgMatch::SplitPara _MatchImgNormSplitInfo;    // 原始图像归一化后的分割信息
	ImgMatch::SplitPara _RefImgNormSplitInfo;	   // 参考图像归一化后的分割信息
};

*/

#endif // QIMGMATCH_ESTIMATE_H
